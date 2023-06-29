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

    struct Hash {
        std::size_t operator()(const Vertex& v) const {
            std::size_t result = 0;
            hash_combine<glm::vec3>(result, v.position);
            hash_combine<glm::vec3>(result, v.normal);
            hash_combine<glm::vec3>(result, v.color);
            hash_combine<glm::vec2>(result, v.texcoord);
            return result;
        }
    };

    static VertexInputDescription GetVertexInputDescription() {
        VertexInputDescription description{};

        // we will have just 1 vertex buffer binding, with a per-vertex rate
        VkVertexInputBindingDescription mainBinding{};
        mainBinding.binding   = 0;
        mainBinding.stride    = sizeof(Vertex);
        mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        description.bindings.emplace_back(mainBinding);

        VkVertexInputAttributeDescription positionAttribute{};
        positionAttribute.binding  = 0;
        positionAttribute.location = 0;
        positionAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
        positionAttribute.offset   = offsetof(Vertex, position);
        description.attributes.emplace_back(positionAttribute);

        VkVertexInputAttributeDescription normalAttribute{};
        normalAttribute.binding  = 0;
        normalAttribute.location = 1;
        normalAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
        normalAttribute.offset   = offsetof(Vertex, normal);
        description.attributes.emplace_back(normalAttribute);

        VkVertexInputAttributeDescription colorAttribute{};
        colorAttribute.binding  = 0;
        colorAttribute.location = 2;
        colorAttribute.format   = VK_FORMAT_R32G32B32_SFLOAT;
        colorAttribute.offset   = offsetof(Vertex, color);
        description.attributes.emplace_back(colorAttribute);

        VkVertexInputAttributeDescription texcoordAttribute{};
        texcoordAttribute.binding  = 0;
        texcoordAttribute.location = 3;
        texcoordAttribute.format   = VK_FORMAT_R32G32_SFLOAT;
        texcoordAttribute.offset   = offsetof(Vertex, texcoord);
        description.attributes.emplace_back(texcoordAttribute);

        return description;
    }
};

struct Mesh {
    using IndexType                           = uint32_t;
    constexpr static VkIndexType kVkIndexType = VK_INDEX_TYPE_UINT32;

    std::vector<Vertex>    vertices{};
    std::vector<IndexType> indices{};
    AllocatedBuffer        vertex_buffer{};
    AllocatedBuffer        index_buffer{};
};

struct MeshPushConstants {
    Vec4f   color{};
    Mat4x4f model{};
};

struct Texture {
    VkFormat       format{};
    AllocatedImage image{};
    VkImageView    image_view{};
};

struct Material {
    VkPipeline       pipeline{};
    VkPipelineLayout pipeline_layout{};
    VkDescriptorSet  texture_set{};
};

struct RenderObject {
    Mesh*      mesh     = nullptr;
    Material*  material = nullptr;
    Vec3f      position = Vec3f::kZero;
    Quaternion rotation = Quaternion::kIdentity;
    Vec3f      scale    = Vec3f::kUnitScale;
};

struct CameraData {
    Mat4x4f view{};
    Mat4x4f proj{};
    Mat4x4f proj_view{};
};

struct EnvLightingData {
    Vec4f fog_color{};      // w is for exponent
    Vec4f fog_distances{};  // x for min, y for max, zw unused.
    Vec4f ambient_color{};
    Vec4f sunlight_direction{};  // w for sun power
    Vec4f sunlight_color{};
};

struct MeshInstanceData {
    Mat4x4f model_matrix{};
};

struct FrameData {
    VkCommandPool   command_pool{};
    VkCommandBuffer main_command_buffer{};
    VkFence         render_fence{};
    VkSemaphore     render_semaphore{};
    VkSemaphore     present_semaphore{};

    AllocatedBuffer camera_buffer{};
    AllocatedBuffer mesh_instance_buffer{};

    VkDescriptorSet global_descriptor_set{};
    VkDescriptorSet mesh_instance_descriptor_set{};
};

}  // namespace vk
}  // namespace lumi