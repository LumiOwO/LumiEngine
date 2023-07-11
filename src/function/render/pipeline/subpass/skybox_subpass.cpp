#include "skybox_subpass.h"

#include "function/cvars/cvar_system.h"
#include "function/render/pipeline/pass/render_pass.h"
#include "function/render/render_resource.h"

namespace lumi {

void SkyboxMaterial::CreateDescriptorSet(RenderResource* resource) {
    EditDescriptorSet(resource, false);
}

void SkyboxMaterial::CreatePipeline(RenderResource* resource,
                                    VkRenderPass    render_pass) {
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

    // Empty vertex description is ok
    vk::VertexInputDescription vertexDescription = {};
    pipeline_builder.vertex_input_info.pVertexAttributeDescriptions =
        vertexDescription.attributes.data();
    pipeline_builder.vertex_input_info.vertexAttributeDescriptionCount =
        (uint32_t)vertexDescription.attributes.size();
    pipeline_builder.vertex_input_info.pVertexBindingDescriptions =
        vertexDescription.bindings.data();
    pipeline_builder.vertex_input_info.vertexBindingDescriptionCount =
        (uint32_t)vertexDescription.bindings.size();

    // Finally build pipeline
    // TODO: !fix skybox subpass index!
    this->pipeline = pipeline_builder.Build(device, render_pass, 1);

    resource->PushDestructor([this, device]() {
        vkDestroyPipeline(device, this->pipeline, nullptr);
        vkDestroyPipelineLayout(device, this->pipeline_layout, nullptr);
    });
}

void SkyboxMaterial::Upload(RenderResource* resource) {
    EditDescriptorSet(resource, true);
}

void SkyboxMaterial::EditDescriptorSet(RenderResource* resource,
                                       bool            update_only) {
    auto editor = resource->EditDescriptorSet(&descriptor_set);
    auto rhi    = resource->rhi;

    // Update textures
    {
        vk::Texture* texture = resource->GetTexture(skybox_tex_name);
        if (texture == nullptr) {
            texture = resource->GetTexture(kDefaultSkyboxTexName);
        }
        VkSampler sampler = resource->GetSampler(texture->sampler_name);
        editor.BindImage(
            kBindingSkyboxCubeMap, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, sampler, texture->image.image_view,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    editor.Execute(update_only);
}

void SkyboxSubpass::Init(uint32_t subpass_idx) {
    auto resource = render_pass_->resource;

    material_ = (SkyboxMaterial*)resource->CreateMaterial(
        "_skybox", "SkyboxMaterial", render_pass_->vk_render_pass());
    material_->skybox_tex_name = "skybox_specular";
    material_->Upload(resource.get());
}

void SkyboxSubpass::CmdRender(VkCommandBuffer cmd) {
    CmdBindMaterial(cmd, material_);
    // 2 triangles (6 vertex) each face, 6 faces
    vkCmdDraw(cmd, 36, 1, 0, 0);
}

}  // namespace lumi