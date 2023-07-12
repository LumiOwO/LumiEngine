#include "unlit_material.h"

#include "function/render/render_resource.h"

namespace lumi {

void UnlitMaterial::CreateDescriptorSet(RenderResource* resource) {
    EditDescriptorSet(resource, false);
}

void UnlitMaterial::CreatePipeline(RenderResource* resource,
                                   VkRenderPass    render_pass,
                                   uint32_t        subpass_idx) {
    VkDevice device = resource->rhi->device();

    vk::PipelineBuilder pipeline_builder{};

    VkShaderModule vert =
        resource->CreateShaderModule(kShaderName, kShaderTypeVertex);
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               vert));
    VkShaderModule frag =
        resource->CreateShaderModule(kShaderName, kShaderTypeFragment);
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                               frag));

    std::vector<VkDescriptorSetLayout> set_layouts{
        this->descriptor_set.layout,
        resource->global.descriptor_set.layout,
        resource->mesh_instances.descriptor_set.layout,
    };

    auto pipeline_layout_info           = vk::BuildPipelineLayoutCreateInfo();
    pipeline_layout_info.setLayoutCount = (uint32_t)set_layouts.size();
    pipeline_layout_info.pSetLayouts    = set_layouts.data();

    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr,
                                    &this->pipeline_layout));
    pipeline_builder.pipeline_layout = this->pipeline_layout;

    pipeline_builder.vertex_input_info = vk::BuildVertexInputStateCreateInfo();

    pipeline_builder.input_assembly =
        vk::BuildInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_CULL_MODE);

    pipeline_builder.rasterizer =
        vk::BuildRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);

    pipeline_builder.multisample = vk::BuildMultisampleStateCreateInfo();

    pipeline_builder.color_blend_attachment =
        vk::BuildColorBlendAttachmentState();

    pipeline_builder.depth_stencil =
        vk::BuildPipelineDepthStencilStateCreateInfo(
            true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    vk::VertexInputDescription vertexDescription =
        vk::Vertex::GetVertexInputDescription();
    pipeline_builder.vertex_input_info.pVertexAttributeDescriptions =
        vertexDescription.attributes.data();
    pipeline_builder.vertex_input_info.vertexAttributeDescriptionCount =
        (uint32_t)vertexDescription.attributes.size();
    pipeline_builder.vertex_input_info.pVertexBindingDescriptions =
        vertexDescription.bindings.data();
    pipeline_builder.vertex_input_info.vertexBindingDescriptionCount =
        (uint32_t)vertexDescription.bindings.size();

    // Finally build pipeline
    this->pipeline = pipeline_builder.Build(device, render_pass, subpass_idx);

    resource->PushDestructor([this, device]() {
        vkDestroyPipeline(device, this->pipeline, nullptr);
        vkDestroyPipelineLayout(device, this->pipeline_layout, nullptr);
    });
}

void UnlitMaterial::Upload(RenderResource* resource) {
    EditDescriptorSet(resource, true);
}

void UnlitMaterial::EditDescriptorSet(RenderResource* resource,
                                      bool            update_only) {
    auto editor = resource->BeginEditDescriptorSet(&descriptor_set);
    auto rhi    = resource->rhi;

    // Update textures
    {
        vk::Texture* texture = resource->GetTexture(base_color_tex_name);
        if (texture == nullptr) {
            texture = resource->GetTexture(kDefaultBaseColorTexName);
        }
        VkSampler sampler = resource->GetSampler(texture->sampler_name);
        editor.BindImage(
            kBindingBaseColor, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, sampler, texture->image.image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    editor.Execute(update_only);
}

}  // namespace lumi