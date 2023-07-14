#pragma once

#include "function/render/pipeline/subpass/render_subpass.h"

namespace lumi {

class VulkanRHI;
class RenderResource;

class RenderPass {
public:
    std::shared_ptr<VulkanRHI> rhi{};
    std::shared_ptr<RenderResource> resource{};

protected:
    std::vector<VkClearValue>  clear_values_{};
    VkRenderPass               vk_render_pass_{};

public:
    RenderPass(std::shared_ptr<VulkanRHI>      rhi,
               std::shared_ptr<RenderResource> resource)
        : rhi(rhi), resource(resource) {}

    VkRenderPass vk_render_pass() const { return vk_render_pass_; }

    void Init() {
        PreInit();
        SetClearValues();

        CreateAttachmentImages();
        CreateRenderPass();
        CreateFrameBuffers();

        PostInit();
    }

    void CmdBeginRenderPass(VkCommandBuffer cmd) {
        VkRenderPassBeginInfo info{};
        info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        info.pNext               = nullptr;
        info.renderPass          = vk_render_pass_;
        info.renderArea.offset.x = 0;
        info.renderArea.offset.y = 0;
        info.renderArea.extent   = GetExtent();
        info.framebuffer         = GetFramebuffer();
        info.clearValueCount     = (uint32_t)clear_values_.size();
        info.pClearValues        = clear_values_.data();
        vkCmdBeginRenderPass(cmd, &info, VK_SUBPASS_CONTENTS_INLINE);
    }

    void CmdEndRenderPass(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
    }

    virtual void CmdRender(VkCommandBuffer cmd) = 0;

    virtual void Finalize() = 0;

    virtual VkExtent2D GetExtent() = 0;

protected:
    virtual void PreInit() = 0;

    virtual void PostInit() = 0;

    virtual void CreateAttachmentImages() = 0;

    virtual void CreateRenderPass() = 0;

    virtual void CreateFrameBuffers() = 0;

    virtual void SetClearValues() = 0;

    virtual VkFramebuffer GetFramebuffer() = 0;
};

}  // namespace lumi