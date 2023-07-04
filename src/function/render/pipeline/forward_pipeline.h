#pragma once

#include "render_pipeline.h"
#include "pass/present_pass.h"

namespace lumi {

class ForwardPipeline : public RenderPipeline {
private:
    std::shared_ptr<PresentPass> present_pass_{};

public:
    using RenderPipeline::RenderPipeline;

    virtual void Init() override {
        // TODO: add shadow map pass

        present_pass_ = std::make_shared<PresentPass>(rhi, resource);
        present_pass_->Init();
    }

    virtual void Finalize() override {
        present_pass_->Finalize();
    }

    virtual void CmdRender(VkCommandBuffer cmd) override {
        present_pass_->CmdBeginRenderPass(cmd);
        present_pass_->CmdRender(cmd);
        present_pass_->CmdEndRenderPass(cmd);
    }

    virtual void RecreateSwapchain() override {
        present_pass_->RecreateSwapchain();
    }
};

}  // namespace lumi