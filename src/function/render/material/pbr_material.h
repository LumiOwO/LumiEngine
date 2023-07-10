#pragma once

#include "material.h"

namespace lumi {

struct PBRMaterial : public Material {
    enum AlphaMode {
        kAlphaModeOpaque = 0,
        kAlphaModeMask,
        kAlphaModeBlend,
    };

    struct Params {
        AlphaMode alpha_mode        = kAlphaModeOpaque;
        float     alpha_cutoff      = 1.0f;
        float     metallic_factor   = 1.0f;
        float     roughness_factor  = 1.0f;
        Vec4f     base_color_factor = Vec4f(1.0f);
        Vec4f     emissive_factor   = Vec4f(1.0f);

        uint8_t texcoord_set_base_color         = 0;
        uint8_t texcoord_set_metallic_roughness = 0;
        uint8_t texcoord_set_normal             = 0;
        uint8_t texcoord_set_occlusion          = 0;
        uint8_t texcoord_set_emissive           = 0;
    } params = {};

    // TODO: remove
    std::string diffuse_tex_name = "white";

    std::string base_color_tex_name         = "white";
    std::string metallic_roughness_tex_name = "white";
    std::string normal_tex_name             = "white";
    std::string occlusion_tex_name          = "white";
    std::string emissive_tex_name           = "white";

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass) override;

    virtual void Upload(RenderResource* resource) override;

private:
    void EditDescriptorSet(RenderResource* resource, bool update_only);
};

META(PBRMaterial) {
    // TODO: reflection
    Meta::class_<PBRMaterial>("PBRMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

}  // namespace lumi