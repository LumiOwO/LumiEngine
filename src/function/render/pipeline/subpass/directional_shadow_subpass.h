#pragma once

#include "render_subpass.h"
#include "function/render/material/material.h"

namespace lumi {

struct DirectionalShadowMaterial : public Material {
    enum BindingSlot {
        kBindingSlotCount
    };

    constexpr static const char* kShaderName           = "shadow/directional";

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass,
                                uint32_t        subpass_idx) override;

    virtual void Upload(RenderResource* resource) override;

protected:
    virtual void EditDescriptorSet(RenderResource* resource,
                                   bool            update_only) override;
};

META(DirectionalShadowMaterial) {
    Meta::class_<DirectionalShadowMaterial>("DirectionalShadowMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

class DirectionalShadowSubpass : public RenderSubpass {
private:
    DirectionalShadowMaterial* material_{};

public:
    using RenderSubpass::RenderSubpass;

    virtual void Init(uint32_t subpass_idx) override;

    virtual void CmdRender(VkCommandBuffer cmd) override;
};

}  // namespace lumi