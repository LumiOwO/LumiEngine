#pragma once

#include "render_subpass.h"

namespace lumi {

class ImGuiSubpass : public RenderSubpass {
public:
    using RenderSubpass::RenderSubpass;

    virtual void Init(uint32_t subpass_idx) override;

    virtual void CmdRender(VkCommandBuffer cmd) override;

    void DestroyImGuiContext();
};

}  // namespace lumi