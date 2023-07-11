#include "render_resource.h"

#include "core/scope_guard.h"
#include "material/pbr_material.h"

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#define TINYGLTF_IMPLEMENTATION
#include "tinygltf/tiny_gltf.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace lumi {

void RenderResource::Init() {
    // Create descriptor allocator
    descriptor_allocator_.Init(rhi->device());
    descriptor_layout_cache_.Init(rhi->device());
    dtor_queue_resource_.Push([this]() {
        descriptor_layout_cache_.Finalize();
        descriptor_allocator_.Finalize();
    });

    InitGlobalResource();

    InitMeshInstancesResource();

    InitDefaultTextures();
}

void RenderResource::InitGlobalResource() {
    // --- Resource buffer ---
    size_t cam_size   = rhi->PaddedSizeOfSSBO<CamDataSSBO>();
    size_t env_size   = rhi->PaddedSizeOfSSBO<EnvDataSSBO>();
    size_t alloc_size = rhi->kFramesInFlight * (cam_size + env_size);
    global.buffer     = rhi->AllocateBuffer(
        alloc_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Map resource staging buffer memory to pointer
    global.staging_buffer =
        rhi->AllocateBuffer(alloc_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VMA_MEMORY_USAGE_CPU_ONLY);
    global.data.begin = rhi->MapMemory(&global.staging_buffer);

    dtor_queue_resource_.Push([this]() {
        rhi->UnmapMemory(&global.staging_buffer);
        rhi->DestroyBuffer(&global.staging_buffer);
        rhi->DestroyBuffer(&global.buffer);
    });

    // --- Build descriptor set ---
    auto editor = EditDescriptorSet(&global.descriptor_set);

    editor.BindBuffer(kGlobalBindingCamera,  //
                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      global.buffer.buffer, 0, sizeof(CamDataSSBO));
    editor.BindBuffer(kGlobalBindingEnvironment,  //
                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                      global.buffer.buffer, 0, sizeof(EnvDataSSBO));

    editor.Execute();
}

void RenderResource::InitMeshInstancesResource() {
    // --- Resource buffer ---
    size_t size =
        rhi->PaddedSizeOfSSBO(sizeof(MeshInstanceSSBO) * kMaxVisibleObjects);
    size_t alloc_size     = rhi->kFramesInFlight * size;
    mesh_instances.buffer = rhi->AllocateBuffer(
        alloc_size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // Map resource staging buffer memory to pointer
    mesh_instances.staging_buffer =
        rhi->AllocateBuffer(alloc_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VMA_MEMORY_USAGE_CPU_ONLY);
    mesh_instances.data.begin = rhi->MapMemory(&mesh_instances.staging_buffer);

    dtor_queue_resource_.Push([this]() {
        rhi->UnmapMemory(&mesh_instances.staging_buffer);
        rhi->DestroyBuffer(&mesh_instances.staging_buffer);
        rhi->DestroyBuffer(&mesh_instances.buffer);
    });

    // --- Build descriptor set ---
    auto editor = EditDescriptorSet(&mesh_instances.descriptor_set);

    editor.BindBuffer(kMeshInstanceBinding,
                      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                      VK_SHADER_STAGE_VERTEX_BIT, mesh_instances.buffer.buffer,
                      0, sizeof(MeshInstanceSSBO) * kMaxVisibleObjects);

    editor.Execute();
}

void RenderResource::InitDefaultTextures() {
    // Samplers
    VkSamplerCreateInfo info_nearest =
        vk::BuildSamplerCreateInfo(VK_FILTER_NEAREST);
    CreateSampler("nearest", &info_nearest);

    VkSamplerCreateInfo info_linear =
        vk::BuildSamplerCreateInfo(VK_FILTER_LINEAR);
    CreateSampler("linear", &info_linear);

    VkSamplerCreateInfo info_hdr = vk::BuildSamplerCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    info_hdr.anisotropyEnable        = VK_TRUE;
    info_hdr.maxAnisotropy           = rhi->max_sampler_anisotropy();
    info_hdr.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info_hdr.unnormalizedCoordinates = VK_FALSE;
    info_hdr.compareEnable           = VK_FALSE;
    info_hdr.compareOp               = VK_COMPARE_OP_ALWAYS;
    info_hdr.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info_hdr.minLod                  = 0.0f;
    info_hdr.maxLod                  = 0.0f;
    CreateSampler("hdr", &info_hdr);

    info_hdr.minLod     = 0.0f;
    info_hdr.maxLod     = 6.0f;  // TODO: irradiance_texture_miplevels
    info_hdr.mipLodBias = 0.0f;
    CreateSampler("cubemap", &info_hdr);

    // srgb
    vk::TextureCreateInfo tex_info{};
    tex_info.width  = 1;
    tex_info.height = 1;
    tex_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    tex_info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    tex_info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    tex_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    tex_info.sampler_name = "nearest";

    // clang-format off
    CreateTexture2D("white", &tex_info, &Color4u8::kWhite);
    CreateTexture2D("black", &tex_info, &Color4u8::kBlack);
    CreateTexture2D("red",   &tex_info, &Color4u8::kRed);
    CreateTexture2D("green", &tex_info, &Color4u8::kGreen);
    CreateTexture2D("blue",  &tex_info, &Color4u8::kBlue);
    // clang-format on

    // linear
    tex_info.format         = VK_FORMAT_R8G8B8A8_UNORM;
    Color4u8 normal_default = Color4u8(128, 128, 255, 255);
    CreateTexture2D("normal_default", &tex_info, &normal_default);

    // lut
    CreateTextureHDRFromFile("lut_brdf", "textures/lut/brdf.hdr");
}

void RenderResource::Finalize() { dtor_queue_resource_.Flush(); }

void RenderResource::PushDestructor(std::function<void()> &&destructor) {
    dtor_queue_resource_.Push(std::move(destructor));
}

void RenderResource::ResetMappedPointers() {
    int idx = rhi->frame_idx();

    // global
    {
        size_t cam_size = rhi->PaddedSizeOfSSBO<CamDataSSBO>();
        size_t env_size = rhi->PaddedSizeOfSSBO<EnvDataSSBO>();

        char *begin = (char *)(global.data.begin);
        char *cam   = begin + idx * (cam_size + env_size);
        char *env   = cam + cam_size;

        global.data.cam = reinterpret_cast<CamDataSSBO *>(cam);
        global.data.env = reinterpret_cast<EnvDataSSBO *>(env);
    }
    // Mesh Instances
    {
        size_t size = rhi->PaddedSizeOfSSBO(sizeof(MeshInstanceSSBO) *
                                            kMaxVisibleObjects);

        char *begin     = (char *)(mesh_instances.data.begin);
        char *instances = begin + idx * size;

        mesh_instances.data.cur_instance =
            reinterpret_cast<MeshInstanceSSBO *>(instances);
    }
}

std::vector<uint32_t> RenderResource::GlobalSSBODynamicOffsets() const {
    auto res = std::vector<uint32_t>(kGlobalBindingCount);

    size_t cam_size = rhi->PaddedSizeOfSSBO<CamDataSSBO>();
    size_t env_size = rhi->PaddedSizeOfSSBO<EnvDataSSBO>();

    res[0] = (uint32_t)(cam_size + env_size) * rhi->frame_idx();
    res[1] = res[0] + (uint32_t)cam_size;
    return res;
}

std::vector<uint32_t> RenderResource::MeshInstanceSSBODynamicOffsets() const {
    auto res = std::vector<uint32_t>(kMeshInstanceBindingCount);

    size_t size =
        rhi->PaddedSizeOfSSBO(sizeof(MeshInstanceSSBO) * kMaxVisibleObjects);

    res[0] = (uint32_t)size * rhi->frame_idx();
    return res;
}

VkShaderModule RenderResource::GetShaderModule(const std::string &name,
                                               ShaderType         type) {
    auto it = shaders_[type].find(name);
    if (it == shaders_[type].end()) {
        return VK_NULL_HANDLE;
    } else {
        return it->second;
    }
}

vk::Texture *RenderResource::GetTexture(const std::string &name) {
    auto it = textures_.find(name);
    if (it == textures_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

VkSampler RenderResource::GetSampler(const std::string &name) {
    auto it = samplers_.find(name);
    if (it == samplers_.end()) {
        return VK_NULL_HANDLE;
    } else {
        return it->second;
    }
}

Material *RenderResource::GetMaterial(const std::string &name) {
    auto it = materials_.find(name);
    if (it == materials_.end()) {
        return nullptr;
    } else {
        return it->second.get();
    }
}

Mesh *RenderResource::GetMesh(const std::string &name) {
    auto it = meshes_.find(name);
    if (it == meshes_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

VkShaderModule RenderResource::CreateShaderModule(const std::string &name,
                                                  ShaderType         type) {
    VkShaderModule res = GetShaderModule(name, type);
    if (res) return res;

    const char *postfix = "";
    switch (type) {
        case kShaderTypeVertex:
            postfix = ".vert.spv";
            break;
        case kShaderTypeFragment:
            postfix = ".frag.spv";
            break;
        case kShaderTypeCompute:
            postfix = ".comp.spv";
            break;
        default:
            LOG_ERROR("Unknown shader type {} when creating {}", type, name);
            return nullptr;
    }

    auto        p_shader = &shaders_[type][name];
    std::string filepath = LUMI_SHADERS_DIR "/" + name + postfix;
    if (!LoadVkShaderModule(filepath, p_shader)) {
        LOG_ERROR("Error when loading shader from {}", filepath);
    }

    dtor_queue_resource_.Push([this, p_shader]() {
        vkDestroyShaderModule(rhi->device(), *p_shader, nullptr);
    });
    return *p_shader;
}

VkSampler RenderResource::CreateSampler(const std::string   &name,
                                        VkSamplerCreateInfo *info) {
    VkSampler res = GetSampler(name);
    if (res) return res;

    auto p_sampler = &samplers_[name];
    vkCreateSampler(rhi->device(), info, nullptr, p_sampler);
    dtor_queue_resource_.Push([this, p_sampler]() {
        vkDestroySampler(rhi->device(), *p_sampler, nullptr);
    });
    return *p_sampler;
}

Material *RenderResource::CreateMaterial(const std::string &name,
                                         const std::string &type_name,
                                         VkRenderPass       render_pass) {
    Material *res = GetMaterial(name);
    if (res) {
        LOG_WARNING("Create material with an existed name {}", name);
        return res;
    }

    auto material_type = meta::Type::get_by_name(type_name);
    if (!material_type) {
        LOG_ERROR("Unknown material type {}", type_name);
        return nullptr;
    }
    auto material =
        material_type.create().get_value<std::shared_ptr<Material>>();
    materials_[name] = material;

    if (render_pass == VK_NULL_HANDLE) {
        render_pass = rhi->main_render_pass();
    }
    material->CreateDescriptorSet(this);
    material->CreatePipeline(this, render_pass);
    return material.get();
}

Mesh *RenderResource::CreateMeshFromObjFile(const std::string &name,
                                            const fs::path    &filepath) {
    Mesh *res = GetMesh(name);
    if (res) {
        LOG_WARNING("Create mesh with an existed name {}", name);
        return res;
    }

    auto &absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto in = std::ifstream(absolute_path);

    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                     absolute_path.string().c_str(),
                     absolute_path.parent_path().string().c_str());
    if (!warn.empty()) {
        LOG_WARNING(warn.c_str());
    }
    if (!err.empty()) {
        LOG_ERROR(err.c_str());
        return nullptr;
    }

    auto &mesh = meshes_[name];

    auto vertex_hashmap =
        std::unordered_map<vk::Vertex, Mesh::IndexType, vk::Vertex::Hash>();

    // Loop over shapes
    for (const auto &shape : shapes) {
        for (const auto &idx : shape.mesh.indices) {
            // vertex position
            tinyobj::real_t vx = attrib.vertices[3LL * idx.vertex_index + 0];
            tinyobj::real_t vy = attrib.vertices[3LL * idx.vertex_index + 1];
            tinyobj::real_t vz = attrib.vertices[3LL * idx.vertex_index + 2];
            // vertex normal
            tinyobj::real_t nx = attrib.normals[3LL * idx.normal_index + 0];
            tinyobj::real_t ny = attrib.normals[3LL * idx.normal_index + 1];
            tinyobj::real_t nz = attrib.normals[3LL * idx.normal_index + 2];
            // vertex texcoord
            tinyobj::real_t tx = attrib.texcoords[2LL * idx.texcoord_index + 0];
            tinyobj::real_t ty = attrib.texcoords[2LL * idx.texcoord_index + 1];
            // vertex colors
            tinyobj::real_t r = attrib.colors[3LL * idx.vertex_index + 0];
            tinyobj::real_t g = attrib.colors[3LL * idx.vertex_index + 1];
            tinyobj::real_t b = attrib.colors[3LL * idx.vertex_index + 2];

            // copy it into our vertex
            vk::Vertex new_vert{};
            new_vert.position.x  = vx;
            new_vert.position.y  = vy;
            new_vert.position.z  = vz;
            new_vert.normal.x    = nx;
            new_vert.normal.y    = ny;
            new_vert.normal.z    = nz;
            new_vert.texcoord0.x = tx;
            new_vert.texcoord0.y = 1.0f - ty;
            new_vert.color.r     = r;
            new_vert.color.g     = g;
            new_vert.color.b     = b;

            auto it = vertex_hashmap.find(new_vert);
            if (it == vertex_hashmap.end()) {
                vertex_hashmap[new_vert] =
                    (Mesh::IndexType)mesh.vertices.size();
                mesh.vertices.emplace_back(new_vert);
            }

            mesh.indices.emplace_back(vertex_hashmap[new_vert]);
        }
    }

    UploadMesh(&mesh);
    return &mesh;
}

vk::Texture *RenderResource::CreateTexture2DFromFile(const std::string &name,
                                                     const fs::path &filepath,
                                                     bool            is_srgb) {

    vk::Texture *res = GetTexture(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    int   texWidth, texHeight, texChannels;
    auto &absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    stbi_uc *pixels = stbi_load(absolute_path.string().c_str(), &texWidth,
                                &texHeight, &texChannels, STBI_default);
    if (!pixels) {
        LOG_ERROR("Failed to load texture file {}", filepath);
        return nullptr;
    }

    VkFormat           format = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;
    switch (texChannels) {
        case STBI_rgb_alpha:
            format =
                is_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case STBI_grey:
            format = is_srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        default:
            LOG_ERROR("Unknown image format when loading {}", filepath);
    }

    vk::TextureCreateInfo info{};
    info.width  = texWidth;
    info.height = texHeight;
    info.format = format;
    info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.memory_usage    = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags    = aspect;
    info.sampler_name    = "nearest";
    vk::Texture *texture = CreateTexture2D(name, &info, pixels);

    stbi_image_free(pixels);
    return texture;
}

vk::Texture *RenderResource::CreateTextureHDRFromFile(
    const std::string &name, const fs::path &filepath) {

    vk::Texture *res = GetTexture(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    int   texWidth, texHeight, texChannels;
    auto &absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    float *pixels =
        stbi_loadf(absolute_path.string().c_str(), &texWidth, &texHeight,
                   &texChannels, STBI_rgb_alpha);  // required 4-channels
    if (!pixels) {
        LOG_ERROR("Failed to load texture file {}", filepath);
        return nullptr;
    }

    vk::TextureCreateInfo info{};
    info.width  = texWidth;
    info.height = texHeight;
    info.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.memory_usage    = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags    = VK_IMAGE_ASPECT_COLOR_BIT;
    info.sampler_name    = "nearest";
    vk::Texture *texture = CreateTexture2D(name, &info, pixels);

    stbi_image_free(pixels);
    return texture;
}

vk::Texture *RenderResource::CreateTextureCubeMapFromFile(
    const std::string &name, const fs::path &basepath) {
    vk::Texture *res = GetTexture(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    auto &absolute_path =
        basepath.is_absolute() ? basepath : LUMI_ASSETS_DIR / basepath;
    // assume all textures have same width, height and format
    int texWidth, texHeight, texChannels;

    std::array<const char *, 6> faces = {
        "_X+.hdr", "_X-.hdr", "_Z+.hdr", "_Z-.hdr", "_Y+.hdr", "_Y-.hdr",
    };
    std::array<void *, 6> image_datas{};
    for (int i = 0; i < 6; i++) {
        std::string &cur_name = absolute_path.string() + faces[i];
        float       *pixels =
            stbi_loadf(cur_name.c_str(), &texWidth, &texHeight, &texChannels,
                       STBI_rgb_alpha);  // required 4-channels
        if (!pixels) {
            LOG_ERROR("Failed to load texture file {}", cur_name);
            return nullptr;
        }
        image_datas[i] = pixels;
    }

    vk::TextureCreateInfo info{};
    info.width       = texWidth;
    info.height      = texHeight;
    info.format      = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.image_usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                       VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    info.sampler_name = "cubemap";

    uint32_t mip_levels =
        uint32_t(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    vk::Texture *texture =
        CreateTextureCubeMap(name, &info, image_datas, mip_levels);

    for (auto &data : image_datas) {
        stbi_image_free(data);
    }
    return texture;
}

vk::Texture *RenderResource::CreateTexture2D(const std::string     &name,  //
                                             vk::TextureCreateInfo *info,  //
                                             const void            *pixels) {

    vk::Texture *res = GetTexture(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    vk::Texture *texture = &textures_[name];
    rhi->AllocateTexture2D(texture, info);
    UploadTexture2D(texture, pixels, info->aspect_flags);

    VkSampler sampler = GetSampler(info->sampler_name);
    if (!sampler) {
        LOG_WARNING("Unknown sampler name {} when creating texture {}",
                    info->sampler_name, name);
    }
    texture->sampler_name = info->sampler_name;

    dtor_queue_resource_.Push(
        [this, texture]() { rhi->DestroyTexture(texture); });
    return texture;
}

vk::Texture *RenderResource::CreateTextureCubeMap(const std::string     &name,
                                                  vk::TextureCreateInfo *info,
                                                  std::array<void *, 6>  pixels,
                                                  uint32_t mip_levels) {
    vk::Texture *res = GetTexture(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    vk::Texture *texture = &textures_[name];
    rhi->AllocateTextureCubeMap(texture, info, mip_levels);
    UploadTextureCubeMap(texture, pixels, info->aspect_flags, mip_levels);

    VkSampler sampler = GetSampler(info->sampler_name);
    if (!sampler) {
        LOG_WARNING("Unknown sampler name {} when creating texture {}",
                    info->sampler_name, name);
    }
    texture->sampler_name = info->sampler_name;

    dtor_queue_resource_.Push(
        [this, texture]() { rhi->DestroyTexture(texture); });
    return texture;
}

bool RenderResource::LoadVkShaderModule(const std::string &filepath,
                                        VkShaderModule    *p_shader_module) {
    auto shader_file =
        std::ifstream(filepath, std::ios::ate | std::ios::binary);
    if (!shader_file.is_open()) {
        return false;
    }
    // The cursor is at the end, it gives the size directly in bytes
    size_t file_size = (size_t)shader_file.tellg();

    std::vector<char> buffer(file_size);
    shader_file.seekg(0);
    shader_file.read(buffer.data(), file_size);
    shader_file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.codeSize = buffer.size();
    createInfo.pCode    = (uint32_t *)buffer.data();

    if (vkCreateShaderModule(rhi->device(), &createInfo, nullptr,
                             p_shader_module) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void RenderResource::UploadMesh(Mesh *mesh) {
    {
        // vertex buffer
        const size_t buffer_size = mesh->vertices.size() * sizeof(vk::Vertex);

        // Create staging buffer & dst buffer
        auto staging_buffer =
            rhi->AllocateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VMA_MEMORY_USAGE_CPU_ONLY);
        ScopeGuard guard = [this, &staging_buffer]() {
            rhi->DestroyBuffer(&staging_buffer);
        };
        mesh->vertex_buffer = rhi->AllocateBuffer(  //
            buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        dtor_queue_resource_.Push(
            [this, mesh]() { rhi->DestroyBuffer(&mesh->vertex_buffer); });

        // data -> staging buffer
        rhi->CopyBuffer(mesh->vertices.data(), &staging_buffer, buffer_size);
        // staging buffer -> dst buffer
        rhi->CopyBuffer(&staging_buffer, &mesh->vertex_buffer, buffer_size);
    }
    {
        // index buffer
        const size_t buffer_size =
            mesh->indices.size() * sizeof(Mesh::IndexType);

        // Create staging buffer & dst buffer
        auto staging_buffer =
            rhi->AllocateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VMA_MEMORY_USAGE_CPU_ONLY);
        ScopeGuard guard = [this, &staging_buffer]() {
            rhi->DestroyBuffer(&staging_buffer);
        };
        mesh->index_buffer = rhi->AllocateBuffer(  //
            buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);
        dtor_queue_resource_.Push(
            [this, mesh]() { rhi->DestroyBuffer(&mesh->index_buffer); });

        // data -> staging buffer
        rhi->CopyBuffer(mesh->indices.data(), &staging_buffer, buffer_size);
        // staging buffer -> dst buffer
        rhi->CopyBuffer(&staging_buffer, &mesh->index_buffer, buffer_size);
    }
}

void RenderResource::UploadTexture2D(vk::Texture *texture, const void *pixels,
                                     VkImageAspectFlags aspect) {

    VkDeviceSize channels     = 0;
    size_t       element_size = sizeof(char);
    switch (texture->format) {
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            element_size = sizeof(float);
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
            channels = 4;
            break;
        case VK_FORMAT_R32_SFLOAT:
            element_size = sizeof(float);
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_UNORM:
            channels = 1;
            break;
        default:
            LOG_ERROR("Unknown texture format {}", texture->format);
            break;
    }
    VkDeviceSize image_size =
        channels * texture->width * texture->height * element_size;

    // allocate temporary buffer for holding texture data to upload
    vk::AllocatedBuffer staging_buffer =
        rhi->AllocateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VMA_MEMORY_USAGE_CPU_ONLY);
    ScopeGuard guard = [this, &staging_buffer]() {
        rhi->DestroyBuffer(&staging_buffer);
    };

    // data -> staging buffer
    rhi->CopyBuffer(pixels, &staging_buffer, image_size);

    // staging buffer -> texture
    rhi->ImmediateSubmit([this, texture, aspect,
                          &staging_buffer](VkCommandBuffer cmd) {
        // --- Transit image layout to transfer_dst ---
        rhi->CmdImageLayoutTransition(cmd, texture->image.image, aspect,
                                      VK_IMAGE_LAYOUT_UNDEFINED,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // --- Copy data to texture ---
        rhi->CmdCopyBufferToImage(cmd, staging_buffer.buffer,
                                  texture->image.image, aspect, texture->width,
                                  texture->height);

        // --- Transit image layout to shader readable ---
        rhi->CmdImageLayoutTransition(cmd, texture->image.image, aspect,
                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
}

void RenderResource::UploadTextureCubeMap(vk::Texture          *texture,
                                          std::array<void *, 6> pixels,
                                          VkImageAspectFlags    aspect,
                                          uint32_t              mip_levels) {
    VkDeviceSize channels     = 0;
    size_t       element_size = sizeof(char);
    switch (texture->format) {
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            element_size = sizeof(float);
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
            channels = 4;
            break;
        case VK_FORMAT_R32_SFLOAT:
            element_size = sizeof(float);
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_UNORM:
            channels = 1;
            break;
        default:
            LOG_ERROR("Unknown texture format {}", texture->format);
            break;
    }
    VkDeviceSize image_size =
        channels * texture->width * texture->height * element_size;
    VkDeviceSize cube_size = image_size * 6;

    // allocate temporary buffer for holding texture data to upload
    vk::AllocatedBuffer staging_buffer = rhi->AllocateBuffer(
        cube_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    char *dst_data = (char *)rhi->MapMemory(&staging_buffer);

    ScopeGuard guard = [this, &staging_buffer]() {
        rhi->UnmapMemory(&staging_buffer);
        rhi->DestroyBuffer(&staging_buffer);
    };

    // data -> staging buffer
    VkDeviceSize offset = 0;
    for (int i = 0; i < 6; i++) {
        char *src_data = (char *)pixels[i];
        memcpy(dst_data + offset, src_data, image_size);
        offset += image_size;
    }

    // staging buffer -> cubemap
    rhi->ImmediateSubmit([this, texture, aspect, mip_levels,
                          &staging_buffer](VkCommandBuffer cmd) {
        // --- Transit image layout to transfer_dst ---
        rhi->CmdImageLayoutTransition(
            cmd, texture->image.image, aspect, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels, 6);

        // --- Copy mip level 0 data to texture ---
        rhi->CmdCopyBufferToImage(cmd, staging_buffer.buffer,
                                  texture->image.image, aspect, texture->width,
                                  texture->height, 6);

        // --- Generate mipmaps & transit to shader readable---
        rhi->CmdGenerateMipMaps(cmd, texture, aspect, mip_levels, 6);
    });
}

void RenderResource::LoadFromGLTFFile(const fs::path &filepath) {
    auto &absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;

    tinygltf::Model    gltf_model;
    tinygltf::TinyGLTF gltf_context;

    std::string error;
    std::string warning;

    bool loaded =
        (absolute_path.extension() == ".glb")
            ? gltf_context.LoadBinaryFromFile(&gltf_model, &error, &warning,
                                              absolute_path.string().c_str())
            : gltf_context.LoadASCIIFromFile(&gltf_model, &error, &warning,
                                             absolute_path.string().c_str());

    if (!warning.empty()) {
        LOG_WARNING(warning.c_str());
    }
    if (!loaded || !error.empty()) {
        LOG_ERROR(warning.c_str());
        return;
    }

    auto &name = absolute_path.stem().string();
    GLTFLoadMaterials(name, gltf_model);

    // Load Mesh
    const tinygltf::Scene &scene =
        gltf_model
            .scenes[gltf_model.defaultScene > -1 ? gltf_model.defaultScene : 0];
    auto &mesh = meshes_[name];

    for (size_t i = 0; i < scene.nodes.size(); i++) {
        const tinygltf::Node node = gltf_model.nodes[scene.nodes[i]];
        GLTFLoadMesh(gltf_model, node, scene.nodes[i], &mesh);
    }
    UploadMesh(&mesh);
}

void RenderResource::GLTFLoadTexture(const std::string &name,
                                     tinygltf::Model &gltf_model, int idx,
                                     bool is_srgb) {
    const std::string &tex_name = name + "_tex_" + std::to_string(idx);
    if (GetTexture(tex_name) != nullptr) return;

    tinygltf::Texture &tex   = gltf_model.textures[idx];
    tinygltf::Image   &image = gltf_model.images[tex.source];

    vk::TextureCreateInfo info{};
    info.width  = image.width;
    info.height = image.height;
    info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

    switch (image.component) {
        case 1:
            info.format = is_srgb ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;
            break;
        case 4:
            info.format =
                is_srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            LOG_ERROR("Unsupported image component number {}", image.component);
            break;
    }

    if (tex.sampler == -1) {
        // No sampler specified, use a default one
        info.sampler_name = "linear";
    } else {
        tinygltf::Sampler &sampler = gltf_model.samplers[tex.sampler];
        switch (sampler.minFilter) {
            case -1:
            case TINYGLTF_TEXTURE_FILTER_NEAREST:
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
                info.sampler_name = "nearest";
                break;
            case TINYGLTF_TEXTURE_FILTER_LINEAR:
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
                info.sampler_name = "linear";
                break;
            default:
                LOG_ERROR(
                    "Unknown filter mode when loading GLTF texture "
                    "{}_tex_{}",
                    name, idx);
                break;
        }
    }

    CreateTexture2D(tex_name, &info, image.image.data());
}

void RenderResource::GLTFLoadMaterials(const std::string &name,
                                       tinygltf::Model   &gltf_model) {
    for (size_t i = 0; i < gltf_model.materials.size(); i++) {
        tinygltf::Material &mat = gltf_model.materials[i];

        PBRMaterial *material = (PBRMaterial *)CreateMaterial(
            name + "_mat_" + std::to_string(i), "PBRMaterial");

        if (mat.doubleSided) {
            material->double_sided = true;
        }
        if (mat.values.find("baseColorTexture") != mat.values.end()) {
            int tex_idx = mat.values["baseColorTexture"].TextureIndex();
            GLTFLoadTexture(name, gltf_model, tex_idx, true);
            material->base_color_tex_name =
                name + "_tex_" + std::to_string(tex_idx);
            material->params.data->texcoord_set_base_color =
                mat.values["baseColorTexture"].TextureTexCoord();
        }
        if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
            int tex_idx = mat.values["metallicRoughnessTexture"].TextureIndex();
            GLTFLoadTexture(name, gltf_model, tex_idx, true);
            material->metallic_roughness_tex_name =
                name + "_tex_" + std::to_string(tex_idx);
            material->params.data->texcoord_set_metallic_roughness =
                mat.values["metallicRoughnessTexture"].TextureTexCoord();
        }
        if (mat.values.find("roughnessFactor") != mat.values.end()) {
            material->params.data->roughness_factor =
                static_cast<float>(mat.values["roughnessFactor"].Factor());
        }
        if (mat.values.find("metallicFactor") != mat.values.end()) {
            material->params.data->metallic_factor =
                static_cast<float>(mat.values["metallicFactor"].Factor());
        }
        if (mat.values.find("baseColorFactor") != mat.values.end()) {
            auto color = mat.values["baseColorFactor"].ColorFactor();
            material->params.data->base_color_factor =
                Vec4f(color[0], color[1], color[2], color[3]);
        }
        if (mat.additionalValues.find("normalTexture") !=
            mat.additionalValues.end()) {
            int tex_idx = mat.additionalValues["normalTexture"].TextureIndex();
            GLTFLoadTexture(name, gltf_model, tex_idx, false);
            material->normal_tex_name =
                name + "_tex_" + std::to_string(tex_idx);
            material->params.data->texcoord_set_normal =
                mat.additionalValues["normalTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("emissiveTexture") !=
            mat.additionalValues.end()) {
            int tex_idx =
                mat.additionalValues["emissiveTexture"].TextureIndex();
            GLTFLoadTexture(name, gltf_model, tex_idx, true);
            material->emissive_tex_name =
                name + "_tex_" + std::to_string(tex_idx);
            material->params.data->texcoord_set_emissive =
                mat.additionalValues["emissiveTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("occlusionTexture") !=
            mat.additionalValues.end()) {
            int tex_idx =
                mat.additionalValues["occlusionTexture"].TextureIndex();
            GLTFLoadTexture(name, gltf_model, tex_idx, false);
            material->occlusion_tex_name =
                name + "_tex_" + std::to_string(tex_idx);
            material->params.data->texcoord_set_occlusion =
                mat.additionalValues["occlusionTexture"].TextureTexCoord();
        }
        if (mat.additionalValues.find("alphaMode") !=
            mat.additionalValues.end()) {
            tinygltf::Parameter param = mat.additionalValues["alphaMode"];
            if (param.string_value == "BLEND") {
                material->params.data->alpha_mode =
                    PBRMaterial::kAlphaModeBlend;
            }
            if (param.string_value == "MASK") {
                material->params.data->alpha_mode = PBRMaterial::kAlphaModeMask;
                material->params.data->alpha_cutoff = 0.5f;
            }
        }
        if (mat.additionalValues.find("alphaCutoff") !=
            mat.additionalValues.end()) {
            material->params.data->alpha_cutoff = static_cast<float>(
                mat.additionalValues["alphaCutoff"].Factor());
        }
        if (mat.additionalValues.find("emissiveFactor") !=
            mat.additionalValues.end()) {
            auto color = mat.additionalValues["emissiveFactor"].ColorFactor();
            material->params.data->emissive_factor =
                Vec4f(color[0], color[1], color[2], 1.0f);
        }

        material->Upload(this);
    }
}

void RenderResource::GLTFGetMeshProperties(const tinygltf::Model &gltf_model,
                                           const tinygltf::Node  &gltf_node,
                                           size_t                &vertex_count,
                                           size_t                &index_count) {

    if (gltf_node.children.size() > 0) {
        for (size_t i = 0; i < gltf_node.children.size(); i++) {
            GLTFGetMeshProperties(gltf_model,
                                  gltf_model.nodes[gltf_node.children[i]],
                                  vertex_count, index_count);
        }
    }

    if (gltf_node.mesh > -1) {
        const tinygltf::Mesh mesh = gltf_model.meshes[gltf_node.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            auto &primitive = mesh.primitives[i];
            vertex_count +=
                gltf_model
                    .accessors[primitive.attributes.find("POSITION")->second]
                    .count;
            if (primitive.indices > -1) {
                index_count += gltf_model.accessors[primitive.indices].count;
            }
        }
    }
}

void RenderResource::GLTFLoadMesh(const tinygltf::Model &model,
                                  const tinygltf::Node  &node,
                                  uint32_t node_index, Mesh *mesh) {

    // Node with children
    if (node.children.size() > 0) {
        for (size_t i = 0; i < node.children.size(); i++) {
            GLTFLoadMesh(model, model.nodes[node.children[i]], node.children[i],
                         mesh);
        }
    }
    if (node.mesh == -1) return;

    // Node contains mesh data
    const tinygltf::Mesh gltf_mesh = model.meshes[node.mesh];
    for (size_t j = 0; j < gltf_mesh.primitives.size(); j++) {
        auto &primitive = gltf_mesh.primitives[j];

        uint32_t vertexStart = static_cast<uint32_t>(mesh->vertices.size());
        uint32_t indexStart  = static_cast<uint32_t>(mesh->indices.size());
        uint32_t indexCount  = 0;
        uint32_t vertexCount = 0;
        Vec3f    posMin{};
        Vec3f    posMax{};
        bool     hasIndices = primitive.indices > -1;

        // Vertices
        const float *bufferPos          = nullptr;
        const float *bufferNormals      = nullptr;
        const float *bufferTexCoordSet0 = nullptr;
        const float *bufferTexCoordSet1 = nullptr;
        const float *bufferColorSet0    = nullptr;
        const void  *bufferJoints       = nullptr;
        const float *bufferWeights      = nullptr;

        int posByteStride;
        int normByteStride;
        int uv0ByteStride;
        int uv1ByteStride;
        int color0ByteStride;
        int jointByteStride;
        int weightByteStride;

        int jointComponentType;

        // Position attribute is required
        LOG_ASSERT(primitive.attributes.find("POSITION") !=
                   primitive.attributes.end());

        const tinygltf::Accessor &posAccessor =
            model.accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView &posView =
            model.bufferViews[posAccessor.bufferView];
        bufferPos = reinterpret_cast<const float *>(
            &(model.buffers[posView.buffer]
                  .data[posAccessor.byteOffset + posView.byteOffset]));
        posMin      = Vec3f(posAccessor.minValues[0], posAccessor.minValues[1],
                            posAccessor.minValues[2]);
        posMax      = Vec3f(posAccessor.maxValues[0], posAccessor.maxValues[1],
                            posAccessor.maxValues[2]);
        vertexCount = static_cast<uint32_t>(posAccessor.count);
        posByteStride =
            posAccessor.ByteStride(posView)
                ? (posAccessor.ByteStride(posView) / sizeof(float))
                : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        // Normals
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            const tinygltf::Accessor &normAccessor =
                model.accessors[primitive.attributes.find("NORMAL")->second];
            const tinygltf::BufferView &normView =
                model.bufferViews[normAccessor.bufferView];
            bufferNormals = reinterpret_cast<const float *>(
                &(model.buffers[normView.buffer]
                      .data[normAccessor.byteOffset + normView.byteOffset]));
            normByteStride =
                normAccessor.ByteStride(normView)
                    ? (normAccessor.ByteStride(normView) / sizeof(float))
                    : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        }

        // UVs
        if (primitive.attributes.find("TEXCOORD_0") !=
            primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor =
                model
                    .accessors[primitive.attributes.find("TEXCOORD_0")->second];
            const tinygltf::BufferView &uvView =
                model.bufferViews[uvAccessor.bufferView];
            bufferTexCoordSet0 = reinterpret_cast<const float *>(
                &(model.buffers[uvView.buffer]
                      .data[uvAccessor.byteOffset + uvView.byteOffset]));
            uv0ByteStride =
                uvAccessor.ByteStride(uvView)
                    ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                    : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
        }
        if (primitive.attributes.find("TEXCOORD_1") !=
            primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor =
                model
                    .accessors[primitive.attributes.find("TEXCOORD_1")->second];
            const tinygltf::BufferView &uvView =
                model.bufferViews[uvAccessor.bufferView];
            bufferTexCoordSet1 = reinterpret_cast<const float *>(
                &(model.buffers[uvView.buffer]
                      .data[uvAccessor.byteOffset + uvView.byteOffset]));
            uv1ByteStride =
                uvAccessor.ByteStride(uvView)
                    ? (uvAccessor.ByteStride(uvView) / sizeof(float))
                    : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
        }

        // Vertex colors
        if (primitive.attributes.find("COLOR_0") !=
            primitive.attributes.end()) {
            const tinygltf::Accessor &accessor =
                model.accessors[primitive.attributes.find("COLOR_0")->second];
            const tinygltf::BufferView &view =
                model.bufferViews[accessor.bufferView];
            bufferColorSet0 = reinterpret_cast<const float *>(
                &(model.buffers[view.buffer]
                      .data[accessor.byteOffset + view.byteOffset]));
            color0ByteStride =
                accessor.ByteStride(view)
                    ? (accessor.ByteStride(view) / sizeof(float))
                    : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        }

        for (size_t v = 0; v < posAccessor.count; v++) {
            auto &vert = mesh->vertices.emplace_back();

            auto pos      = &bufferPos[v * posByteStride];
            vert.position = Vec3f(pos[0], pos[1], pos[2]);

            if (bufferNormals) {
                auto normal = &bufferNormals[v * normByteStride];
                vert.normal = Vec3f(normal[0], normal[1], normal[2]);
            }

            if (bufferColorSet0) {
                auto color = &bufferColorSet0[v * color0ByteStride];
                vert.color = Vec3f(color[0], color[1], color[2]);
            }

            if (bufferTexCoordSet0) {
                auto uv0       = &bufferTexCoordSet0[v * uv0ByteStride];
                vert.texcoord0 = Vec2f(uv0[0], uv0[1]);
            }

            if (bufferTexCoordSet1) {
                auto uv1       = &bufferTexCoordSet1[v * uv1ByteStride];
                vert.texcoord1 = Vec2f(uv1[0], uv1[1]);
            }
        }

        // Indices
        if (!hasIndices) continue;

        const tinygltf::Accessor &accessor =
            model.accessors[primitive.indices > -1 ? primitive.indices : 0];
        const tinygltf::BufferView &bufferView =
            model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

        indexCount = static_cast<uint32_t>(accessor.count);
        const void *dataPtr =
            &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

        switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t *buf = static_cast<const uint32_t *>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    mesh->indices.emplace_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t *buf = static_cast<const uint16_t *>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    mesh->indices.emplace_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t *buf = static_cast<const uint8_t *>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    mesh->indices.emplace_back(buf[index] + vertexStart);
                }
                break;
            }
            default:
                LOG_ERROR("Index component type {} not supported",
                          accessor.componentType);
                return;
        }
    }
}

}  // namespace lumi