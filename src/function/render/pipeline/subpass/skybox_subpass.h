#pragma once

#include "render_subpass.h"
#include "function/render/material/material.h"

namespace lumi {

struct SkyboxMaterial : public Material {
    enum BindingSlot { 
        kBindingSkyboxCubeMap = 0, 
        kBindingSlotCount 
    };

    // TODO: change to default skybox
    constexpr static const char* kDefaultSkyboxTexName = "white";
    constexpr static const char* kShaderName           = "skybox";

    std::string skybox_tex_name = kDefaultSkyboxTexName;

    virtual void CreateDescriptorSet(RenderResource* resource) override;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass) override;

    virtual void Upload(RenderResource* resource) override;

protected:
    virtual void EditDescriptorSet(RenderResource* resource,
                                   bool            update_only) override;
};

META(SkyboxMaterial) {
    Meta::class_<SkyboxMaterial>("SkyboxMaterial")
        .constructor<>()(meta::Policy::ctor::as_std_shared_ptr);
}

class SkyboxSubpass : public RenderSubpass {
private:
    SkyboxMaterial* material_{};

public:
    using RenderSubpass::RenderSubpass;

    virtual void Init(uint32_t subpass_idx) override;

    virtual void CmdRender(VkCommandBuffer cmd) override;
};

}  // namespace lumi