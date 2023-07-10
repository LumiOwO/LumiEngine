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
                                VkRenderPass    render_pass) override;

    virtual void Upload(RenderResource* resource) override;

private:
    void EditDescriptorSet(RenderResource* resource, bool update_only);
};

META(UnlitMaterial) {
    // TODO: reflection
    Meta::class_<UnlitMaterial>("UnlitMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

}  // namespace lumi