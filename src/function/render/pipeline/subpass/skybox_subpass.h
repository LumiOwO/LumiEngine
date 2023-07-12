#pragma once

#include "render_subpass.h"

namespace lumi {

class SkyboxSubpass : public RenderSubpass {
public:
    using RenderSubpass::RenderSubpass;

    virtual void Init(uint32_t subpass_idx) override;

    virtual void CmdRender(VkCommandBuffer cmd) override;
};

}  // namespace lumi