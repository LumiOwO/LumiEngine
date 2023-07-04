#include "vulkan_rhi.h"

#include "core/scope_guard.h"
#include "function/cvars/cvar_system.h"

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace lumi {

void VulkanRHI::Init() {
    CreateVulkanInstance();
    CreateSwapchain();
    CreateCommands();
    CreateSyncStructures();
}

void VulkanRHI::CreateVulkanInstance() {
    vkb::InstanceBuilder builder;

    // make the Vulkan instance, with basic debug features
    auto&& vkb_inst = builder.set_app_name(LUMI_ENGINE_NAME)
                          .require_api_version(1, 3, 0)
#ifdef LUMI_ENABLE_DEBUG_LOG
                          .request_validation_layers(true)
                          .use_default_debug_messenger()
#endif
                          .build()
                          .value();
    instance_ = vkb_inst.instance;
#ifdef LUMI_ENABLE_DEBUG_LOG
    debug_messenger_ = vkb_inst.debug_messenger;
#endif

    // create the surface of the window
    VK_CHECK(CreateSurface(instance_, &surface_));

    // use vkbootstrap to select a GPU.
    VkPhysicalDeviceFeatures required_features{};
    required_features.geometryShader = VK_TRUE;
    vkb::PhysicalDeviceSelector      selector{vkb_inst};
    std::vector<vkb::PhysicalDevice> physical_devices =
        selector.set_minimum_version(1, 1)
            .set_surface(surface_)
            .set_required_features(required_features)
            .select_devices()
            .value();
    LOG_ASSERT(physical_devices.size() > 0,
               "No suitable physical device found");

    // Rating to choose best device
    int         max_score             = 0;
    int         chosen_idx            = -1;
    std::string physical_devices_info = "Found physical devices:";
    for (int i = 0; i < physical_devices.size(); i++) {
        int   score           = 0;
        auto& physical_device = physical_devices[i];

        // Discrete GPUs have a significant performance advantage
        if (physical_device.properties.deviceType ==
            VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += physical_device.properties.limits.maxImageDimension2D;

        physical_devices_info += "\n- ";
        physical_devices_info += physical_device.name;
        physical_devices_info += " (score = " + std::to_string(score) + ")";
        if (score > max_score) {
            max_score  = score;
            chosen_idx = i;
        }
    }

    vkb::PhysicalDevice physical_device = physical_devices[chosen_idx];
    LOG_DEBUG(physical_devices_info.c_str());
    LOG_INFO("Selected physical device: {}", physical_device.name);

    VkPhysicalDeviceShaderDrawParametersFeatures
        shader_draw_parameters_features{};
    shader_draw_parameters_features.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    shader_draw_parameters_features.pNext                = nullptr;
    shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;

    vkb::Device vkb_device = vkb::DeviceBuilder(physical_device)
                                 .add_pNext(&shader_draw_parameters_features)
                                 .build()
                                 .value();

    device_          = vkb_device.device;
    physical_device_ = physical_device.physical_device;
    gpu_properties_  = physical_device.properties;

    // use vkbootstrap to get a Graphics queue
    graphics_queue_ = vkb_device.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family_ =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physical_device_;
    allocatorInfo.device         = device_;
    allocatorInfo.instance       = instance_;
    vmaCreateAllocator(&allocatorInfo, &allocator_);

    PushDestructor([this]() {
        vmaDestroyAllocator(allocator_);
        vkDestroyDevice(device_, nullptr);
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
#ifdef LUMI_ENABLE_DEBUG_LOG
        vkb::destroy_debug_utils_messenger(instance_, debug_messenger_);
#endif
        vkDestroyInstance(instance_, nullptr);
    });
}

void VulkanRHI::CreateSwapchain() {
    VkExtent2D extent = GetWindowExtent();

    vkb::SwapchainBuilder swapchainBuilder(physical_device_, device_, surface_);
    vkb::Swapchain        vkbSwapchain =
        swapchainBuilder
            .use_default_format_selection()
            //use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            .set_desired_min_image_count(
                vkb::SwapchainBuilder::DOUBLE_BUFFERING)
            .set_desired_extent(extent.width, extent.height)
            .build()
            .value();
    LOG_DEBUG("Create swapchain with window extent ({}, {})", extent.width,
              extent.height);

    // store swapchain and its related images
    extent_                 = extent;
    swapchain_              = vkbSwapchain.swapchain;
    swapchain_images_       = vkbSwapchain.get_images().value();
    swapchain_image_views_  = vkbSwapchain.get_image_views().value();
    swapchain_image_format_ = vkbSwapchain.image_format;

    dtor_queue_swapchain_.Push([this]() {
        for (auto image_view : swapchain_image_views_) {
            vkDestroyImageView(device_, image_view, nullptr);
        }
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    });
}

void VulkanRHI::CreateCommands() {
    // create a command pool for commands submitted to the graphics queue.
    auto commandPoolInfo = vk::BuildCommandPoolCreateInfo(
        graphics_queue_family_,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (auto& frame : frames_) {
        VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr,
                                     &frame.command_pool));
        // allocate the default command buffer that we will use for rendering
        auto cmdAllocInfo =
            vk::BuildCommandBufferAllocateInfo(frame.command_pool, 1);
        VK_CHECK(vkAllocateCommandBuffers(device_, &cmdAllocInfo,
                                          &frame.main_command_buffer));

        PushDestructor([this, &frame]() {
            vkDestroyCommandPool(device_, frame.command_pool, nullptr);
        });
    }

    // create pool for upload context
    auto uploadCommandPoolInfo =
        vk::BuildCommandPoolCreateInfo(graphics_queue_family_);
    VK_CHECK(vkCreateCommandPool(device_, &uploadCommandPoolInfo, nullptr,
                                 &upload_context_.command_pool));
    // allocate the default command buffer that we will use for the instant commands
    auto uploadCmdAllocInfo =
        vk::BuildCommandBufferAllocateInfo(upload_context_.command_pool, 1);
    VK_CHECK(vkAllocateCommandBuffers(device_, &uploadCmdAllocInfo,
                                      &upload_context_.command_buffer));

    PushDestructor([this]() {
        vkDestroyCommandPool(device_, upload_context_.command_pool, nullptr);
    });
}

