#include "rhi.h"

namespace lumi {
void VulkanRHI::Init(InitInfo init_info) {
    InitVulkan(init_info.CreateSurface);
    InitSwapchain(init_info.width, init_info.height);
    InitCommands();
    InitDefaultRenderPass();
    InitFrameBuffers();
    InitSyncStructures();
    InitPipelines();
}

void VulkanRHI::InitVulkan(fCreateSurface CreateSurface) {
    vkb::InstanceBuilder builder;

    // make the Vulkan instance, with basic debug features
    auto vkb_inst = builder.set_app_name(LUMI_ENGINE_NAME)
                        .require_api_version(1, 1, 0)
                        .request_validation_layers(true)
                        .use_default_debug_messenger()
                        .build()
                        .value();
    instance_ = vkb_inst.instance;
    debug_messenger_ = vkb_inst.debug_messenger;

    // create the surface of the window
    VK_CHECK(CreateSurface(instance_, &surface_));

    // use vkbootstrap to select a GPU.
    // We want a GPU that can write to the surface and supports Vulkan 1.1
    VkPhysicalDeviceFeatures         required_features{};
    required_features.geometryShader = VK_TRUE;
    vkb::PhysicalDeviceSelector      selector{vkb_inst};
    std::vector<vkb::PhysicalDevice> physical_devices =
        selector.set_minimum_version(1, 1)
            .set_surface(surface_)
            .set_required_features(required_features)
            .select_devices()
            .value();
    LOG_ASSERT(physical_devices.size() > 0, "No suitable physical device found.");

    // Rating to choose best device
    int max_score = 0;
    int chosen_idx = -1;
    std::string physical_devices_info = "Found physical devices:";
    for (int i = 0; i < physical_devices.size(); i++) {
        int score = 0;
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
            max_score = score;
            chosen_idx = i;
        }
    }

    vkb::PhysicalDevice physicalDevice = physical_devices[chosen_idx]; 
    LOG_DEBUG(physical_devices_info.c_str());
    LOG_INFO("Selected physical device: {}", physicalDevice.name);

    // create the final Vulkan device
    vkb::DeviceBuilder deviceBuilder{physicalDevice};
    vkb::Device vkbDevice = deviceBuilder.build().value();
    device_          = vkbDevice.device;
    physical_device_ = physicalDevice.physical_device;

    // use vkbootstrap to get a Graphics queue
    graphics_queue_ = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphics_queue_family_ =
        vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    

    destruction_queue_default_.Push([=]() {
        vkDestroyDevice(device_, nullptr);
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkb::destroy_debug_utils_messenger(instance_, debug_messenger_);
        vkDestroyInstance(instance_, nullptr);
    });
}

void VulkanRHI::InitSwapchain(const int width, const int height) {
    vkb::SwapchainBuilder swapchainBuilder(physical_device_, device_, surface_);

    vkb::Swapchain vkbSwapchain =
        swapchainBuilder
            .use_default_format_selection()
            //use vsync present mode
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            .set_desired_min_image_count(
                vkb::SwapchainBuilder::DOUBLE_BUFFERING)
            .set_desired_extent(width, height)
            .build()
            .value();

    // store swapchain and its related images
    window_extent_.width    = width;
    window_extent_.height   = height;
    swapchain_              = vkbSwapchain.swapchain;
    swapchain_images_       = vkbSwapchain.get_images().value();
    swapchain_image_views_  = vkbSwapchain.get_image_views().value();
    swapchain_image_format_ = vkbSwapchain.image_format;

    destruction_queue_swapchain_.Push([=]() {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    });
}

void VulkanRHI::InitCommands() {
    // create a command pool for commands submitted to the graphics queue.
    auto commandPoolInfo = vk::BuildCommandPoolCreateInfo(
        graphics_queue_family_,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr,
                                 &command_pool_));

    // allocate the default command buffer that we will use for rendering
    auto cmdAllocInfo = vk::BuildCommandBufferAllocateInfo(command_pool_, 1);
    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdAllocInfo,
                                      &main_command_buffer_));

    destruction_queue_default_.Push(
        [=]() { vkDestroyCommandPool(device_, command_pool_, nullptr); });
}

