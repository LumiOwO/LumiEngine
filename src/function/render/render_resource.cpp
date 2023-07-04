#include "render_resource.h"

#include "core/scope_guard.h"

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace lumi {

void RenderResource::Init() {
    descriptor_allocator_.Init(rhi->device());
    descriptor_layout_cache_.Init(rhi->device());
    dtor_queue_resource_.Push([this]() {
        descriptor_layout_cache_.Finalize();
        descriptor_allocator_.Finalize();
    });

    // Per frame
    {
        // --- Resource buffer ---
        size_t alloc_size = rhi->kFramesInFlight *
                            rhi->PaddedSizeOfSSBO<PerFrameBufferObject>();
        per_frame.buffer =
            rhi->AllocateBuffer(alloc_size,
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VMA_MEMORY_USAGE_GPU_ONLY);

        // Map resource staging buffer memory to pointer
        per_frame.staging_buffer =
            rhi->AllocateBuffer(alloc_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VMA_MEMORY_USAGE_CPU_ONLY);
        per_frame.data = rhi->MapMemory(&per_frame.staging_buffer);

        dtor_queue_resource_.Push([this]() {
            rhi->UnmapMemory(&per_frame.staging_buffer);
            rhi->DestroyBuffer(&per_frame.staging_buffer);
            rhi->DestroyBuffer(&per_frame.buffer);
        });

        // TODO: Create per frame textures

        // --- Build descriptor set ---
        auto editor = EditDescriptorSet(&per_frame.descriptor_set);

        editor.BindBuffer(
            0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            per_frame.buffer.buffer, 0, sizeof(PerFrameBufferObject));

        editor.Execute();
    }

    // Per pass
    {
        // --- Resource buffer ---
        size_t alloc_size =
            rhi->kFramesInFlight *
            rhi->PaddedSizeOfSSBO(sizeof(PerVisibleBufferObject) *
                                  kMaxVisibleObjects);
        per_visible.buffer =
            rhi->AllocateBuffer(alloc_size,
                                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VMA_MEMORY_USAGE_GPU_ONLY);

        // Map resource staging buffer memory to pointer
        per_visible.staging_buffer =
            rhi->AllocateBuffer(alloc_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VMA_MEMORY_USAGE_CPU_ONLY);
        per_visible.data = rhi->MapMemory(&per_visible.staging_buffer);

        dtor_queue_resource_.Push([this]() {
            rhi->UnmapMemory(&per_visible.staging_buffer);
            rhi->DestroyBuffer(&per_visible.staging_buffer);
            rhi->DestroyBuffer(&per_visible.buffer);
        });

        // --- Build descriptor set ---
        auto editor = EditDescriptorSet(&per_visible.descriptor_set);

        editor.BindBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
                          VK_SHADER_STAGE_VERTEX_BIT, per_visible.buffer.buffer,
                          0,
                          sizeof(PerVisibleBufferObject) * kMaxVisibleObjects);

        editor.Execute();
    }

    // Global sampler
    VkSamplerCreateInfo info_nearest =
        vk::BuildSamplerCreateInfo(VK_FILTER_NEAREST);
    vkCreateSampler(rhi->device(), &info_nearest, nullptr, &sampler_nearest);

    VkSamplerCreateInfo info_linear =
        vk::BuildSamplerCreateInfo(VK_FILTER_LINEAR);
    vkCreateSampler(rhi->device(), &info_linear, nullptr, &sampler_linear);

    dtor_queue_resource_.Push([this]() {
        vkDestroySampler(rhi->device(), sampler_nearest, nullptr);
        vkDestroySampler(rhi->device(), sampler_linear, nullptr);
    });

    // Default texture
    vk::Texture2DCreateInfo tex_info{};
    tex_info.width  = 1;
    tex_info.height = 1;
    tex_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    tex_info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    tex_info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    tex_info.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;

    CreateTexture2D("white", &tex_info, &Color4u8::kWhite);
    CreateTexture2D("black", &tex_info, &Color4u8::kBlack);
}

