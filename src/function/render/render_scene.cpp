#include "render_scene.h"

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

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

    // TODO: load shapes to index buffer
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            // hardcode loading to triangles
            int fv = 3;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

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

                // copy it into our vertex
                vk::Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                // we are setting the vertex color as the vertex normal.
                // This is just for display purposes
                new_vert.color = new_vert.normal;

                mesh.vertices.emplace_back(new_vert);
            }
            index_offset += fv;
        }
    }

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
    // Create meshes
    vk::Mesh triangle_mesh{};
    triangle_mesh.vertices.resize(3);

    triangle_mesh.vertices[0].position = {1.f, 1.f, 0.5f};
    triangle_mesh.vertices[1].position = {-1.f, 1.f, 0.5f};
    triangle_mesh.vertices[2].position = {0.f, -1.f, 0.5f};

    triangle_mesh.vertices[0].color = {0.f, 1.f, 0.0f};  //pure green
    triangle_mesh.vertices[1].color = {0.f, 1.f, 0.0f};  //pure green
    triangle_mesh.vertices[2].color = {0.f, 1.f, 0.0f};  //pure green

    vk::Mesh monkey_mesh{};
    if (!LoadMeshFromObjFile(monkey_mesh, "models/monkey_smooth.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    rhi_->UploadMesh(triangle_mesh);
    rhi_->UploadMesh(monkey_mesh);

    // note that we are copying them. 
    // Eventually we will delete the hardcoded _monkey and _triangle meshes, 
    // so it's no problem now.
    meshes_["triangle"] = triangle_mesh;
    meshes_["monkey"]   = monkey_mesh;

    // Create materials
    materials_["meshtriangle"] =
        std::move(rhi_->CreateMaterial("meshtriangle"));

    // Create renderables
    vk::RenderObject monkey{};
    monkey.mesh     = GetMesh("monkey");
    monkey.material = GetMaterial("meshtriangle");
    renderables.emplace_back(monkey);

    for (int x = -20; x <= 20; x++) {
        for (int y = -20; y <= 20; y++) {
            vk::RenderObject tri{};
            tri.mesh     = GetMesh("triangle");
            tri.material = GetMaterial("meshtriangle");
            tri.position = Vec3f(x, 0, y);
            tri.scale    = Vec3f(0.2, 0.2, 0.2);
            renderables.emplace_back(tri);
        }
    }
}

}  // namespace lumi