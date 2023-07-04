#pragma once

#include "render_subpass.h"

namespace lumi {

struct Material;

class MeshLightingSubpass : public RenderSubpass {
public:
    using RenderSubpass::RenderSubpass;

    virtual void CmdRender(VkCommandBuffer cmd) override;

private:
    void CmdBindMaterial(VkCommandBuffer cmd, Material* material);
};

}  // namespace lumi