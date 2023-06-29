#pragma once

#include "core/math.h"
#include "core/hash.h"

#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"

namespace lumi {
namespace vk {

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

struct AllocatedBuffer {
    VkBuffer      buffer{};
    VmaAllocation allocation{};
};

struct AllocatedImage {
    VkImage       image{};
    VmaAllocation allocation{};
};

struct FrameData {
    VkCommandPool   command_pool{};
    VkCommandBuffer main_command_buffer{};
    VkFence         render_fence{};
    VkSemaphore     render_semaphore{};
    VkSemaphore     present_semaphore{};
};

struct UploadContext {
    VkCommandPool   command_pool{};
    VkCommandBuffer command_buffer{};
    VkFence         upload_fence{};
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
    Vec2f texcoord{};

    bool operator==(const Vertex& rhs) const {
        return position == rhs.position && normal == rhs.normal &&
               color == rhs.color && texcoord == rhs.texcoord;
    }
};

struct Mesh {
    using IndexType                           = uint16_t;
    constexpr static VkIndexType kVkIndexType = VK_INDEX_TYPE_UINT16;

    std::vector<Vertex>    vertices{};
    std::vector<IndexType> indices{};
    AllocatedBuffer        vertex_buffer{};
    AllocatedBuffer        index_buffer{};
};

struct MeshPushConstants {
    Vec4f   color{};
    Mat4x4f mvp{};
};

struct Material {
    VkPipeline       pipeline{};
    VkPipelineLayout pipelineLayout{};
};

struct RenderObject {
    Mesh*      mesh     = nullptr;
    Material*  material = nullptr;
    Vec3f      position = Vec3f::kZero;
    Quaternion rotation = Quaternion::kIdentity;
    Vec3f      scale    = Vec3f::kUnitScale;
};

}  // namespace vk
}  // namespace lumi


// hash for Vertex
namespace std {
template <>
struct hash<lumi::vk::Vertex> {
    std::size_t operator()(const lumi::vk::Vertex& v) const {
        std::size_t result = 0;
        lumi::hash_combine<glm::vec3>(result, v.position);
        lumi::hash_combine<glm::vec3>(result, v.normal);
        lumi::hash_combine<glm::vec3>(result, v.color);
        lumi::hash_combine<glm::vec2>(result, v.texcoord);
        return result;
    }
};
}  // namespace std