void RenderResource::Finalize() { dtor_queue_resource_.Flush(); }

void RenderResource::PushDestructor(std::function<void()>&& destructor) {
    dtor_queue_resource_.Push(std::move(destructor));
}

void RenderResource::ResetMappedPointers() {
    int idx = rhi->frame_idx();

    per_frame.object = reinterpret_cast<PerFrameBufferObject*>(
        (char*)(per_frame.data) +
        idx * rhi->PaddedSizeOfSSBO<PerFrameBufferObject>());

    per_visible.object = reinterpret_cast<PerVisibleBufferObject*>(
        (char*)(per_visible.data) +
        idx * rhi->PaddedSizeOfSSBO(sizeof(PerVisibleBufferObject) *
                                    kMaxVisibleObjects));
}

uint32_t RenderResource::GetPerFrameDynamicOffset() const {
    uint32_t perframe_offset =
        (uint32_t)rhi->PaddedSizeOfSSBO<PerFrameBufferObject>() *
        rhi->frame_idx();
    return perframe_offset;
}

uint32_t RenderResource::GetPerVisibleDynamicOffset() const {
    uint32_t per_visible_offset =
        (uint32_t)rhi->PaddedSizeOfSSBO(sizeof(PerVisibleBufferObject) *
                                        kMaxVisibleObjects) *
        rhi->frame_idx();
    return per_visible_offset;
}

VkShaderModule RenderResource::GetShaderModule(const std::string& name,
                                               ShaderType         type) {
    auto it = shaders_[type].find(name);
    if (it == shaders_[type].end()) {
        return VK_NULL_HANDLE;
    } else {
        return it->second;
    }
}

