#pragma once

#include "core/meta.h"
#include "function/render/rhi/vulkan_utils.h"

namespace lumi {

class RenderResource;

enum DescriptorSetSlot {
    kPerMaterialSlot = 0,
    kPerFrameSlot,
    kPerVisibleSlot,

    kDescriptorSetSlotsCount
};

struct Material {
    vk::DescriptorSet descriptor_set{};
    VkPipeline        pipeline{};
    VkPipelineLayout  pipeline_layout{};
    VkCullModeFlags   cull_mode = VK_CULL_MODE_BACK_BIT;

    virtual void CreateDescriptorSet(RenderResource* resource) = 0;

    virtual void CreatePipeline(RenderResource* resource,
                                VkRenderPass    render_pass) = 0;
    
    virtual void Upload(RenderResource* resource) = 0;
};


//struct TestMaterial : public Material {
//    struct Param {
//        Vec4f color = Vec4f::kZero;
//    } params = {};
//    vk::AllocatedBuffer buffer{};
//
//    std::string texture_name = "";
//};

}  // namespace lumi