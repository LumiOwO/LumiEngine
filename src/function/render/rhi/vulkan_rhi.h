#pragma once

#include "vulkan_utils.h"

namespace lumi {

class RenderScene;

class VulkanRHI {
public:
    constexpr static int      kFramesInFlight = 2;
    constexpr static uint64_t kTimeout = 1000000000ui64;  // Timeout of 1 second

private:
    vk::DestructorQueue dtor_queue_rhi_{};
    vk::DestructorQueue dtor_queue_swapchain_{};

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
    uint32_t                 swapchain_image_idx_{};
    std::vector<VkImage>     swapchain_images_{};
    std::vector<VkImageView> swapchain_image_views_{};
    VkFormat                 swapchain_image_format_{};
    VkQueue                  graphics_queue_{};
    uint32_t                 graphics_queue_family_{};
    VkDescriptorPool         imgui_pool_{};

    int frame_idx_ = 0;
    struct {
        VkCommandPool   command_pool{};
        VkCommandBuffer main_command_buffer{};
        VkFence         render_fence{};
        VkSemaphore     render_semaphore{};
        VkSemaphore     present_semaphore{};
    } frames_[kFramesInFlight] = {};

    struct {
        VkCommandPool   command_pool{};
        VkCommandBuffer command_buffer{};
        VkFence         upload_fence{};
    } upload_context_{};

public:
    VkDevice device() const { return device_; }

    int frame_idx() const { return frame_idx_; }

    const VkExtent2D& extent() const { return extent_; }

    VkFormat swapchain_image_format() const { return swapchain_image_format_; }

    uint32_t swapchain_image_idx() const { return swapchain_image_idx_; }

    const std::vector<VkImage>& swapchain_images() const {
        return swapchain_images_;
    }

    const std::vector<VkImageView>& swapchain_image_views() const {
        return swapchain_image_views_;
    }

    float max_sampler_anisotropy() const {
        return gpu_properties_.limits.maxSamplerAnisotropy;
    }

    VkCommandBuffer GetCurrentCommandBuffer() const {
        return frames_[frame_idx_].main_command_buffer;
    };

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

    void RecreateSwapchain();

    void PushDestructor(std::function<void()>&& destructor);

    void CreateImGuiContext(VkRenderPass render_pass, uint32_t subpass_idx);

    void DestroyImGuiContext();

    void ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func);

    void WaitForCurrentFrame();

    void WaitForAllFrames();

    void* MapMemory(vk::AllocatedBuffer* buffer);

    void UnmapMemory(vk::AllocatedBuffer* buffer);

    vk::AllocatedBuffer AllocateBuffer(size_t             alloc_size,    //
                                       VkBufferUsageFlags buffer_usage,  //
                                       VmaMemoryUsage     memory_usage);

    void DestroyBuffer(vk::AllocatedBuffer* buffer);

    void CopyBuffer(const void* src, vk::AllocatedBuffer* dst, size_t size,
                    size_t offset = 0);

    void CopyBuffer(const vk::AllocatedBuffer* src, vk::AllocatedBuffer* dst,
                    size_t size, size_t offset = 0);

    // Allocate VkImage & VkImageView, VkSampler is not accessed
    void AllocateTexture2D(vk::Texture* texture, vk::TextureCreateInfo* info);

    void AllocateTextureCubeMap(vk::Texture*           texture,
                                vk::TextureCreateInfo* info,
                                uint32_t               mip_levels);

    void DestroyTexture(vk::Texture* texture);

    bool BeginRenderCommand();

    bool EndRenderCommand();

    void CmdImageLayoutTransition(VkCommandBuffer    cmd,             //
                                  VkImage            image,           //
                                  VkImageAspectFlags aspect,          //
                                  VkImageLayout      old_layout,      //
                                  VkImageLayout      new_layout,      //
                                  uint32_t           mip_levels = 1,  //
                                  uint32_t           layers     = 1);

    void CmdCopyBufferToImage(VkCommandBuffer    cmd,     //
                              VkBuffer           buffer,  //
                              VkImage            image,   //
                              VkImageAspectFlags aspect,  //
                              uint32_t           width,   //
                              uint32_t           height,  //
                              uint32_t           layers = 1);

    void CmdGenerateMipMaps(VkCommandBuffer    cmd,         //
                            vk::Texture*       texture,     //
                            VkImageAspectFlags aspect,      //
                            uint32_t           mip_levels,  //
                            uint32_t           layers);

    size_t PaddedSizeOf(size_t size, size_t min_alignment) {
        if (min_alignment > 0) {
            size = (size + min_alignment - 1) & ~(min_alignment - 1);
        }
        return size;
    }

    size_t PaddedSizeOfUBO(size_t size) {
        return PaddedSizeOf(
            size, gpu_properties_.limits.minUniformBufferOffsetAlignment);
    }

    template <typename T>
    size_t PaddedSizeOfUBO() {
        return PaddedSizeOfUBO(sizeof(T));
    }

    size_t PaddedSizeOfSSBO(size_t size) {
        return PaddedSizeOf(
            size, gpu_properties_.limits.minStorageBufferOffsetAlignment);
    }

    template <typename T>
    size_t PaddedSizeOfSSBO() {
        return PaddedSizeOfSSBO(sizeof(T));
    }

private:
    void CreateVulkanInstance();

    void CreateSwapchain();

    void CreateCommands();

    void CreateSyncStructures();
};

}  // namespace lumi