#include "render_scene.h"

#include "core/scope_guard.h"
#include "material/pbr_material.h"
#include "material/unlit_material.h"

#include "function/cvars/cvar_system.h"

namespace lumi {

void RenderScene::LoadScene() {
    // TODO: load from json file

    //if (!resource->CreateMeshFromObjFile("monkey",
    //                                     "models/monkey_smooth.obj")) {
    //    LOG_ERROR("Loading .obj file failed");
    //}

    //if (!resource->CreateMeshFromObjFile(
    //        "empire", "scenes/lost_empire/lost_empire.obj")) {
    //    LOG_ERROR("Loading .obj file failed");
    //}

    //// Load textures
    //if (!resource->CreateTexture2DFromFile(
    //        "empire_diffuse", "scenes/lost_empire/lost_empire-RGBA.png")) {
    //    LOG_ERROR("Loading .png file failed");
    //}

    //// Create materials
    //{
    //    auto material = (UnlitMaterial *)resource->CreateMaterial(
    //        "empire", "UnlitMaterial");
    //    material->base_color_tex_name = "empire_diffuse";
    //    material->Upload(resource.get());
    //}

    //RenderObject &empire = renderables.emplace_back();
    //empire.mesh_name     = "empire";
    //empire.material_name = "empire";
    //empire.position      = {5, -10, 0};

    //int cnt = 3;
    //for (int x = -cnt; x <= cnt; x++) {
    //    for (int y = -cnt; y <= cnt; y++) {
    //        RenderObject& monkey = renderables.emplace_back();
    //        monkey.mesh_name     = "monkey";
    //        monkey.material_name = "monkey";
    //        monkey.rotation = Quaternion::Rotation(Vec3f(0, 0, ToRadians(0))) *
    //                          monkey.rotation;
    //        monkey.position = Vec3f(x, 0, y) * 5;
    //    }
    //}

    resource->LoadFromGLTFFile("scenes/DamagedHelmet/DamagedHelmet.gltf");

    //auto material =
    //    (PBRMaterial *)resource->GetMaterial("DamagedHelmet_mat_0");
    //(*material->params.data) = {};
    //material->Upload(resource.get());

    auto unlit =
        (UnlitMaterial *)resource->CreateMaterial("unlit", "UnlitMaterial");
    unlit->base_color_tex_name = "DamagedHelmet_tex_0";
    unlit->Upload(resource.get());

    RenderObject &helmet = renderables.emplace_back();
    helmet.mesh_name     = "DamagedHelmet";
    helmet.material_name = "DamagedHelmet_mat_0";
    //helmet.material_name = "unlit";
    camera.position = {0, 0, -3};
}

void RenderScene::UpdateVisibleObjects() {
    resource->visible_object_descs.clear();

    // TODO: culling
    for (auto &renderable : renderables) {
        auto &desc    = resource->visible_object_descs.emplace_back();
        desc.mesh     = resource->GetMesh(renderable.mesh_name);
        desc.material = resource->GetMaterial(renderable.material_name);
        desc.object   = &renderable;
    }
}

void RenderScene::UploadGlobalResource() {
    auto cam_data = resource->global.data.cam;

    const Mat4x4f &view     = camera.view();
    const Mat4x4f &proj     = camera.projection();
    cam_data->view          = view;
    cam_data->proj          = proj;
    cam_data->proj_view     = proj * view;
    cam_data->cam_pos       = camera.position;

    auto env_data = resource->global.data.env;

    env_data->sunlight_color =
        Vec4f(cvars::GetVec3f("env.sunlight_color").value(), 1.0f);
    env_data->sunlight_dir =
        cvars::GetVec3f("env.sunlight_dir").value().Normalize();
    env_data->debug_idx = cvars::GetInt("debug.pbr_idx").value();

    size_t cam_size = rhi->PaddedSizeOfSSBO<CamDataSSBO>();
    size_t env_size = rhi->PaddedSizeOfSSBO<EnvDataSSBO>();
    rhi->CopyBuffer(&resource->global.staging_buffer, &resource->global.buffer,
                    cam_size + env_size,
                    resource->GlobalSSBODynamicOffsets()[0]);
}

}  // namespace lumi