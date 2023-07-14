#include "render_subpass.h"

#include "function/render/pipeline/pass/render_pass.h"
#include "function/render/render_resource.h"

namespace lumi {

void RenderSubpass::CmdBindMaterial(VkCommandBuffer cmd, Material* material) {
    auto  resource = render_pass_->resource;
    auto& extent   = render_pass_->GetExtent();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = (float)extent.height;  // flip y
    viewport.width    = (float)extent.width;
    viewport.height   = -(float)extent.height;  // flip y
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdSetCullMode(cmd, material->double_sided ? VK_CULL_MODE_NONE
                                                 : VK_CULL_MODE_BACK_BIT);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            material->pipeline_layout,
                            kDescriptorSetSlotMaterial, 1,
                            &material->descriptor_set.set, 0, nullptr);

    auto global_offsets = resource->GlobalSSBODynamicOffsets();
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline_layout,
        kDescriptorSetSlotGlobal, 1, &resource->global.descriptor_set.set,
        (uint32_t)global_offsets.size(), global_offsets.data());

    auto mesh_instance_offsets = resource->MeshInstanceSSBODynamicOffsets();
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline_layout,
        kDescriptorSetSlotMeshInstance, 1,
        &resource->mesh_instances.descriptor_set.set,
        (uint32_t)mesh_instance_offsets.size(), mesh_instance_offsets.data());
}

}  // namespace lumi