#pragma once

#include "vulkan_utils.h"

namespace lumi {

class VulkanRHI {
private:
    vk::DestructionQueue destruction_queue_default_{};
    vk::DestructionQueue destruction_queue_swapchain_{};

    VkInstance       instance_{};         // Vulkan library handle
    VkPhysicalDevice physical_device_{};  // GPU chosen as the default device
    VkDevice         device_{};           // Vulkan device for commands
    VkSurfaceKHR     surface_{};          // Vulkan window surface
    VmaAllocator     allocator_{};

#ifdef LUMI_ENABLE_DEBUG_LOG
    VkDebugUtilsMessengerEXT debug_messenger_{};  // Vulkan debug output handle
#endif

    VkExtent2D               extent_{};
    VkSwapchainKHR           swapchain_{};
    VkFormat                 swapchain_image_format_{};
    std::vector<VkImage>     swapchain_images_{};
    std::vector<VkImageView> swapchain_image_views_{};
    vk::AllocatedImage       depth_image_{};
    VkImageView              depth_image_view_{};
    VkFormat                 depth_format_{};

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
    VkPipelineLayout mesh_pipeline_layout_{};
    VkPipeline       triangle_pipeline_{};
    VkPipeline       mesh_pipeline_{};
    vk::Mesh         triangle_mesh_{};

    vk::UploadContext upload_context_{};

    int frame_number_ = 0;

public:
    using CreateSurfaceFunc =
        std::function<VkResult(VkInstance, VkSurfaceKHR*)>;
    using GetWindowExtentFunc     = std::function<VkExtent2D()>;
    using ImGuiWindowInitFunc     = std::function<void()>;
    using ImGuiWindowShutdownFunc = std::function<void()>;
    using ImGuiWindowNewFrameFunc = std::function<void()>;

    // function objects provided by window
    CreateSurfaceFunc       CreateSurface{};
    GetWindowExtentFunc     GetWindowExtent{};
    ImGuiWindowInitFunc     ImGuiWindowInit{};
    ImGuiWindowShutdownFunc ImGuiWindowShutdown{};
    ImGuiWindowNewFrameFunc ImGuiWindowNewFrame{};
    
    void Init();

    void Finalize();

    void Render();

    bool LoadShaderModule(const char*     filepath,
                          VkShaderModule* out_shader_module);

private:
    void CreateVulkanInstance();
    void CreateSwapchain();
    void CreateCommands();
    void CreateDefaultRenderPass();
    void CreateFrameBuffers();
    void CreateSyncStructures();
    void CreatePipelines();

    void RecreateSwapChain();

    void BindPipeline();

    void RenderPass();
    void GUIPass();

    VkResult WaitForLastFrame();

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func);

    void ImGuiInit();

    void LoadMeshes();

    void UploadMesh(vk::Mesh& vk_mesh);
};

}  // namespace lumi