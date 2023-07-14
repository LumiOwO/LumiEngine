#pragma once

#include "function/render/pipeline/subpass/directional_shadow_subpass.h"
#include "function/render/render_resource.h"
#include "render_pass.h"

namespace lumi {

class ShadowPass : public RenderPass {
private:
    enum AttachmentIndex {
        kAttachmentDirectional = 0,
        //kAttachmentPoint,

        kAttachmentCount
    };

    struct {
        std::shared_ptr<vk::Texture> directional_shadow_map_{};
    } attachments_;


    enum SubpassIndex {
        kSubpassDirectional = 0,
        //kSubpassPoint,

        kSubpassCount
    };

    constexpr static uint32_t kShadowMapSize = 2048;

    std::shared_ptr<DirectionalShadowSubpass> directional_shadow_pass_{};

    VkFramebuffer framebuffer_{};

    vk::DestructorQueue dtor_queue_shadow_{};

public:
    constexpr static const char* kDirectionalShadowMapName =
        "_shadow_map_directional";

    using RenderPass::RenderPass;

    virtual void CmdRender(VkCommandBuffer cmd) {
        directional_shadow_pass_->CmdRender(cmd);
    }

    virtual void Finalize() override;

    virtual VkExtent2D GetExtent() override;

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