#pragma once

#include "function/render/rhi/vulkan_rhi.h"

namespace lumi {

class RenderPass;
struct Material;

class RenderSubpass {
protected:
    RenderPass* render_pass_{};

public:
    RenderSubpass(RenderPass* render_pass) : render_pass_(render_pass) {}

    virtual void CmdRender(VkCommandBuffer cmd) = 0;

    virtual void Init(uint32_t subpass_idx) = 0;

protected:
    void CmdBindMaterial(VkCommandBuffer cmd, Material* material);
};

}  // namespace lumi