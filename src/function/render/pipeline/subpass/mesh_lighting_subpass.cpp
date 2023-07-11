#include "mesh_lighting_subpass.h"

#include "function/cvars/cvar_system.h"
#include "function/render/pipeline/pass/render_pass.h"
#include "function/render/render_resource.h"

namespace lumi {

void MeshLightingSubpass::CmdRender(VkCommandBuffer cmd) {
    auto rhi      = render_pass_->rhi;
    auto resource = render_pass_->resource;

    // reorganize mesh
    std::unordered_map<
        Material*, std::unordered_map<Mesh*, std::vector<RenderObjectDesc*>>>
        drawcall_batch;

    for (auto& object : resource->visible_object_descs) {
        auto& batch = drawcall_batch[object.material][object.mesh];
        batch.emplace_back(&object);
    }

    auto     cur_instance       = resource->mesh_instances.data.cur_instance;
    uint32_t first_instance_idx = 0;

    for (auto& [material, material_batch] : drawcall_batch) {
        CmdBindMaterial(cmd, material);

        for (auto& [mesh, batch] : material_batch) {
            // Write to staging buffer
            for (auto& desc : batch) {
                RenderObject* object = desc->object;
                cur_instance->model_matrix =
                    Mat4x4f::Translation(object->position) *  //
                    Mat4x4f(object->rotation) *               //
                    Mat4x4f::Scale(object->scale);
                cur_instance++;
            }

            uint32_t batch_size = (uint32_t)batch.size();

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &mesh->vertex_buffer.buffer,
                                   &offset);
            vkCmdBindIndexBuffer(cmd, mesh->index_buffer.buffer, 0,
                                 Mesh::kVkIndexType);
            vkCmdDrawIndexed(cmd, (uint32_t)mesh->indices.size(), batch_size, 0,
                             0, first_instance_idx);

            first_instance_idx += batch_size;
        }
    }

    // Upload from staging buffer to gpu storage buffer
    // It's ok to do it here since the rendering has not commited yet.
    rhi->CopyBuffer(
        &resource->mesh_instances.staging_buffer,
        &resource->mesh_instances.buffer,
        sizeof(MeshInstanceSSBO) * resource->visible_object_descs.size(),
        resource->MeshInstanceSSBODynamicOffsets()[0]);
}

void MeshLightingSubpass::CmdBindMaterial(VkCommandBuffer cmd,
                                          Material*       material) {
    auto  resource = render_pass_->resource;
    auto& extent   = render_pass_->rhi->extent();

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

    vkCmdSetCullMode(cmd, material->cull_mode);

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