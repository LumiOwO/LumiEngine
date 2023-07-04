#pragma once

#include "core/hash.h"
#include "core/math.h"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan.h"

namespace lumi {
namespace vk {

// class for destroying vulkan resources
class DestructorQueue {
private:
    std::vector<std::function<void()>> destructors_{};

public:
    void Push(std::function<void()>&& destructor) {
        destructors_.emplace_back(std::move(destructor));
    }

    void Flush() {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); it++) {
            (*it)();
        }
        destructors_.clear();
    }
};

struct AllocatedBuffer {
    VmaAllocation allocation{};
    VkBuffer      buffer{};
};

struct AllocatedImage {
    VmaAllocation allocation{};
    VkImage       image{};
    VkImageView   image_view{};
};

struct Texture2D {
    uint32_t           width{};
    uint32_t           height{};
    VkFormat           format{};
    vk::AllocatedImage image{};
};

struct Texture2DCreateInfo {
    uint32_t                 width{};
    uint32_t                 height{};
    VkFormat                 format{};
    VkImageUsageFlags        image_usage{};
    VmaMemoryUsage           memory_usage{};
    VkImageAspectFlags       aspect_flags{};
};

struct DescriptorSet {
    VkDescriptorSet       set{};
    VkDescriptorSetLayout layout{};
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
            HashCombine<glm::vec3>(result, v.position);
            HashCombine<glm::vec3>(result, v.normal);
            HashCombine<glm::vec3>(result, v.color);
            HashCombine<glm::vec2>(result, v.texcoord);
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

}  // namespace vk
}  // namespace lumi