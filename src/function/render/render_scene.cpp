#include "render_scene.h"

#include "core/scope_guard.h"
#include "material/pbr_material.h"

namespace lumi {

void RenderScene::LoadScene() {
    // TODO: load from json file

    if (!resource->CreateMeshFromObjFile("monkey",
                                         "models/monkey_smooth.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    if (!resource->CreateMeshFromObjFile(
            "empire", "scenes/lost_empire/lost_empire.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    // Load textures
    if (!resource->CreateTexture2DFromFile(
            "empire_diffuse", "scenes/lost_empire/lost_empire-RGBA.png")) {
        LOG_ERROR("Loading .png file failed");
    }

    // Create materials
    // TODO: load from json file
    {
        auto material =
            (PBRMaterial*)resource->CreateMaterial("empire", "PBRMaterial");
        material->diffuse_tex_name = "empire_diffuse";
        material->Upload(resource.get());
    }

    {
        auto material =
            (PBRMaterial*)resource->CreateMaterial("monkey", "PBRMaterial");
        material->Upload(resource.get());
    }

    RenderObject& empire = renderables.emplace_back();
    empire.mesh_name     = "empire";
    empire.material_name = "empire";
    empire.position      = {5, -10, 0};

    int cnt = 3;
    for (int x = -cnt; x <= cnt; x++) {
        for (int y = -cnt; y <= cnt; y++) {
            RenderObject& monkey = renderables.emplace_back();
            monkey.mesh_name     = "monkey";
            monkey.material_name = "monkey";
            monkey.rotation = Quaternion::Rotation(Vec3f(0, 0, ToRadians(0))) *
                              monkey.rotation;
            monkey.position = Vec3f(x, 0, y) * 5;
        }
    }

}

void RenderScene::UpdateVisibleObjects() {
    resource->visible_object_descs.clear();

    // TODO: culling
    for (auto& renderable : renderables) {
        auto& desc    = resource->visible_object_descs.emplace_back();
        desc.mesh     = resource->GetMesh(renderable.mesh_name);
        desc.material = resource->GetMaterial(renderable.material_name);
        desc.object   = &renderable;
    }
}

void RenderScene::UploadPerFrameResource() {
    auto per_frame_buffer_object = resource->per_frame.object;

    const Mat4x4f& view                    = camera.view();
    const Mat4x4f& proj                    = camera.projection();
    per_frame_buffer_object->view          = view;
    per_frame_buffer_object->proj          = proj;
    per_frame_buffer_object->proj_view     = proj * view;
    per_frame_buffer_object->ambient_color = Color4f::kWhite;

    rhi->CopyBuffer(&resource->per_frame.staging_buffer,
                    &resource->per_frame.buffer, sizeof(PerFrameBufferObject),
                    resource->GetPerFrameDynamicOffset());
}

}  // namespace lumi