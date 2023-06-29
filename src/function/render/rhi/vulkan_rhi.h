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

    constexpr static uint64_t kTimeout = 1000000000ui64;  // Timeout of 1 second
    constexpr static int      kFramesInFlight          = 2;
    vk::FrameData             frames_[kFramesInFlight] = {};
    int                       frame_idx_               = 0;

    VkRenderPass               render_pass_{};
    std::vector<VkFramebuffer> frame_buffers_{};

    vk::UploadContext upload_context_{};

    int frame_number_ = 0; // TODO: delete

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

    void Render(const std::vector<vk::RenderObject>& renderables);

    void UploadMesh(vk::Mesh& vk_mesh);

    vk::Material CreateMaterial(const std::string& name);

private:
    void CreateVulkanInstance();
    void CreateSwapchain();
    void CreateCommands();
    void CreateDefaultRenderPass();
    void CreateFrameBuffers();
    void CreateSyncStructures();

    void RecreateSwapChain();

    void CmdBindPipeline(VkCommandBuffer cmd_buffer, VkPipeline pipeline);

    void RenderPass(VkCommandBuffer                      cmd_buffer,
                    const std::vector<vk::RenderObject>& renderables);
    void GUIPass(VkCommandBuffer cmd_buffer);

    VkResult WaitForCurrentFrame();

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func);

    void ImGuiInit();

    bool LoadShaderModule(const std::string& filepath,
                          VkShaderModule*    out_shader_module);
};

}  // namespace lumi