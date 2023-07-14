#pragma once

#include "function/render/pipeline/subpass/imgui_subpass.h"
#include "function/render/pipeline/subpass/mesh_lighting_subpass.h"
#include "function/render/pipeline/subpass/skybox_subpass.h"
#include "function/render/render_resource.h"
#include "render_pass.h"

namespace lumi {

class PresentPass : public RenderPass {
private:
    enum AttachmentIndex {
        kAttachmentSwapchain = 0,
        kAttachmentDepth,

        kAttachmentCount
    };

    std::shared_ptr<vk::Texture> depth_attachment_{};

    enum SubpassIndex {
        kSubpassMeshLighting = 0,
        kSubpassSkybox,
        kSubpassImGui,

        kSubpassCount
    };

    std::shared_ptr<MeshLightingSubpass> mesh_lighting_pass_{};
    std::shared_ptr<SkyboxSubpass>       skybox_pass_{};
    std::shared_ptr<ImGuiSubpass>        imgui_pass_{};

    std::vector<VkFramebuffer> framebuffers_{};
    vk::DestructorQueue        dtor_queue_swapchain_{};
    vk::DestructorQueue        dtor_queue_present_{};

public:
    using RenderPass::RenderPass;

    virtual void CmdRender(VkCommandBuffer cmd) {
        mesh_lighting_pass_->CmdRender(cmd);

        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        skybox_pass_->CmdRender(cmd);

        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        imgui_pass_->CmdRender(cmd);
    }

    virtual void Finalize() override;

    virtual VkExtent2D GetExtent() override;

    void RecreateSwapchain();

protected:
    virtual void PreInit() override;

    virtual void PostInit() override;

    virtual void CreateAttachmentImages() override;

    virtual void CreateRenderPass() override;

    virtual void CreateFrameBuffers() override;

    virtual void SetClearValues() override;

    virtual VkFramebuffer GetFramebuffer() override;
};

}  // namespace lumi