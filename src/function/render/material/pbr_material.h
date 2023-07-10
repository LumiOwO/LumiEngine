#pragma once

#include "material.h"

namespace lumi {

struct PBRMaterial : public Material {
    enum AlphaMode {
        kAlphaModeOpaque = 0,
        kAlphaModeMask,
        kAlphaModeBlend,
    };

    enum BindingSlot {
        kBindingBaseColor = 0,
        kBindingMetallicRoughness,
        kBindingNormal,
        kBindingOcclusion,
        kBindingEmissive,
        kBindingTexturesCount,

        kBindingParameters = kBindingTexturesCount,
        kBindingSlotCount
    };

    constexpr static const char* kDefaultBaseColorTexName         = "white";
    constexpr static const char* kDefaultMetallicRoughnessTexName = "white";
    constexpr static const char* kDefaultNormalTexName            = "white";
    constexpr static const char* kDefaultOcclusionTexName         = "white";
    constexpr static const char* kDefaultEmissiveTexName          = "white";
    constexpr static const char* kShaderName                      = "pbr";

    struct Params {
        uint32_t texcoord_set_base_color         = 0;
        uint32_t texcoord_set_metallic_roughness = 0;
        uint32_t texcoord_set_normal             = 0;
        uint32_t texcoord_set_occlusion          = 0;
        uint32_t texcoord_set_emissive           = 0;
        uint32_t _padding_texcoord_set[3];

        uint32_t alpha_mode        = kAlphaModeOpaque;
        float    alpha_cutoff      = 1.0f;
        float    metallic_factor   = 1.0f;
        float    roughness_factor  = 1.0f;
        Vec4f    base_color_factor = Vec4f(1.0f);
        Vec4f    emissive_factor   = Vec4f(1.0f);
    };

    struct ParamsPack {
        Params*             data{};
        vk::AllocatedBuffer staging_buffer{};
        vk::AllocatedBuffer buffer{};
    } params = {};

    std::string base_color_tex_name         = kDefaultBaseColorTexName;
    std::string metallic_roughness_tex_name = kDefaultMetallicRoughnessTexName;
    std::string normal_tex_name             = kDefaultNormalTexName;
    std::string occlusion_tex_name          = kDefaultOcclusionTexName;
    std::string emissive_tex_name           = kDefaultEmissiveTexName;

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