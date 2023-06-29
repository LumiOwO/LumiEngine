#include "render_scene.h"
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

bool RenderScene::LoadMeshFromObjFile(vk::Mesh&       mesh,
                                      const fs::path& filepath) {
    auto& absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto in = std::ifstream(absolute_path);

    // attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib;
    // shapes contains the info for each separate object in the file
    std::vector<tinyobj::shape_t> shapes;
    // materials contains the information about the material of each shape,
    // but we won't use it.
    std::vector<tinyobj::material_t> materials;

    //error and warning output from the load function
    std::string warn;
    std::string err;

    // load the OBJ file
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                     absolute_path.string().c_str(),
                     absolute_path.parent_path().string().c_str());
    // make sure to output the warnings to the console,
    // in case there are issues with the file
    if (!warn.empty()) {
        LOG_WARNING(warn.c_str());
    }
    // if we have any error, print it to the console, and break the mesh loading.
    // This happens if the file can't be found or is malformed
    if (!err.empty()) {
        LOG_ERROR(err.c_str());
        return false;
    }

    auto vertex_hashmap =
        std::unordered_map<vk::Vertex, vk::Mesh::IndexType, vk::Vertex::Hash>();

    // Loop over shapes
    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            // vertex position
            tinyobj::real_t vx =
                attrib.vertices[3LL * idx.vertex_index + 0];
            tinyobj::real_t vy =
                attrib.vertices[3LL * idx.vertex_index + 1];
            tinyobj::real_t vz =
                attrib.vertices[3LL * idx.vertex_index + 2];
            // vertex normal
            tinyobj::real_t nx = attrib.normals[3LL * idx.normal_index + 0];
            tinyobj::real_t ny = attrib.normals[3LL * idx.normal_index + 1];
            tinyobj::real_t nz = attrib.normals[3LL * idx.normal_index + 2];
            // vertex texcoord
            tinyobj::real_t tx =
                attrib.texcoords[2LL * idx.texcoord_index + 0];
            tinyobj::real_t ty =
                attrib.texcoords[2LL * idx.texcoord_index + 1];
            // Optional: vertex colors
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
                    (vk::Mesh::IndexType)mesh.vertices.size();
                mesh.vertices.emplace_back(new_vert);
            }

            mesh.indices.emplace_back(vertex_hashmap[new_vert]);
        }
    }

    rhi_->UploadMesh(mesh);

    return true;
}

bool RenderScene::LoadTextureFromFile(vk::Texture&    texture,
                                      const fs::path& filepath,
                                      TextureFormat   format) {
    int stb_format = STBI_default;
    switch (format) {
        case TextureFormat::kRGB:
            stb_format     = STBI_rgb;
            texture.format = VK_FORMAT_R8G8B8_SRGB;
            break;
        case TextureFormat::kRGBA:
            stb_format     = STBI_rgb_alpha;
            texture.format = VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case TextureFormat::kGray:
            stb_format     = STBI_grey;
            texture.format = VK_FORMAT_R8_UNORM;
            break;
    }
    auto& absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;

    int      texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(absolute_path.string().c_str(), &texWidth,
                                &texHeight, &texChannels, stb_format);
    if (!pixels) {
        LOG_ERROR("Failed to load texture file {}", filepath);
        return false;
    }

    rhi_->UploadTexture(texture, pixels, texWidth, texHeight, texChannels);

    stbi_image_free(pixels);
    return true;
}


vk::Material* RenderScene::GetMaterial(const std::string& name) {
    //search for the object, and return nullptr if not found
    auto it = materials_.find(name);
    if (it == materials_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

vk::Mesh* RenderScene::GetMesh(const std::string& name) {
    auto it = meshes_.find(name);
    if (it == meshes_.end()) {
        return nullptr;
    } else {
        return &it->second;
    }
}

void RenderScene::LoadScene() {
    // Load meshes
    vk::Mesh triangle_mesh{};
    triangle_mesh.vertices.resize(3);

    triangle_mesh.vertices[0].position = {1.f, 1.f, 0.5f};
    triangle_mesh.vertices[1].position = {-1.f, 1.f, 0.5f};
    triangle_mesh.vertices[2].position = {0.f, -1.f, 0.5f};

    triangle_mesh.vertices[0].color = {0.f, 1.f, 0.0f};  // pure green
    triangle_mesh.vertices[1].color = {0.f, 1.f, 0.0f};  // pure green
    triangle_mesh.vertices[2].color = {0.f, 1.f, 0.0f};  // pure green

    triangle_mesh.indices = {0, 1, 2};
    rhi_->UploadMesh(triangle_mesh);
    meshes_["triangle"] = triangle_mesh;

    if (!LoadMeshFromObjFile(meshes_["monkey"], "models/monkey_smooth.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    if (!LoadMeshFromObjFile(meshes_["empire"],
                             "scenes/lost_empire/lost_empire.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    // Load textures
    if (!LoadTextureFromFile(textures_["empire_diffuse"],
                             "scenes/lost_empire/lost_empire-RGBA.png",
                             TextureFormat::kRGBA)) {
        LOG_ERROR("Loading .png file failed");
    }

    // Create materials
    materials_["meshtriangle"] = std::move(
        rhi_->CreateMaterial("meshtriangle", textures_["empire_diffuse"].image_view));

    // Create renderables
    //vk::RenderObject monkey{};
    //monkey.mesh     = GetMesh("monkey");
    //monkey.material = GetMaterial("meshtriangle");
    //renderables.emplace_back(monkey);

    //for (int x = -20; x <= 20; x++) {
    //    for (int y = -20; y <= 20; y++) {
    //        vk::RenderObject tri{};
    //        tri.mesh     = GetMesh("triangle");
    //        tri.material = GetMaterial("meshtriangle");
    //        tri.position = Vec3f(x, 0, y);
    //        tri.scale    = Vec3f(0.2, 0.2, 0.2);
    //        renderables.emplace_back(tri);
    //    }
    //}

    vk::RenderObject& empire = renderables.emplace_back();
    empire.mesh              = GetMesh("empire");
    empire.material          = GetMaterial("meshtriangle");
    empire.position          = {5, -10, 0};
}

}  // namespace lumi