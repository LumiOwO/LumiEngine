#include "render_scene.h"

#include "core/scope_guard.h"
#include "material/pbr_material.h"
#include "material/unlit_material.h"

#include "function/cvars/cvar_system.h"

namespace lumi {

void RenderScene::LoadScene() {
    // TODO: load from json file

    if (!resource->CreateMeshFromObjFile("monkey",
                                         "models/monkey_smooth.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

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

    // skybox
    resource->CreateTextureCubemapFromFile("skybox_irradiance",
                                           "textures/skybox/skybox_irradiance");
    resource->CreateTextureCubemapFromFile("skybox_specular",
                                           "textures/skybox/skybox_specular");
    
    resource->global.skybox_material->irradiance_cubemap_name =
        "skybox_irradiance";
    resource->global.skybox_material->specular_cubemap_name =
        "skybox_specular";
    resource->UpdateGlobalDescriptorSet();

    // mesh, textures, materials
    resource->LoadFromGLTFFile("scenes/DamagedHelmet/DamagedHelmet.gltf");

    //auto material =
    //    (PBRMaterial *)resource->GetMaterial("DamagedHelmet_mat_0");
    //(*material->params.data) = {};
    //material->Upload(resource.get());

    auto unlit =
        (UnlitMaterial *)resource->CreateMaterial("unlit", "UnlitMaterial");
    unlit->base_color_tex_name = "DamagedHelmet_tex_0";
    unlit->Upload(resource.get());

    resource->CreateMaterial("default", "PBRMaterial");

    // render objects (Scene nodes)
    // TODO: tree structured
    RenderObject &helmet = renderables.emplace_back();
    helmet.mesh_name     = "DamagedHelmet";
    helmet.material_name = "DamagedHelmet_mat_0";
    helmet.rotation = Quaternion(ToRadians(Vec3f(90, 180, 0)));
    //helmet.material_name = "unlit";

    RenderObject &monkey = renderables.emplace_back();
    monkey.mesh_name     = "monkey";
    monkey.material_name = "default";
    monkey.position      = {0, -2, 0};
    monkey.rotation      = Quaternion(ToRadians(Vec3f(0, 0, 0)));

    camera.position = {1.5f, 0, -1.5f};
    camera.eulers_deg = Vec3f(0, -45, 0);

    // TODO: camera control in scene node
}

void RenderScene::UpdateVisibleObjects() {
    auto &visible_batchs = resource->visibles_drawcall_batchs;
    visible_batchs.clear();

    // TODO: culling
    for (auto &renderable : renderables) {
        // synchronize object_to_world matrix
        renderable.object_to_world =
            Mat4x4f::Translation(renderable.position) *  //
            Mat4x4f(renderable.rotation) *               //
            Mat4x4f::Scale(renderable.scale);

        Material *material = resource->GetMaterial(renderable.material_name);
        Mesh     *mesh     = resource->GetMesh(renderable.mesh_name);
        auto     &batch    = visible_batchs[material][mesh];

        auto &desc    = batch.emplace_back();
        desc.material = material;
        desc.mesh     = mesh;
        desc.object   = &renderable;
    }

}

void RenderScene::UploadGlobalResource() {
    // --- Global resource ---
    // Update camera data staging buffer
    auto cam_data = resource->global.data.cam;

    const Mat4x4f &view     = camera.view();
    const Mat4x4f &proj     = camera.projection();
    cam_data->view          = view;
    cam_data->proj          = proj;
    cam_data->proj_view     = proj * view;
    cam_data->cam_pos       = camera.position;

    Vec3f sunlight_dir =
        cvars::GetVec3f("env.sunlight.dir").value().Normalize();
    Mat4x4f sunlight_world_to_clip =
        GetSunlightWorldToClip(camera, sunlight_dir);

    // Update environment data staging buffer
    auto env_data = resource->global.data.env;

    env_data->sunlight_color = cvars::GetVec3f("env.sunlight.color").value();
    env_data->sunlight_intensity =
        cvars::GetFloat("env.sunlight.intensity").value();
    env_data->sunlight_dir  = sunlight_dir;
    env_data->ibl_intensity = cvars::GetFloat("env.IBL.intensity").value();
    env_data->mip_levels =
        resource
            ->GetTexture(
                resource->global.skybox_material->irradiance_cubemap_name)
            ->mip_levels;
    env_data->debug_idx              = cvars::GetInt("debug.shading").value();
    env_data->sunlight_world_to_clip = sunlight_world_to_clip;

    // Upload global data to GPU
    size_t cam_size = rhi->PaddedSizeOfSSBO<CamDataSSBO>();
    size_t env_size = rhi->PaddedSizeOfSSBO<EnvDataSSBO>();
    rhi->CopyBuffer(&resource->global.staging_buffer, &resource->global.buffer,
                    cam_size + env_size,
                    resource->GlobalSSBODynamicOffsets()[0]);

    // --- Mesh instance resource ---
    // Update staging buffer
    size_t visibles_cnt = 0;
    auto   cur_instance = resource->mesh_instances.data.cur_instance;
    for (auto &[mat, mat_batch] : resource->visibles_drawcall_batchs) {
        for (auto &[mesh, batch] : mat_batch) {
            // Write to staging buffer
            for (auto &desc : batch) {
                RenderObject *object          = desc.object;
                cur_instance->object_to_world = object->object_to_world;
                cur_instance->world_to_object =
                    Mat4x4f::Scale(1.0f / object->scale) *  //
                    Mat4x4f(object->rotation.Inverse()) *   //
                    Mat4x4f::Translation(-object->position);
                cur_instance++;
                visibles_cnt++;
            }
        }
    }

    // Upload mesh instance data to GPU
    rhi->CopyBuffer(&resource->mesh_instances.staging_buffer,
                    &resource->mesh_instances.buffer,
                    sizeof(MeshInstanceSSBO) * visibles_cnt,
                    resource->MeshInstanceSSBODynamicOffsets()[0]);
}

Mat4x4f RenderScene::GetSunlightWorldToClip(const Camera &camera,
                                            const Vec3f  &sunlight_dir) {
    // Compute camera frustum bounding box
    BoundingBox frustum_bbox{};

    constexpr int   kCorners              = 8;
    constexpr Vec3f ndc_corners[kCorners] = {
        Vec3f(-1.0f, -1.0f, 1.0f), Vec3f(1.0f, -1.0f, 1.0f),
        Vec3f(1.0f, 1.0f, 1.0f),   Vec3f(-1.0f, 1.0f, 1.0f),
        Vec3f(-1.0f, -1.0f, 0.0f), Vec3f(1.0f, -1.0f, 0.0f),
        Vec3f(1.0f, 1.0f, 0.0f),   Vec3f(-1.0f, 1.0f, 0.0f),
    };

    Mat4x4f clip_to_camera = Mat4x4f::PerspectiveInverse(
        ToRadians(camera.fovy_deg), camera.aspect, camera.near, camera.far);
    Mat4x4f camera_to_world =
        Mat4x4f::Translation(camera.position) * camera.rotation();
    Mat4x4f clip_to_world = camera_to_world * clip_to_camera;

    for (int i = 0; i < kCorners; i++) {
        Vec4f frustum_point_with_w =
            clip_to_world * Vec4f(ndc_corners[i], 1.0f);
        Vec3f frustum_point =
            Vec3f(frustum_point_with_w) / frustum_point_with_w.w;

        frustum_bbox.Merge(frustum_point);
    }

    // Compute scene bounding box
    BoundingBox scene_bbox{};
    for (auto &object : renderables) {
        BoundingBox mesh_bbox_world =
            object.object_to_world * resource->GetMesh(object.mesh_name)->bbox;
        scene_bbox.Merge(mesh_bbox_world);
    }

    // world -> light view
    Vec3f eye =
        frustum_bbox.center() - sunlight_dir * frustum_bbox.extent().Length();
    Vec3f   center             = frustum_bbox.center();
    Vec3f   up                 = Vec3f::kUnitY;
    // Avoid nan
    if (Cross(center - eye, up).LengthSquare() < 0.001f) {
        up = Vec3f::kUnitZ;
    }
    Mat4x4f world_to_lightview = Mat4x4f::LookAt(eye, center, up);

    // light view -> clip
    BoundingBox frustum_bbox_lightview = world_to_lightview * frustum_bbox;
    BoundingBox scene_bbox_lightview   = world_to_lightview * scene_bbox;

    float left =
        std::max(frustum_bbox_lightview.min().x, scene_bbox_lightview.min().x);
    float right =
        std::min(frustum_bbox_lightview.max().x, scene_bbox_lightview.max().x);
    float bottom =
        std::max(frustum_bbox_lightview.min().y, scene_bbox_lightview.min().y);
    float top =
        std::min(frustum_bbox_lightview.max().y, scene_bbox_lightview.max().y);
    // the objects which are nearer than the frustum bounding box may caster shadow as well
    float near = scene_bbox_lightview.min().z;
    float far =
        std::min(frustum_bbox_lightview.max().z, scene_bbox_lightview.max().z);

    Mat4x4f lightview_to_clip =
        Mat4x4f::Orthographic(left, right, bottom, top, near, far);

    return lightview_to_clip * world_to_lightview;
}

}  // namespace lumi