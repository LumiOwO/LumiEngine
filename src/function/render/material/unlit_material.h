#pragma once

#include "material.h"

namespace lumi {

struct UnlitMaterial : public Material {
    enum BindingSlot {
        kBindingBaseColor = 0,
        kBindingSlotCount
    };

    constexpr static const char* kDefaultBaseColorTexName = "white";
    constexpr static const char* kShaderName              = "unlit";

    std::string base_color_tex_name = kDefaultBaseColorTexName;

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass,
                                uint32_t        subpass_idx) override;

    virtual void Upload(RenderResource* resource) override;

protected:
    virtual void EditDescriptorSet(RenderResource* resource,
                                   bool            update_only) override;
};

META(UnlitMaterial) {
    Meta::class_<UnlitMaterial>("UnlitMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

}  // namespace lumi