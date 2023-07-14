#pragma once

#include "pass/present_pass.h"
#include "pass/shadow_pass.h"
#include "render_pipeline.h"

namespace lumi {

class ForwardPipeline : public RenderPipeline {
private:
    std::shared_ptr<ShadowPass>  shadow_pass_{};
    std::shared_ptr<PresentPass> present_pass_{};

public:
    using RenderPipeline::RenderPipeline;

    virtual void Init() override {
        shadow_pass_ = std::make_shared<ShadowPass>(rhi, resource);
        shadow_pass_->Init();

        present_pass_ = std::make_shared<PresentPass>(rhi, resource);
        present_pass_->Init();
    }

    virtual void Finalize() override { 
        shadow_pass_->Finalize(); 

        present_pass_->Finalize(); 
    }

    virtual void CmdRender(VkCommandBuffer cmd) override {
        shadow_pass_->CmdBeginRenderPass(cmd);
        shadow_pass_->CmdRender(cmd);
        shadow_pass_->CmdEndRenderPass(cmd);

        present_pass_->CmdBeginRenderPass(cmd);
        present_pass_->CmdRender(cmd);
        present_pass_->CmdEndRenderPass(cmd);
    }

    virtual void RecreateSwapchain() override {
        present_pass_->RecreateSwapchain();
    }
};

}  // namespace lumi