#pragma once

#include "render_subpass.h"

namespace lumi {

class ImGuiSubpass : public RenderSubpass {
public:
    using RenderSubpass::RenderSubpass;

    virtual void CmdRender(VkCommandBuffer cmd) override;

    void CreateImGuiContext(uint32_t subpass_idx);

    void DestroyImGuiContext();
};

}  // namespace lumi