void VulkanRHI::CreateSyncStructures() {
    auto fenceCreateInfo =
        vk::BuildFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    auto semaphoreCreateInfo = vk::BuildSemaphoreCreateInfo();

    for (auto& frame : frames_) {
        VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr,
                               &frame.render_fence));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                                   &frame.present_semaphore));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                                   &frame.render_semaphore));

        PushDestructor([this, &frame]() {
            vkDestroyFence(device_, frame.render_fence, nullptr);
            vkDestroySemaphore(device_, frame.present_semaphore, nullptr);
            vkDestroySemaphore(device_, frame.render_semaphore, nullptr);
        });
    }

    auto uploadFenceCreateInfo = vk::BuildFenceCreateInfo();
    VK_CHECK(vkCreateFence(device_, &uploadFenceCreateInfo, nullptr,
                           &upload_context_.upload_fence));
    PushDestructor([this]() {
        vkDestroyFence(device_, upload_context_.upload_fence, nullptr);
    });
}

void VulkanRHI::Finalize() {
    dtor_queue_swapchain_.Flush();
    dtor_queue_rhi_.Flush();
}

void VulkanRHI::RecreateSwapchain() {
    dtor_queue_swapchain_.Flush();
    CreateSwapchain();
}

void VulkanRHI::PushDestructor(std::function<void()>&& destructor) {
    dtor_queue_rhi_.Push(std::move(destructor));
}

void VulkanRHI::ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func) {
    VkCommandBuffer cmd = upload_context_.command_buffer;

    auto cmdBeginInfo = vk::BuildCommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    func(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = vk::BuildSubmitInfo(&cmd);
    VK_CHECK(vkQueueSubmit(graphics_queue_, 1, &submit,
                           upload_context_.upload_fence));

    vkWaitForFences(device_, 1, &upload_context_.upload_fence, true, kTimeout);
    vkResetFences(device_, 1, &upload_context_.upload_fence);

    // reset the command buffers inside the command pool
    vkResetCommandPool(device_, upload_context_.command_pool, 0);
}


void VulkanRHI::WaitForCurrentFrame() {
    auto& cur_frame = frames_[frame_idx_];
    VK_CHECK(
        vkWaitForFences(device_, 1, &cur_frame.render_fence, true, kTimeout));
}

void VulkanRHI::WaitForAllFrames() {
    for (auto& frame : frames_) {
        VK_CHECK(
            vkWaitForFences(device_, 1, &frame.render_fence, true, kTimeout));
    }
}

void* VulkanRHI::MapMemory(vk::AllocatedBuffer* buffer) {
    void* data;
    vmaMapMemory(allocator_, buffer->allocation, &data);
    return data;
}

void VulkanRHI::UnmapMemory(vk::AllocatedBuffer* buffer) {
    vmaUnmapMemory(allocator_, buffer->allocation);
}

vk::AllocatedBuffer VulkanRHI::AllocateBuffer(
    size_t alloc_size, VkBufferUsageFlags buffer_usage,
    VmaMemoryUsage           memory_usage) {

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size  = alloc_size;
    bufferInfo.usage = buffer_usage;

    VmaAllocationCreateInfo vmaallocInfo{};
    vmaallocInfo.usage = memory_usage;

    vk::AllocatedBuffer new_buffer{};
    VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &vmaallocInfo,
                             &new_buffer.buffer, &new_buffer.allocation,
                             nullptr));

    return new_buffer;
}

