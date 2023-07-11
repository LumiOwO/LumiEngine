#pragma once

#include "core/meta.h"
#include "function/render/rhi/vulkan_utils.h"

namespace lumi {

class RenderResource;

enum DescriptorSetSlot {
    kDescriptorSetSlotMaterial = 0,
    kDescriptorSetSlotGlobal,
    kDescriptorSetSlotMeshInstance,

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

}  // namespace lumi