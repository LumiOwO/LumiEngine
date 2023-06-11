#pragma once

#include <functional>
#include <vector>

#include "core/log.h"
#include "vulkan/vulkan.h"

#include "vkbootstrap/VkBootstrap.h"

#define VK_CHECK(x)                                             \
    do {                                                        \
        VkResult err = x;                                       \
        if (err) {                                              \
            LOG_ERROR("{}: Detected Vulkan error {}", #x, err); \
            exit(1);                                            \
        }                                                       \
    } while (0)


namespace lumi {

namespace vk {

inline VkCommandPoolCreateInfo BuildCommandPoolCreateInfo(
    uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0) {
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext            = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags            = flags;
    return info;
}

inline VkCommandBufferAllocateInfo BuildCommandBufferAllocateInfo(
    VkCommandPool pool, uint32_t count = 1,
    VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
    VkCommandBufferAllocateInfo info{};
    info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext              = nullptr;
    info.commandPool        = pool;
    info.commandBufferCount = count;
    info.level              = level;
    return info;
}

// class for destroying vulkan resources
class DestructionQueue {
private:
    std::vector<std::function<void()>> destructors_{};

public:
    void push(std::function<void()>&& destructor) {
        destructors_.emplace_back(destructor);
    }

    void flush() {
        for (auto it = destructors_.rbegin(); it != destructors_.rend(); it++) {
            (*it)();
        }
        destructors_.clear();
    }
};

}  // namespace vk

}  // namespace lumi