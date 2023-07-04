#pragma once

#include "material.h"

namespace lumi {

struct PBRMaterial : public Material {
    std::string diffuse_tex_name = "white";

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass) override;

    virtual void Upload(RenderResource* resource) override;
};

META(PBRMaterial) {
    // TODO: reflection
    Meta::class_<PBRMaterial>("PBRMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

}  // namespace lumi