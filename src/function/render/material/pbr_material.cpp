#include "pbr_material.h"

#include "function/render/render_resource.h"

namespace lumi {

void PBRMaterial::CreateDescriptorSet(RenderResource* resource) {
    vk::Texture2D* diffuse_tex = resource->GetTexture2D(diffuse_tex_name);
    if (diffuse_tex == nullptr) {
        diffuse_tex = resource->GetTexture2D("white");
    }

    resource->EditDescriptorSet(&descriptor_set)
        .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                   VK_SHADER_STAGE_FRAGMENT_BIT, resource->sampler_nearest,
                   diffuse_tex->image.image_view,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .Execute();
}

void PBRMaterial::CreatePipeline(RenderResource* resource,
                                 VkRenderPass    render_pass) {
    VkDevice device = resource->rhi->device();

    vk::PipelineBuilder pipeline_builder{};

    VkShaderModule vert =
        resource->CreateShaderModule("meshtriangle", kShaderTypeVertex);
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               vert));
    VkShaderModule frag =
        resource->CreateShaderModule("meshtriangle", kShaderTypeFragment);
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                               frag));

    std::vector<VkDescriptorSetLayout> set_layouts{
        this->descriptor_set.layout,
        resource->per_frame.descriptor_set.layout,
        resource->per_visible.descriptor_set.layout,
    };

    auto pipeline_layout_info = vk::BuildPipelineLayoutCreateInfo();
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
    this->pipeline = pipeline_builder.Build(device, render_pass);

    resource->PushDestructor([this, device]() {
        vkDestroyPipeline(device, this->pipeline, nullptr);
        vkDestroyPipelineLayout(device, this->pipeline_layout, nullptr);
    });
}

void PBRMaterial::Upload(RenderResource* resource) {
    // Update textures
    vk::Texture2D* diffuse_tex = resource->GetTexture2D(diffuse_tex_name);
    if (diffuse_tex == nullptr) {
        diffuse_tex = resource->GetTexture2D("white");
    }

    resource->EditDescriptorSet(&descriptor_set)
        .BindImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                   VK_SHADER_STAGE_FRAGMENT_BIT, resource->sampler_nearest,
                   diffuse_tex->image.image_view,
                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        .Execute(true);
}

}  // namespace lumi