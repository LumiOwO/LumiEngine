#include "mesh_lighting_subpass.h"

#include "function/cvars/cvar_system.h"
#include "function/render/pipeline/pass/render_pass.h"
#include "function/render/render_resource.h"

namespace lumi {

void MeshLightingSubpass::CmdRender(VkCommandBuffer cmd) {
    auto rhi      = render_pass_->rhi;
    auto resource = render_pass_->resource;

    uint32_t first_instance_idx = 0;
    for (auto& [material, mat_batch] : resource->visibles_drawcall_batchs) {
        CmdBindMaterial(cmd, material);

        for (auto& [mesh, batch] : mat_batch) {
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
}

}  // namespace lumi