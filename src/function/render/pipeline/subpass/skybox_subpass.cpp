#include "skybox_subpass.h"

#include "function/render/render_resource.h"
#include "function/render/pipeline/pass/render_pass.h"
#include "function/render/material/skybox_material.h"

#include "function/cvars/cvar_system.h"

namespace lumi {

void SkyboxSubpass::Init(uint32_t subpass_idx) {
    auto resource = render_pass_->resource;

    resource->global.skybox_material =
        (SkyboxMaterial*)resource->CreateMaterial(
            "_skybox", "SkyboxMaterial", render_pass_->vk_render_pass(),
            subpass_idx);
}

void SkyboxSubpass::CmdRender(VkCommandBuffer cmd) {
    auto material = render_pass_->resource->global.skybox_material;
    CmdBindMaterial(cmd, material);

    vkCmdPushConstants(cmd, material->pipeline_layout,
                       VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int),
                       cvars::GetInt("env.skybox").ptr());

    // 2 triangles (6 vertex) each face, 6 faces
    vkCmdDraw(cmd, 36, 1, 0, 0);
}

}  // namespace lumi