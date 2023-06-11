#pragma once

#include "vulkan_utils.h"

namespace lumi {

class VulkanRHI {
private:
    vk::DestructionQueue destruction_queue_default_{};
    vk::DestructionQueue destruction_queue_swapchain_{};

    VkInstance               instance_{};         // Vulkan library handle
    VkDebugUtilsMessengerEXT debug_messenger_{};  // Vulkan debug output handle
    VkPhysicalDevice physical_device_{};  // GPU chosen as the default device
    VkDevice         device_{};           // Vulkan device for commands
    VkSurfaceKHR     surface_{};          // Vulkan window surface

    VkExtent2D               window_extent_{};
    VkSwapchainKHR           swapchain_{};
    VkFormat                 swapchain_image_format_{};
    std::vector<VkImage>     swapchain_images_{};
    std::vector<VkImageView> swapchain_image_views_{};

    VkQueue         graphics_queue_{};         // queue we will submit to
    uint32_t        graphics_queue_family_{};  // family of that queue
    VkCommandPool   command_pool_{};  // the command pool for our commands
    VkCommandBuffer main_command_buffer_{};  // the buffer we will record into

    VkRenderPass               render_pass_{};
    std::vector<VkFramebuffer> frame_buffers_{};

    constexpr static uint64_t kTimeout = 1000000000ui64;  // Timeout of 1 second
    VkSemaphore               present_semaphore_{};
    VkSemaphore               render_semaphore_{};
    VkFence                   render_fence_{};

    VkPipelineLayout triangle_pipeline_layout_{};
    VkPipeline       triangle_pipeline_{};

public:
    using fCreateSurface = std::function<VkResult(VkInstance, VkSurfaceKHR*)>;
    struct InitInfo {
        fCreateSurface CreateSurface{};
        int            width  = 0;
        int            height = 0;
    };

    void Init(InitInfo init_info);

    void Finalize();

    void Draw();

    bool LoadShaderModule(const char*     filepath,
                          VkShaderModule* out_shader_module);

private:
    void InitVulkan(fCreateSurface CreateSurface);
    void InitSwapchain(const int width, const int height);
    void InitCommands();
    void InitDefaultRenderPass();
    void InitFrameBuffers();
    void InitSyncStructures();
    void InitPipelines();
};

}  // namespace lumi