void VulkanRHI::DestroyBuffer(vk::AllocatedBuffer* buffer) {
    vmaDestroyBuffer(allocator_, buffer->buffer, buffer->allocation);
}

void VulkanRHI::CopyBuffer(const void* src, vk::AllocatedBuffer* dst,
                           size_t size, size_t offset) {
    char* src_data = (char*)src;
    char* dst_data = (char*)MapMemory(dst);
    memcpy(dst_data + offset, src_data + offset, size);
    UnmapMemory(dst);
}

void VulkanRHI::CopyBuffer(const vk::AllocatedBuffer* src,
                           vk::AllocatedBuffer* dst, size_t size,
                           size_t offset) {
    ImmediateSubmit([src, dst, size, offset](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.srcOffset = offset;
        copy.dstOffset = offset;
        copy.size      = size;
        vkCmdCopyBuffer(cmd, src->buffer, dst->buffer, 1, &copy);
    });
}

void VulkanRHI::AllocateTexture2D(vk::Texture2D*           texture,
                                  vk::Texture2DCreateInfo* info) {

    texture->width  = info->width;
    texture->height = info->height;
    texture->format = info->format;

    // Create VkImage
    VkExtent3D image_extent{};
    image_extent.width  = info->width;
    image_extent.height = info->height;
    image_extent.depth  = 1;

    VkImageCreateInfo img_info =
        vk::BuildImageCreateInfo(info->format, info->image_usage, image_extent);

    VmaAllocationCreateInfo img_allocinfo{};
    img_allocinfo.usage         = info->memory_usage;

    VK_CHECK(vmaCreateImage(allocator_, &img_info, &img_allocinfo,
                            &texture->image.image, &texture->image.allocation,
                            nullptr));

    // Create VkImageView
    VkImageViewCreateInfo imageinfo = vk::BuildImageViewCreateInfo(
        texture->format, texture->image.image, info->aspect_flags);
    VK_CHECK(vkCreateImageView(device_, &imageinfo, nullptr,
                               &texture->image.image_view));
}

void VulkanRHI::DestroyTexture2D(vk::Texture2D* texture) {
    vkDestroyImageView(device_, texture->image.image_view, nullptr);
    vmaDestroyImage(allocator_, texture->image.image,
                    texture->image.allocation);
}

bool VulkanRHI::BeginRenderCommand() {
    WaitForCurrentFrame();
    auto& cur_frame = frames_[frame_idx_];

    // request image from the swapchain
    VkResult acquire_swapchain_image_result = vkAcquireNextImageKHR(
        device_, swapchain_, kTimeout, cur_frame.present_semaphore, nullptr,
        &swapchain_image_idx_);
    if (acquire_swapchain_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    } else {
        VK_CHECK(acquire_swapchain_image_result);
    }

    // now that we are sure that the commands finished executing,
    // we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetFences(device_, 1, &cur_frame.render_fence));
    VK_CHECK(vkResetCommandBuffer(cur_frame.main_command_buffer, 0));
    VkCommandBuffer cmd = cur_frame.main_command_buffer;

    auto cmdBeginInfo = vk::BuildCommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    return true;
}

bool VulkanRHI::EndRenderCommand() {
    auto& cur_frame = frames_[frame_idx_];

    VkCommandBuffer cmd = cur_frame.main_command_buffer;
    VK_CHECK(vkEndCommandBuffer(cmd));

    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext                = nullptr;
    submit.pWaitDstStageMask    = &waitStage;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &cur_frame.present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &cur_frame.render_semaphore;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &cmd;
    VK_CHECK(
        vkQueueSubmit(graphics_queue_, 1, &submit, cur_frame.render_fence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = &swapchain_;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = &cur_frame.render_semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &swapchain_image_idx_;

    VkResult queue_present_result =
        vkQueuePresentKHR(graphics_queue_, &presentInfo);
    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR ||
        queue_present_result == VK_SUBOPTIMAL_KHR) {
        return false;
    } else {
        VK_CHECK(queue_present_result);
    }

    // increase the number of frames drawn
    frame_idx_ = (frame_idx_ == kFramesInFlight - 1) ? 0 : frame_idx_ + 1;
    return true;
}

}  // namespace lumi