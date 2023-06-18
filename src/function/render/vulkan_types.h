#pragma once

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

#include "core/math.h"

namespace lumi {

namespace vk {

struct AllocatedBuffer {
    VkBuffer      buffer{};
    VmaAllocation allocation{};
};

struct AllocatedImage {
    VkImage       image{};
    VmaAllocation allocation{};
};

struct UploadContext {
    VkFence         upload_fence{};
    VkCommandPool   command_pool{};
    VkCommandBuffer command_buffer{};
};

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription>   bindings{};
    std::vector<VkVertexInputAttributeDescription> attributes{};
    VkPipelineVertexInputStateCreateFlags          flags{};
};

struct Vertex {
    Vec3f position{};
    Vec3f normal{};
    Vec3f color{};
};

struct Mesh {
    std::vector<Vertex> vertices{};
    AllocatedBuffer     vertex_buffer{};
};

struct MeshPushConstants {
    Vec4f   color{};
    Mat4x4f mvp{};
};

// class for destroying vulkan resources
class DestructionQueue {
private:
    std::vector<std::function<void()>> destructors_{};

public:
    void Push(std::function<void()>&& destructor) {
        destructors_.emplace_back(destructor);
    }

    void Flush() {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); it++) {
            (*it)();
        }
        destructors_.clear();
    }
};
}  // namespace vk

}  // namespace lumi