vk::Texture2D* RenderResource::GetTexture2D(const std::string& name) {
    auto it = textures_.find(name);
    if (it == textures_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

Material* RenderResource::GetMaterial(const std::string& name) {
    auto it = materials_.find(name);
    if (it == materials_.end()) {
        return nullptr;
    } else {
        return it->second.get();
    }
}

Mesh* RenderResource::GetMesh(const std::string& name) {
    auto it = meshes_.find(name);
    if (it == meshes_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

VkShaderModule RenderResource::CreateShaderModule(const std::string& name,
                                                  ShaderType         type) {
    VkShaderModule res = GetShaderModule(name, type);
    if (res) return res;

    const char* postfix = "";
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

Material* RenderResource::CreateMaterial(const std::string& name,
                                         const std::string& type_name,
                                         VkRenderPass       render_pass) {
    Material* res = GetMaterial(name);
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

Mesh* RenderResource::CreateMeshFromObjFile(const std::string& name,
                                            const fs::path&    filepath) {
    Mesh* res = GetMesh(name);
    if (res) {
        LOG_WARNING("Create mesh with an existed name {}", name);
        return res;
    }

    auto& absolute_path =
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

    auto& mesh = meshes_[name];

    auto vertex_hashmap =
        std::unordered_map<vk::Vertex, Mesh::IndexType, vk::Vertex::Hash>();

    // Loop over shapes
    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
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
            new_vert.position.x = vx;
            new_vert.position.y = vy;
            new_vert.position.z = vz;
            new_vert.normal.x   = nx;
            new_vert.normal.y   = ny;
            new_vert.normal.z   = nz;
            new_vert.texcoord.x = tx;
            new_vert.texcoord.y = 1.0f - ty;
            new_vert.color.r    = r;
            new_vert.color.g    = g;
            new_vert.color.b    = b;

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

vk::Texture2D* RenderResource::CreateTexture2DFromFile(
    const std::string& name, const fs::path& filepath) {

    vk::Texture2D* res = GetTexture2D(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    int   texWidth, texHeight, texChannels;
    auto& absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    stbi_uc* pixels = stbi_load(absolute_path.string().c_str(), &texWidth,
                                &texHeight, &texChannels, STBI_default);
    if (!pixels) {
        LOG_ERROR("Failed to load texture file {}", filepath);
        return nullptr;
    }

    VkFormat           format = VK_FORMAT_UNDEFINED;
    VkImageAspectFlags aspect = VK_IMAGE_ASPECT_NONE;
    switch (texChannels) {
        case STBI_rgb:
            format = VK_FORMAT_R8G8B8_SRGB;
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case STBI_rgb_alpha:
            format = VK_FORMAT_R8G8B8A8_SRGB;
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case STBI_grey:
            format = VK_FORMAT_R8_UNORM;
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        default:
            LOG_ERROR("Unknown image format when loading {}", filepath);
    }

    vk::Texture2DCreateInfo info{};
    info.width  = texWidth;
    info.height = texHeight;
    info.format = format;
    info.image_usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.memory_usage      = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags      = aspect;
    vk::Texture2D* texture = CreateTexture2D(name, &info, pixels);

    stbi_image_free(pixels);
    return texture;
}

vk::Texture2D* RenderResource::CreateTexture2D(
    const std::string&       name,  //
    vk::Texture2DCreateInfo* info,  //
    const void*              pixels) {

    vk::Texture2D* res = GetTexture2D(name);
    if (res) {
        LOG_WARNING("Create texture with an existed name {}", name);
        return res;
    }

    vk::Texture2D* texture = &textures_[name];
    rhi->AllocateTexture2D(texture, info);
    UploadTexture2D(texture, pixels);

    dtor_queue_resource_.Push(
        [this, texture]() { rhi->DestroyTexture2D(texture); });
    return texture;
}

bool RenderResource::LoadVkShaderModule(const std::string& filepath,
                                        VkShaderModule*    p_shader_module) {
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
    createInfo.pCode    = (uint32_t*)buffer.data();

    if (vkCreateShaderModule(rhi->device(), &createInfo, nullptr,
                             p_shader_module) != VK_SUCCESS) {
        return false;
    }
    return true;
}

void RenderResource::UploadMesh(Mesh* mesh) {
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

void RenderResource::UploadTexture2D(vk::Texture2D* texture,
                                     const void*    pixels) {
    VkDeviceSize channels = 0;
    switch (texture->format) {
        case VK_FORMAT_R8G8B8_SRGB:
            channels = 3;
            break;
        case VK_FORMAT_R8G8B8A8_SRGB:
            channels = 4;
            break;
        case VK_FORMAT_R8_UNORM:
            channels = 1;
            break;
        default:
            LOG_ERROR("Unknown texture format {}", texture->format);
            break;
    }
    VkDeviceSize image_size = channels * texture->width * texture->height;

    // allocate temporary buffer for holding texture data to upload
    vk::AllocatedBuffer staging_buffer =
        rhi->AllocateBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            VMA_MEMORY_USAGE_CPU_ONLY);
    ScopeGuard guard = [this, &staging_buffer]() {
        rhi->DestroyBuffer(&staging_buffer);
    };

    // data -> staging buffer
    rhi->CopyBuffer(pixels, &staging_buffer, image_size);

    // staging buffer -> image
    rhi->ImmediateSubmit([texture, &staging_buffer](VkCommandBuffer cmd) {
        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer{};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image            = texture->image.image;
        imageBarrier_toTransfer.subresourceRange = range;
        imageBarrier_toTransfer.srcAccessMask    = 0;
        imageBarrier_toTransfer.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &imageBarrier_toTransfer);

        VkExtent3D imageExtent{};
        imageExtent.width  = (uint32_t)texture->width;
        imageExtent.height = (uint32_t)texture->height;
        imageExtent.depth  = 1;

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = imageExtent;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, staging_buffer.buffer, texture->image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        // barrier the image into the shader readable layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
    });
}

}  // namespace lumi