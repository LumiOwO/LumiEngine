#pragma once

#include "pass/render_pass.h"
#include "function/render/render_resource.h"

namespace lumi {

class RenderPipeline {
public:
    std::shared_ptr<VulkanRHI>      rhi{};
    std::shared_ptr<RenderResource> resource{};

public:
    RenderPipeline(std::shared_ptr<VulkanRHI>      rhi,
                   std::shared_ptr<RenderResource> resource)
        : rhi(rhi), resource(resource) {}

    virtual void Init() = 0;

    virtual void Finalize() = 0;

    virtual void CmdRender(VkCommandBuffer cmd) = 0;

    virtual void RecreateSwapchain() = 0;

    void Render() {
        bool success = rhi->BeginRenderCommand();
        if (!success) {
            RecreateSwapchain();
            return;
        }

        VkCommandBuffer cmd = rhi->GetCurrentCommandBuffer();
        CmdRender(cmd);

        success = rhi->EndRenderCommand();
        if (!success) {
            RecreateSwapchain();
            return;
        }
    }
};

}  // namespace lumi