#pragma once

#include "function/render/rhi/vulkan_rhi.h"

namespace lumi {

class RenderPass;

class RenderSubpass {
protected:
    RenderPass* render_pass_{};

public:
    RenderSubpass(RenderPass* render_pass) : render_pass_(render_pass) {}

    virtual void CmdRender(VkCommandBuffer cmd) = 0;
};

}  // namespace lumi