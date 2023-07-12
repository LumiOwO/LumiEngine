#pragma once

#include "material.h"

namespace lumi {

struct SkyboxMaterial : public Material {
    enum BindingSlot {
        kBindingSkyboxIrradiance = 0,
        kBindingSkyboxSpecular,

        kBindingSlotCount
    };

    constexpr static const char* kDefaultSkyboxTexName = "skybox_empty";
    constexpr static const char* kShaderName           = "skybox";

    std::string irradiance_cubemap_name = kDefaultSkyboxTexName;
    std::string specular_cubemap_name   = kDefaultSkyboxTexName;

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass,
                                uint32_t        subpass_idx) override;

    virtual void Upload(RenderResource* resource) override;

protected:
    virtual void EditDescriptorSet(RenderResource* resource,
                                   bool            update_only) override;
};

META(SkyboxMaterial) {
    Meta::class_<SkyboxMaterial>("SkyboxMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

}  // namespace lumi