void VulkanRHI::InitDefaultRenderPass() {
    // the renderpass will use this color attachment.
    VkAttachmentDescription color_attachment{};
    color_attachment.format         = swapchain_image_format_;
    color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // attachment number will index into the pAttachments array
    // in the parent renderpass itself
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // we are going to create 1 subpass, which is the minimum you can do
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_attachment_ref;

    // create render pass
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &color_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    VK_CHECK(
        vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_));

    destruction_queue_default_.Push(
        [=]() { vkDestroyRenderPass(device_, render_pass_, nullptr); });
}

void VulkanRHI::InitFrameBuffers() {
    // Create the framebuffers for the swapchain images. 
    // This will connect the render-pass to the images for rendering
    auto fb_info = vk::BuildFramebufferCreateInfo(render_pass_, window_extent_);

    // grab how many images we have in the swapchain
    const size_t swapchain_imagecount = swapchain_images_.size();
    frame_buffers_.resize(swapchain_imagecount);
    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {
        fb_info.pAttachments = &swapchain_image_views_[i];
        VK_CHECK(vkCreateFramebuffer(device_, &fb_info, nullptr,
                                     &frame_buffers_[i]));

        destruction_queue_swapchain_.Push([=]() {
            vkDestroyFramebuffer(device_, frame_buffers_[i], nullptr);
            vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);
        });
    }
}

void VulkanRHI::InitSyncStructures() {
    // Create synchronization structures
    // we want to create the fence with the Create Signaled flag,
    // so we can wait on it before using it on a GPU command (for the first frame)
    auto fenceCreateInfo =
        vk::BuildFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &render_fence_));

    // for the semaphores we don't need any flags
    auto semaphoreCreateInfo = vk::BuildSemaphoreCreateInfo();
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                               &present_semaphore_));
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                               &render_semaphore_));

    destruction_queue_default_.Push([=]() {
        vkDestroyFence(device_, render_fence_, nullptr);
        vkDestroySemaphore(device_, present_semaphore_, nullptr);
        vkDestroySemaphore(device_, render_semaphore_, nullptr);
    });
}

void VulkanRHI::InitPipelines() {
    VkShaderModule triangleFragShader;
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/triangle.frag.spv",
                          &triangleFragShader)) {
        LOG_ERROR("Error when building the triangle fragment shader module");
    } else {
        LOG_INFO("Triangle fragment shader successfully loaded");
    }

    VkShaderModule triangleVertexShader;
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/triangle.vert.spv",
                          &triangleVertexShader)) {
        LOG_ERROR("Error when building the triangle vertex shader module");

    } else {
        LOG_INFO("Triangle vertex shader successfully loaded");
    }

    // build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, 
    // so no need to use anything other than empty default
    auto pipeline_layout_info = vk::BuildPipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr,
                                    &triangle_pipeline_layout_));
    
    // build the stage-create-info for both vertex and fragment stages.
    // This lets the pipeline know the shader modules per stage
    vk::PipelineBuilder pipeline_builder{};

    pipeline_builder.shader_stages.push_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               triangleVertexShader));
    pipeline_builder.shader_stages.push_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                               triangleFragShader));
    // vertex input controls how to read vertices from vertex buffers.
    // We aren't using it yet
    pipeline_builder.vertex_input_info = vk::BuildVertexInputStateCreateInfo();
    // input assembly is the configuration for
    // drawing triangle lists, strips, or individual points.
    // we are just going to draw triangle list
    pipeline_builder.input_assembly =
        vk::BuildInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // build viewport and scissor from the swapchain extents
    pipeline_builder.viewport.x        = 0.0f;
    pipeline_builder.viewport.y        = 0.0f;
    pipeline_builder.viewport.width    = (float)window_extent_.width;
    pipeline_builder.viewport.height   = (float)window_extent_.height;
    pipeline_builder.viewport.minDepth = 0.0f;
    pipeline_builder.viewport.maxDepth = 1.0f;
    pipeline_builder.scissor.offset    = {0, 0};
    pipeline_builder.scissor.extent    = window_extent_;
    // configure the rasterizer to draw filled triangles
    pipeline_builder.rasterizer =
        vk::BuildRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    // we don't use multisampling, so just run the default one
    pipeline_builder.multisample = vk::BuildMultisampleStateCreateInfo();
    // a single blend attachment with no blending and writing to RGBA
    pipeline_builder.color_blend_attachment =
        vk::BuildColorBlendAttachmentState();
    // use the triangle layout we created
    pipeline_builder.pipeline_layout = triangle_pipeline_layout_;

    // finally build the pipeline
    triangle_pipeline_ = pipeline_builder.Build(device_, render_pass_);

    //destroy all shader modules, outside of the queue
    vkDestroyShaderModule(device_, triangleFragShader, nullptr);
    vkDestroyShaderModule(device_, triangleVertexShader, nullptr);

    destruction_queue_default_.Push([=]() {
        vkDestroyPipeline(device_, triangle_pipeline_, nullptr);
        vkDestroyPipelineLayout(device_, triangle_pipeline_layout_, nullptr);
    });
}

