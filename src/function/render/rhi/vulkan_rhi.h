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
    VkPhysicalDeviceProperties gpu_properties_{};

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
    
    vk::AllocatedBuffer env_lighting_buffer_{};

    VkRenderPass               render_pass_{};
    std::vector<VkFramebuffer> frame_buffers_{};

    VkDescriptorPool      descriptor_pool_{};
    VkDescriptorSetLayout global_set_layout_{};
    VkDescriptorSetLayout mesh_instance_set_layout_{};
    VkDescriptorSetLayout single_texture_set_layout_{};

    vk::UploadContext upload_context_{};

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

    void UploadMesh(vk::Mesh& mesh);

    void UploadTexture(vk::Texture& texture, const void* pixels, int width,
                       int height, int channels);

    // TODO: refactor material system
    vk::Material CreateMaterial(const std::string& name, VkImageView temp);

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func);

private:
    void CreateVulkanInstance();
    void CreateSwapchain();
    void CreateCommands();
    void CreateDefaultRenderPass();
    void CreateFrameBuffers();
    void CreateSyncStructures();

    void CreateDescriptors();

    void RecreateSwapChain();

    void CmdBindPipeline(VkCommandBuffer cmd_buffer, vk::Material* material);

    void RenderPass(VkCommandBuffer                      cmd_buffer,
                    const std::vector<vk::RenderObject>& renderables);
    void GUIPass(VkCommandBuffer cmd_buffer);

    VkResult WaitForCurrentFrame();

    void ImGuiInit();

    bool LoadShaderModule(const std::string& filepath,
                          VkShaderModule*    out_shader_module);

    void CopyBuffer(const void* src, vk::AllocatedBuffer dst, size_t size);

    void CopyBuffer(const vk::AllocatedBuffer src, vk::AllocatedBuffer dst,
                    size_t size);

    vk::AllocatedBuffer CreateBuffer(size_t             alloc_size,
                                     VkBufferUsageFlags buffer_usage,
                                     VmaMemoryUsage     memory_usage,
                                     bool               auto_destroy = true);

    void DestroyBuffer(vk::AllocatedBuffer buffer);

    vk::AllocatedImage CreateImage2D(int               width,   //
                                     int               height,  //
                                     VkFormat          image_format,
                                     VkImageUsageFlags image_usage,
                                     VmaMemoryUsage    memory_usage,
                                     bool              auto_destroy = true);

    void DestroyImage(vk::AllocatedImage image);

    template <typename T>
    size_t PaddedSizeOf() {
        // Calculate required alignment based on minimum device offset alignment
        size_t minUboAlignment =
            gpu_properties_.limits.minUniformBufferOffsetAlignment;
        size_t alignedSize = sizeof(T);
        if (minUboAlignment > 0) {
            alignedSize =
                (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
        }
        return alignedSize;
    }
};

}  // namespace lumi