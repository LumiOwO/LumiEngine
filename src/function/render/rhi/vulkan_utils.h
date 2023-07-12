#pragma once

#include <functional>
#include <vector>
#include <fstream>

#include "vulkan_types.h"
#include "vkbootstrap/VkBootstrap.h"
#include "core/log.h"

#define VK_CHECK(x)                                   \
    do {                                              \
        VkResult err = x;                             \
        if (err) {                                    \
            LOG_ERROR("Vulkan error {} in " #x, err); \
            exit(1);                                  \
        }                                             \
    } while (0)

namespace lumi {

namespace vk {

inline VkCommandPoolCreateInfo BuildCommandPoolCreateInfo(
    uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {

    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext            = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags            = flags;
    return info;
}

inline VkCommandBufferAllocateInfo BuildCommandBufferAllocateInfo(
    VkCommandPool pool, uint32_t count = 1,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {

    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = nullptr;
    info.commandPool        = pool;
    info.commandBufferCount = count;
    info.level              = level;
    return info;
}

inline VkPipelineShaderStageCreateInfo BuildPipelineShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule shaderModule) {

    VkPipelineShaderStageCreateInfo info{};
    info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext  = nullptr;
    info.stage  = stage;
    info.module = shaderModule;
    info.pName  = "main";
    return info;
}

inline VkPipelineVertexInputStateCreateInfo BuildVertexInputStateCreateInfo() {
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.vertexBindingDescriptionCount   = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
}

inline VkPipelineInputAssemblyStateCreateInfo BuildInputAssemblyCreateInfo(
    VkPrimitiveTopology topology) {

    VkPipelineInputAssemblyStateCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext    = nullptr;
    info.topology = topology;
    info.primitiveRestartEnable = VK_FALSE;
    return info;
}

inline VkPipelineRasterizationStateCreateInfo BuildRasterizationStateCreateInfo(
    VkPolygonMode polygonMode) {

    VkPipelineRasterizationStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.depthClampEnable        = VK_FALSE;
    info.rasterizerDiscardEnable = VK_FALSE;
    info.polygonMode             = polygonMode;
    info.lineWidth               = 1.0f;
    info.cullMode                = VK_CULL_MODE_NONE;  // use dynamic
    info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    info.depthBiasEnable         = VK_FALSE;  // no depth bias
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp          = 0.0f;
    info.depthBiasSlopeFactor    = 0.0f;
    return info;
}

inline VkPipelineMultisampleStateCreateInfo BuildMultisampleStateCreateInfo() {
    VkPipelineMultisampleStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading      = 1.0f;
    info.pSampleMask           = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable      = VK_FALSE;
    return info;
}

inline VkPipelineColorBlendAttachmentState BuildColorBlendAttachmentState() {
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    return colorBlendAttachment;
}

inline VkPipelineLayoutCreateInfo BuildPipelineLayoutCreateInfo() {
    VkPipelineLayoutCreateInfo info{};
    info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext                  = nullptr;
    info.flags                  = 0;
    info.setLayoutCount         = 0;
    info.pSetLayouts            = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges    = nullptr;
    return info;
}

inline VkFramebufferCreateInfo BuildFramebufferCreateInfo(
    VkRenderPass render_pass, VkExtent2D window_extent) {

    VkFramebufferCreateInfo info{};
    info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext           = nullptr;
    info.renderPass      = render_pass;
    info.attachmentCount = 1;
    info.width           = window_extent.width;
    info.height          = window_extent.height;
    info.layers          = 1;
    return info;
}

inline VkFenceCreateInfo BuildFenceCreateInfo(VkFenceCreateFlags flags = 0) {
    VkFenceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

inline VkSemaphoreCreateInfo BuildSemaphoreCreateInfo(
    VkSemaphoreCreateFlags flags = 0) {

    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

inline VkCommandBufferBeginInfo BuildCommandBufferBeginInfo(
    VkCommandBufferUsageFlags flags = 0) {

    VkCommandBufferBeginInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext            = nullptr;
    info.pInheritanceInfo = nullptr;
    info.flags            = flags;
    return info;
}

inline VkSubmitInfo BuildSubmitInfo(VkCommandBuffer* p_cmd) {
    VkSubmitInfo info{};
    info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext                = nullptr;
    info.waitSemaphoreCount   = 0;
    info.pWaitSemaphores      = nullptr;
    info.pWaitDstStageMask    = nullptr;
    info.commandBufferCount   = 1;
    info.pCommandBuffers      = p_cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores    = nullptr;
    return info;
}

inline VkImageCreateInfo BuildImageCreateInfo(VkFormat          format,
                                              VkImageUsageFlags usageFlags,
                                              VkExtent3D        extent) {
    VkImageCreateInfo info{};
    info.sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext       = nullptr;
    info.imageType   = VK_IMAGE_TYPE_2D;
    info.format      = format;
    info.extent      = extent;
    info.mipLevels   = 1;
    info.arrayLayers = 1;
    info.samples     = VK_SAMPLE_COUNT_1_BIT;
    info.tiling      = VK_IMAGE_TILING_OPTIMAL;
    info.usage       = usageFlags;
    return info;
}

inline VkImageViewCreateInfo BuildImageViewCreateInfo(
    VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {

    VkImageViewCreateInfo info{};
    info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext    = nullptr;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image    = image;
    info.format   = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    info.subresourceRange.aspectMask     = aspectFlags;
    return info;
}

inline VkPipelineDepthStencilStateCreateInfo
BuildPipelineDepthStencilStateCreateInfo(bool bDepthTest, bool bDepthWrite,
                                         VkCompareOp compareOp) {
    VkPipelineDepthStencilStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;
    info.depthTestEnable       = bDepthTest ? VK_TRUE : VK_FALSE;
    info.depthWriteEnable      = bDepthWrite ? VK_TRUE : VK_FALSE;
    info.depthCompareOp        = bDepthTest ? compareOp : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds        = 0.0f;  // Optional
    info.maxDepthBounds        = 1.0f;  // Optional
    info.stencilTestEnable     = VK_FALSE;
    return info;
}

inline VkDescriptorSetLayoutBinding BuildDescriptorSetLayoutBinding(
    VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
    VkDescriptorSetLayoutBinding setbind{};
    setbind.binding            = binding;
    setbind.descriptorCount    = 1;
    setbind.descriptorType     = type;
    setbind.pImmutableSamplers = nullptr;
    setbind.stageFlags         = stageFlags;
    return setbind;
}

inline VkWriteDescriptorSet BuildWriteDescriptorSet(
    VkDescriptorType type, VkDescriptorSet dstSet,
    VkDescriptorBufferInfo* bufferInfo, uint32_t binding) {
    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext           = nullptr;
    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = bufferInfo;
    return write;
}

inline VkSamplerCreateInfo BuildSamplerCreateInfo(
    VkFilter             filters,
    VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT) {
    VkSamplerCreateInfo info{};
    info.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext        = nullptr;
    info.magFilter    = filters;
    info.minFilter    = filters;
    info.addressModeU = samplerAddressMode;
    info.addressModeV = samplerAddressMode;
    info.addressModeW = samplerAddressMode;
    return info;
}

inline VkWriteDescriptorSet BuildWriteDescriptorSet(
    VkDescriptorType type, VkDescriptorSet dstSet,
    VkDescriptorImageInfo* imageInfo, uint32_t binding) {
    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext           = nullptr;
    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = imageInfo;
    return write;
}

struct PipelineBuilder {
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages{};
    VkPipelineVertexInputStateCreateInfo         vertex_input_info{};
    VkPipelineInputAssemblyStateCreateInfo       input_assembly{};
    VkViewport                                   viewport{};
    VkRect2D                                     scissor{};
    VkPipelineRasterizationStateCreateInfo       rasterizer{};
    VkPipelineColorBlendAttachmentState          color_blend_attachment{};
    std::vector<VkDynamicState>                  dynamic_states{};
    VkPipelineMultisampleStateCreateInfo         multisample{};
    VkPipelineLayout                             pipeline_layout{};
    VkPipelineDepthStencilStateCreateInfo        depth_stencil{};

    VkPipeline Build(VkDevice device, VkRenderPass render_pass,
                     uint32_t subpass_idx) {
        // make viewport state from our stored viewport and scissor.
        // at the moment we won't support multiple viewports or scissors
        VkPipelineViewportStateCreateInfo viewport_state{};
        viewport_state.sType =
            VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.pNext         = nullptr;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports    = &viewport;
        viewport_state.scissorCount  = 1;
        viewport_state.pScissors     = &scissor;

        // setup dummy color blending. We aren't using transparent objects yet
        // the blending is just "no blend", but we do write to the color attachment
        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType =
            VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.pNext           = nullptr;
        color_blending.logicOpEnable   = VK_FALSE;
        color_blending.logicOp         = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = 1;
        color_blending.pAttachments    = &color_blend_attachment;

        // set dynamic viewport
        VkPipelineDynamicStateCreateInfo dynamic_info{};
        dynamic_info.sType =
            VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_info.pNext             = nullptr;
        dynamic_info.flags             = 0;
        dynamic_info.dynamicStateCount = (uint32_t)dynamic_states.size();
        dynamic_info.pDynamicStates    = dynamic_states.data();

        // build the actual pipeline
        // we now use all of the info structs we have been writing
        // into this one to create the pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.stageCount          = (uint32_t)shader_stages.size();
        pipelineInfo.pStages             = shader_stages.data();
        pipelineInfo.pVertexInputState   = &vertex_input_info;
        pipelineInfo.pInputAssemblyState = &input_assembly;
        pipelineInfo.pViewportState      = &viewport_state;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisample;
        pipelineInfo.pColorBlendState    = &color_blending;
        pipelineInfo.pDynamicState       = &dynamic_info;
        pipelineInfo.layout              = pipeline_layout;
        pipelineInfo.renderPass          = render_pass;
        pipelineInfo.subpass             = subpass_idx;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState  = &depth_stencil;

        // it's easy to error out on create graphics pipeline,
        // so we handle it a bit better than the common VK_CHECK case
        VkPipeline newPipeline;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                      nullptr, &newPipeline) != VK_SUCCESS) {
            LOG_ERROR("Failed to create pipeline");
            return VK_NULL_HANDLE;  // failed to create graphics pipeline
        } else {
            return newPipeline;
        }
    }
};

}  // namespace vk
}  // namespace lumi