void VulkanRHI::Finalize() {
    // make sure the GPU has stopped doing its things
    vkWaitForFences(device_, 1, &render_fence_, true, kTimeout);
    destruction_queue_swapchain_.Flush();
    destruction_queue_default_.Flush();
}

void VulkanRHI::Draw() {
    static int _frame_number = 0;

    // wait until the GPU has finished rendering the last frame.
    VK_CHECK(vkWaitForFences(device_, 1, &render_fence_, true, kTimeout));
    VK_CHECK(vkResetFences(device_, 1, &render_fence_));

    // request image from the swapchain
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device_, swapchain_, kTimeout,
                                   present_semaphore_, nullptr,
                                   &swapchainImageIndex));

    // now that we are sure that the commands finished executing, 
    // we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetCommandBuffer(main_command_buffer_, 0));

    // begin the command buffer recording. 
    // We will use this command buffer exactly once, 
    // so we want to let Vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext            = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(main_command_buffer_, &cmdBeginInfo));

    //make a clear-color from frame number. This will flash with a 120*pi frame period.
    VkClearValue clearValue;
    float        flash = std::abs(std::sin(_frame_number / 120.f));
    clearValue.color   = {{flash, flash, flash, 1.0f}};

    // start the main renderpass.
    // We will use the clear color from above, 
    // and the framebuffer of the index the swapchain gave us
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext               = nullptr;
    rpInfo.renderPass          = render_pass_;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent   = window_extent_;
    rpInfo.framebuffer         = frame_buffers_[swapchainImageIndex];
    rpInfo.clearValueCount     = 1;
    rpInfo.pClearValues        = &clearValue;
    vkCmdBeginRenderPass(main_command_buffer_, &rpInfo,
                         VK_SUBPASS_CONTENTS_INLINE);


    vkCmdBindPipeline(main_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      triangle_pipeline_);
    vkCmdDraw(main_command_buffer_, 3, 1, 0, 0);


    vkCmdEndRenderPass(main_command_buffer_);
    VK_CHECK(vkEndCommandBuffer(main_command_buffer_));

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, 
    // as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished
    VkPipelineStageFlags waitStage =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext                = nullptr;
    submit.pWaitDstStageMask    = &waitStage;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = &present_semaphore_;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &render_semaphore_;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &main_command_buffer_;
    // submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(graphics_queue_, 1, &submit, render_fence_));

    // this will put the image we just rendered into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as it's necessary that drawing commands have finished 
    // before the image is displayed to the user
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = &swapchain_;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = &render_semaphore_;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(graphics_queue_, &presentInfo));

    // increase the number of frames drawn
    _frame_number++;
}

bool VulkanRHI::LoadShaderModule(const char*     filepath,
                                 VkShaderModule* out_shader_module) {
    auto shader_file =
        std::ifstream(filepath, std::ios::ate | std::ios::binary);
    if (!shader_file.is_open()) {
        return false;
    }
    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)shader_file.tellg();
    // spirv expects the buffer to be on uint32,
    // so make sure to reserve an int vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    // put file cursor at beginning
    shader_file.seekg(0);
    // load the entire file into the buffer
    shader_file.read((char*)buffer.data(), fileSize);
    // now that the file is loaded into the buffer, we can close it
    shader_file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    //codeSize has to be in bytes, so multiply the ints in the buffer by size of int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode    = buffer.data();

    //check that the creation goes well.
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {
        return false;
    }
    *out_shader_module = shaderModule;
    return true;
}

}  // namespace lumi