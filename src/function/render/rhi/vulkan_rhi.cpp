#include "vulkan_rhi.h"
#include "function/cvars/cvar_system.h"
#include "core/scope_guard.h"

// TODO: remove
#include "function/render/render_scene.h"

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
    CreateDefaultRenderPass();
    CreateFrameBuffers();
    CreateSyncStructures();

    CreateDescriptors();

    ImGuiInit();
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
    LOG_ASSERT(physical_devices.size() > 0, "No suitable physical device found");

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

    //initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physical_device_;
    allocatorInfo.device         = device_;
    allocatorInfo.instance       = instance_;
    vmaCreateAllocator(&allocatorInfo, &allocator_);

    destruction_queue_default_.Push([this]() {
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
    vkb::Swapchain vkbSwapchain =
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

    // depth image size will match the window
    VkExtent3D depthImageExtent = {extent.width, extent.height, 1};
    depth_format_               = VK_FORMAT_D32_SFLOAT;
    // the depth image will be an image with the format 
    // we selected and Depth Attachment usage flag
    VkImageCreateInfo dimg_info = vk::BuildImageCreateInfo(
        depth_format_, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        depthImageExtent);
    // for the depth image, we want to allocate it from GPU local memory
    VmaAllocationCreateInfo dimg_allocinfo{};
    dimg_allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    dimg_allocinfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    vmaCreateImage(allocator_, &dimg_info, &dimg_allocinfo, &depth_image_.image,
                   &depth_image_.allocation, nullptr);

    // build an image-view for the depth image to use for rendering
    VkImageViewCreateInfo dview_info = vk::BuildImageViewCreateInfo(
        depth_format_, depth_image_.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(
        vkCreateImageView(device_, &dview_info, nullptr, &depth_image_view_));


    destruction_queue_swapchain_.Push([this]() {
        vkDestroyImageView(device_, depth_image_view_, nullptr);
        vmaDestroyImage(allocator_, depth_image_.image,
                        depth_image_.allocation);
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

        destruction_queue_default_.Push([this, &frame]() {
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

    destruction_queue_default_.Push([this]() {
        vkDestroyCommandPool(device_, upload_context_.command_pool, nullptr);
    });
}

void VulkanRHI::CreateDefaultRenderPass() {
    std::vector<VkAttachmentDescription> attachments{};

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
    attachments.emplace_back(color_attachment);

    // Depth attachment
    VkAttachmentDescription depth_attachment{};
    depth_attachment.flags          = 0;
    depth_attachment.format         = depth_format_;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments.emplace_back(depth_attachment);

    // attachment number will index into the pAttachments array
    // in the parent renderpass itself
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // we are going to create 1 subpass, which is the minimum you can do
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    std::vector<VkSubpassDependency> dependencies{};
    // color subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.emplace_back(dependency);
    // depth subpass dependency
    VkSubpassDependency depth_dependency{};
    depth_dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
    depth_dependency.dstSubpass   = 0;
    depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.srcAccessMask = 0;
    depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depth_dependency.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies.emplace_back(depth_dependency);

    // create render pass
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = (uint32_t)attachments.size();
    render_pass_info.pAttachments    = attachments.data();
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = (uint32_t)dependencies.size();
    render_pass_info.pDependencies   = dependencies.data();
    VK_CHECK(
        vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_));

    destruction_queue_default_.Push(
        [this]() { vkDestroyRenderPass(device_, render_pass_, nullptr); });
}

void VulkanRHI::CreateFrameBuffers() {
    // Create the framebuffers for the swapchain images. 
    // This will connect the render-pass to the images for rendering
    auto fb_info = vk::BuildFramebufferCreateInfo(render_pass_, extent_);

    // grab how many images we have in the swapchain
    const size_t swapchain_imagecount = swapchain_images_.size();
    frame_buffers_.resize(swapchain_imagecount);
    // create framebuffers for each of the swapchain image views
    for (size_t i = 0; i < swapchain_imagecount; i++) {
        std::vector<VkImageView> attachments = {
            swapchain_image_views_[i],
            depth_image_view_,
        };
        fb_info.attachmentCount = (uint32_t)attachments.size();
        fb_info.pAttachments    = attachments.data();
        VK_CHECK(vkCreateFramebuffer(device_, &fb_info, nullptr,
                                     &frame_buffers_[i]));

        destruction_queue_swapchain_.Push([this, i]() {
            vkDestroyFramebuffer(device_, frame_buffers_[i], nullptr);
            vkDestroyImageView(device_, swapchain_image_views_[i], nullptr);
        });
    }
}

void VulkanRHI::CreateSyncStructures() {
    // Create synchronization structures
    // we want to create the fence with the Create Signaled flag,
    // so we can wait on it before using it on a GPU command (for the first frame)
    auto fenceCreateInfo =
        vk::BuildFenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);

    // for the semaphores we don't need any flags
    auto semaphoreCreateInfo = vk::BuildSemaphoreCreateInfo();

    for (auto& frame : frames_) {
        VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr,
                               &frame.render_fence));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                                   &frame.present_semaphore));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                                   &frame.render_semaphore));

        destruction_queue_default_.Push([this, &frame]() {
            vkDestroyFence(device_, frame.render_fence, nullptr);
            vkDestroySemaphore(device_, frame.present_semaphore, nullptr);
            vkDestroySemaphore(device_, frame.render_semaphore, nullptr);
        });
    }
    

    auto uploadFenceCreateInfo = vk::BuildFenceCreateInfo();
    VK_CHECK(vkCreateFence(device_, &uploadFenceCreateInfo, nullptr,
                           &upload_context_.upload_fence));
    destruction_queue_default_.Push([this]() {
        vkDestroyFence(device_, upload_context_.upload_fence, nullptr);
    });
}

void VulkanRHI::CreateDescriptors() {
    // create a descriptor pool that will hold 10 uniform buffers
    std::vector<VkDescriptorPoolSize> sizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.pNext         = nullptr;
    pool_info.flags         = 0;
    pool_info.maxSets       = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes    = sizes.data();
    vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_);

    std::vector<VkDescriptorSetLayoutBinding> bindings{};

    VkDescriptorSetLayoutBinding camBufferBinding =
        vk::BuildDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                            VK_SHADER_STAGE_VERTEX_BIT, 0);
    bindings.emplace_back(camBufferBinding);

    VkDescriptorSetLayoutBinding envLightingBind =
        vk::BuildDescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1);
    bindings.emplace_back(envLightingBind);

    VkDescriptorSetLayoutCreateInfo setinfo{};
    setinfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setinfo.pNext        = nullptr;
    setinfo.flags        = 0;
    setinfo.bindingCount = (uint32_t)bindings.size();
    setinfo.pBindings    = bindings.data();
    vkCreateDescriptorSetLayout(device_, &setinfo, nullptr,
                                &global_set_layout_);

    VkDescriptorSetLayoutBinding mesh_instance_bind =
        vk::BuildDescriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                            VK_SHADER_STAGE_VERTEX_BIT, 0);
    VkDescriptorSetLayoutCreateInfo set2info{};
    set2info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set2info.pNext        = nullptr;
    set2info.flags        = 0;
    set2info.bindingCount = 1;
    set2info.pBindings    = &mesh_instance_bind;
    vkCreateDescriptorSetLayout(device_, &set2info, nullptr,
                                &mesh_instance_set_layout_);

    VkDescriptorSetLayoutBinding textureBind =
        vk::BuildDescriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 0);

    VkDescriptorSetLayoutCreateInfo set3info{};
    set3info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    set3info.pNext        = nullptr;
    set3info.flags        = 0;
    set3info.bindingCount = 1;
    set3info.pBindings    = &textureBind;
    vkCreateDescriptorSetLayout(device_, &set3info, nullptr,
                                &single_texture_set_layout_);

    // add descriptor set layout to deletion queues
    destruction_queue_default_.Push([this]() {
        vkDestroyDescriptorSetLayout(device_, global_set_layout_, nullptr);
        vkDestroyDescriptorSetLayout(device_, mesh_instance_set_layout_,
                                     nullptr);
        vkDestroyDescriptorSetLayout(device_, single_texture_set_layout_,
                                     nullptr);
        vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    });

    env_lighting_buffer_ = CreateBuffer(
        kFramesInFlight * PaddedSizeOf<vk::EnvLightingData>(),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < kFramesInFlight; i++) {
        auto& frame = frames_[i];

        // Create buffers
        frame.camera_buffer = CreateBuffer(PaddedSizeOf<vk::CameraData>(),
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        constexpr int MAX_OBJECTS = 10000;
        frame.mesh_instance_buffer = CreateBuffer(
            PaddedSizeOf<vk::MeshInstanceData>() * MAX_OBJECTS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // allocate one descriptor set for each frame
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool     = descriptor_pool_;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &global_set_layout_;
        vkAllocateDescriptorSets(device_, &allocInfo, &frame.global_descriptor_set);

        // allocate the descriptor set that will point to object buffer
        VkDescriptorSetAllocateInfo mesh_set_alloc{};
        mesh_set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        mesh_set_alloc.pNext = nullptr;
        mesh_set_alloc.descriptorPool     = descriptor_pool_;
        mesh_set_alloc.descriptorSetCount = 1;
        mesh_set_alloc.pSetLayouts        = &mesh_instance_set_layout_;
        vkAllocateDescriptorSets(device_, &mesh_set_alloc,
                                 &frame.mesh_instance_descriptor_set);

        // informations about the buffer we want to point at in the descriptor
        std::vector<VkDescriptorBufferInfo> infos{};
        std::vector<VkWriteDescriptorSet>   writes{};
        {
            // camera
            VkDescriptorBufferInfo& info = infos.emplace_back();

            info.buffer = frame.camera_buffer.buffer;
            info.offset = 0;
            info.range  = sizeof(vk::CameraData);

            VkWriteDescriptorSet write = vk::BuildWriteDescriptorSet(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, frame.global_descriptor_set,
                &info, 0);
            writes.emplace_back(write);
        }
        {
            // env light
            VkDescriptorBufferInfo& info = infos.emplace_back();

            info.buffer = env_lighting_buffer_.buffer;
            info.offset = 0;
            info.range  = sizeof(vk::EnvLightingData);

            VkWriteDescriptorSet write = vk::BuildWriteDescriptorSet(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                frame.global_descriptor_set, &info, 1);
            writes.emplace_back(write);
        }
        {
            // mesh instance data
            VkDescriptorBufferInfo& info = infos.emplace_back();

            info.buffer = frame.mesh_instance_buffer.buffer;
            info.offset = 0;
            info.range  = sizeof(vk::MeshInstanceData) * MAX_OBJECTS;

            VkWriteDescriptorSet write = vk::BuildWriteDescriptorSet(
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                frame.mesh_instance_descriptor_set, &info, 0);
            writes.emplace_back(write);
        }

        vkUpdateDescriptorSets(device_, (uint32_t)writes.size(), writes.data(),
                               0, nullptr);
    }

}

vk::Material VulkanRHI::CreateMaterial(const std::string& name,
                                       VkImageView        temp) {
    vk::PipelineBuilder pipeline_builder{};

    // build the stage-create-info for both vertex and fragment stages.
    // This lets the pipeline know the shader modules per stage
    VkShaderModule vertex_shader{};
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/" + name + ".vert.spv",
                          &vertex_shader)) {
        LOG_ERROR("Error when building \"{}\" vertex shader module", name);
    }

    VkShaderModule fragment_shader{};
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/" + name + ".frag.spv",
                          &fragment_shader)) {
        LOG_ERROR("Error when building \"{}\" fragment shader module", name);
    }

    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               vertex_shader));
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                               fragment_shader));

    // setup push constants VkPushConstantRange push_constant;
    VkPushConstantRange push_constant{};
    push_constant.offset     = 0;
    push_constant.size       = sizeof(vk::MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayout> setLayouts{
        global_set_layout_,
        mesh_instance_set_layout_,
        single_texture_set_layout_,
    };

    // build the pipeline layout that controls the inputs/outputs of the shader
    auto pipeline_layout_info = vk::BuildPipelineLayoutCreateInfo();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant;
    pipeline_layout_info.setLayoutCount         = (uint32_t)setLayouts.size();
    pipeline_layout_info.pSetLayouts            = setLayouts.data();

    VkPipelineLayout pipeline_layout{};
    VK_CHECK(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr,
                                    &pipeline_layout));
    pipeline_builder.pipeline_layout = pipeline_layout;

    // vertex input controls how to read vertices from vertex buffers.
    // We aren't using it yet
    pipeline_builder.vertex_input_info = vk::BuildVertexInputStateCreateInfo();
    // input assembly is the configuration for
    // drawing triangle lists, strips, or individual points.
    // we are just going to draw triangle list
    pipeline_builder.input_assembly =
        vk::BuildInputAssemblyCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // no need to init viewport & scissor
    // we use dynamic viewport
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_CULL_MODE);

    // configure the rasterizer to draw filled triangles
    pipeline_builder.rasterizer =
        vk::BuildRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
    // we don't use multisampling, so just run the default one
    pipeline_builder.multisample = vk::BuildMultisampleStateCreateInfo();
    // a single blend attachment with no blending and writing to RGBA
    pipeline_builder.color_blend_attachment =
        vk::BuildColorBlendAttachmentState();

    // default depthtesting
    pipeline_builder.depth_stencil =
        vk::BuildPipelineDepthStencilStateCreateInfo(
            true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    vk::VertexInputDescription vertexDescription =
        vk::Vertex::GetVertexInputDescription();
    pipeline_builder.vertex_input_info.pVertexAttributeDescriptions =
        vertexDescription.attributes.data();
    pipeline_builder.vertex_input_info.vertexAttributeDescriptionCount =
        (uint32_t)vertexDescription.attributes.size();
    pipeline_builder.vertex_input_info.pVertexBindingDescriptions =
        vertexDescription.bindings.data();
    pipeline_builder.vertex_input_info.vertexBindingDescriptionCount =
        (uint32_t)vertexDescription.bindings.size();

    // Finally build pipeline
    VkPipeline pipeline = pipeline_builder.Build(device_, render_pass_);

    // create a sampler for the texture
    VkSamplerCreateInfo samplerInfo =
        vk::BuildSamplerCreateInfo(VK_FILTER_NEAREST);
    VkSampler blockySampler{};
    vkCreateSampler(device_, &samplerInfo, nullptr, &blockySampler);

    // destroy all shader modules, outside of the queue
    vkDestroyShaderModule(device_, vertex_shader, nullptr);
    vkDestroyShaderModule(device_, fragment_shader, nullptr);

    destruction_queue_default_.Push(
        [this, pipeline, pipeline_layout, blockySampler]() {
        vkDestroySampler(device_, blockySampler, nullptr);
        vkDestroyPipeline(device_, pipeline, nullptr);
        vkDestroyPipelineLayout(device_, pipeline_layout, nullptr);
    });

    // allocate the descriptor set for single-texture to use on the material
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext          = nullptr;
    allocInfo.descriptorPool = descriptor_pool_;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &single_texture_set_layout_;
    VkDescriptorSet texture_set{};
    vkAllocateDescriptorSets(device_, &allocInfo, &texture_set);

    //write to the descriptor set so that it points to our empire_diffuse texture
    VkDescriptorImageInfo imageBufferInfo{};
    imageBufferInfo.sampler     = blockySampler;
    imageBufferInfo.imageView   = temp;
    imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet texture1 =
        vk::BuildWriteDescriptorSet(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    texture_set, &imageBufferInfo, 0);
    vkUpdateDescriptorSets(device_, 1, &texture1, 0, nullptr);

    vk::Material mat{};
    mat.pipeline        = pipeline;
    mat.pipeline_layout = pipeline_layout;
    mat.texture_set     = texture_set;
    return mat;
}

void VulkanRHI::Finalize() {
    // make sure the GPU has stopped doing its things
    for (auto& frame : frames_) {
        vkWaitForFences(device_, 1, &frame.render_fence, true, kTimeout);
    }
    destruction_queue_swapchain_.Flush();
    destruction_queue_default_.Flush();
}

void VulkanRHI::RecreateSwapChain() {
    WaitForCurrentFrame();

    VkExtent2D extent = GetWindowExtent();
    if (extent.width == 0 || extent.height == 0) return;

    destruction_queue_swapchain_.Flush();
    CreateSwapchain();
    CreateFrameBuffers();
}

void VulkanRHI::Render(std::shared_ptr<RenderScene> scene) {
    // wait until the GPU has finished rendering the last frame.
    VK_CHECK(WaitForCurrentFrame());
    auto& cur_frame = frames_[frame_idx_];

    // request image from the swapchain
    uint32_t swapchainImageIndex;
    VkResult acquire_swapchain_image_result = vkAcquireNextImageKHR(
        device_, swapchain_, kTimeout, cur_frame.present_semaphore, nullptr,
        &swapchainImageIndex);
    if (acquire_swapchain_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else {
        VK_CHECK(acquire_swapchain_image_result);
    }

    // now that we are sure that the commands finished executing, 
    // we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetFences(device_, 1, &cur_frame.render_fence));
    VK_CHECK(vkResetCommandBuffer(cur_frame.main_command_buffer, 0));
    VkCommandBuffer cmd_buffer = cur_frame.main_command_buffer;

    // begin the command buffer recording. 
    // We will use this command buffer exactly once, 
    // so we want to let Vulkan know that
    VkCommandBufferBeginInfo cmdBeginInfo{};
    cmdBeginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBeginInfo.pNext            = nullptr;
    cmdBeginInfo.pInheritanceInfo = nullptr;
    cmdBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd_buffer, &cmdBeginInfo));

    std::vector<VkClearValue> clear_values{};
    // screen clear value
    const Vec3f& clear_color = cvars::GetVec3f("clear_color").value();
    VkClearValue clearValue{};
    clearValue.color = {{clear_color.r, clear_color.g, clear_color.b, 1.0f}};
    clear_values.emplace_back(clearValue);

    // clear depth at 1
    VkClearValue depthClear{};
    depthClear.depthStencil.depth = 1.f;
    clear_values.emplace_back(depthClear);

    // start the main renderpass.
    // We will use the clear color from above, 
    // and the framebuffer of the index the swapchain gave us
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.pNext               = nullptr;
    rpInfo.renderPass          = render_pass_;
    rpInfo.renderArea.offset.x = 0;
    rpInfo.renderArea.offset.y = 0;
    rpInfo.renderArea.extent   = extent_;
    rpInfo.framebuffer         = frame_buffers_[swapchainImageIndex];
    rpInfo.clearValueCount     = (uint32_t)clear_values.size();
    rpInfo.pClearValues        = clear_values.data();
    vkCmdBeginRenderPass(cmd_buffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    RenderPass(cmd_buffer, scene);

    GUIPass(cmd_buffer);

    vkCmdEndRenderPass(cmd_buffer);
    VK_CHECK(vkEndCommandBuffer(cmd_buffer));


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
    submit.pWaitSemaphores      = &cur_frame.present_semaphore;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &cur_frame.render_semaphore;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &cmd_buffer;
    // submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(
        vkQueueSubmit(graphics_queue_, 1, &submit, cur_frame.render_fence));

    // this will put the image we just rendered into the visible window.
    // we want to wait on the _renderSemaphore for that,
    // as it's necessary that drawing commands have finished 
    // before the image is displayed to the user
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.pSwapchains        = &swapchain_;
    presentInfo.swapchainCount     = 1;
    presentInfo.pWaitSemaphores    = &cur_frame.render_semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices      = &swapchainImageIndex;
    
    VkResult queue_present_result =
        vkQueuePresentKHR(graphics_queue_, &presentInfo);
    if (queue_present_result == VK_ERROR_OUT_OF_DATE_KHR ||
        queue_present_result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapChain();
        return;
    } else {
        VK_CHECK(queue_present_result);
    }

    // increase the number of frames drawn
    frame_idx_ = (frame_idx_ == kFramesInFlight - 1) ? 0 : frame_idx_ + 1;
}

void VulkanRHI::CmdBindPipeline(VkCommandBuffer cmd_buffer,
                                vk::Material*   material) {
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      material->pipeline);

    // set viewport & scissor when swapchain recreated
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = (float)extent_.height;  // flip viewport for vulkan
    viewport.width    = (float)extent_.width;
    viewport.height   = -(float)extent_.height;  // flip viewport for vulkan
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent_;
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    vkCmdSetCullMode(cmd_buffer, VK_CULL_MODE_BACK_BIT);

    uint32_t uniform_offset =
        (uint32_t)PaddedSizeOf<vk::EnvLightingData>() * frame_idx_;
    vkCmdBindDescriptorSets(
        cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline_layout,
        0, 1, &frames_[frame_idx_].global_descriptor_set, 1, &uniform_offset);

    vkCmdBindDescriptorSets(
        cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline_layout,
        1, 1, &frames_[frame_idx_].mesh_instance_descriptor_set, 0, nullptr);

    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            material->pipeline_layout, 2, 1,
                            &material->texture_set, 0, nullptr);
}

void VulkanRHI::RenderPass(VkCommandBuffer              cmd_buffer,
                           std::shared_ptr<RenderScene> scene) { 
    Mat4x4f view = scene->camera.view();
    Mat4x4f projection = scene->camera.projection();

    auto& cam_allocation = frames_[frame_idx_].camera_buffer.allocation;
    vk::CameraData* cam_data = nullptr;
    vmaMapMemory(allocator_, cam_allocation, (void**)&cam_data);
    cam_data->view      = view;
    cam_data->proj      = projection;
    cam_data->proj_view = projection * view;
    vmaUnmapMemory(allocator_, cam_allocation);

    auto& env_allocation = env_lighting_buffer_.allocation;
    char* env_data = nullptr;
    vmaMapMemory(allocator_, env_allocation, (void**)&env_data);
    env_data += PaddedSizeOf<vk::EnvLightingData>() * frame_idx_;
    ((vk::EnvLightingData*)env_data)->ambient_color =
        Vec4f(cvars::GetVec3f("ambient").value(), 1.0f);
    vmaUnmapMemory(allocator_, env_allocation);

    auto& mesh_allocation = frames_[frame_idx_].mesh_instance_buffer.allocation;
    vk::MeshInstanceData* mesh_data = nullptr;
    vmaMapMemory(allocator_, mesh_allocation, (void**)&mesh_data);
    for (int i = 0; i < scene->renderables.size(); i++) {
        auto& object = scene->renderables[i];

        Mat4x4f model = Mat4x4f::Translation(object.position) *
                        object.rotation.ToMatrix() *
                        Mat4x4f::Scale(object.scale);
        mesh_data[i].model_matrix = model;
    }
    vmaUnmapMemory(allocator_, mesh_allocation);

    vk::Mesh*     lastMesh     = nullptr;
    vk::Material* lastMaterial = nullptr;
    for (int i = 0; i < scene->renderables.size(); i++) {
        auto& object = scene->renderables[i];

        if (object.mesh == nullptr) {
            continue;
        }
        // TODO: add unknown material (purple)
        if (object.material == nullptr) {
            continue;
        }

        // only bind the pipeline if it doesn't match with the already bound one
        if (object.material != lastMaterial) {
            CmdBindPipeline(cmd_buffer, object.material);
            lastMaterial = object.material;
        }

        vk::MeshPushConstants constants{};
        constants.model = {};
        constants.color = Vec4f(cvars::GetVec3f("clear_color").value(), 1.0f);

        // upload the mesh to the GPU via push constants
        vkCmdPushConstants(cmd_buffer, object.material->pipeline_layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(vk::MeshPushConstants), &constants);

        // only bind the mesh if it's a different one from last bind
        if (object.mesh != lastMesh) {
            // bind the mesh vertex buffer with offset 0
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd_buffer, 0, 1,
                                   &object.mesh->vertex_buffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd_buffer, object.mesh->index_buffer.buffer,
                                 0, vk::Mesh::kVkIndexType);
            lastMesh = object.mesh;
        }
        // we can now draw
        vkCmdDrawIndexed(cmd_buffer, (uint32_t)object.mesh->indices.size(), 1,
                         0, 0, i);
    }

}

bool VulkanRHI::LoadShaderModule(const std::string& filepath,
                                 VkShaderModule*    out_shader_module) {
    auto shader_file =
        std::ifstream(filepath, std::ios::ate | std::ios::binary);
    if (!shader_file.is_open()) {
        return false;
    }
    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    size_t fileSize = (size_t)shader_file.tellg();
    std::vector<char> buffer(fileSize);
    // put file cursor at beginning
    shader_file.seekg(0);
    // load the entire file into the buffer
    shader_file.read(buffer.data(), fileSize);
    // now that the file is loaded into the buffer, we can close it
    shader_file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.codeSize = buffer.size();
    createInfo.pCode    = (uint32_t*)buffer.data();

    //check that the creation goes well.
    VkShaderModule shaderModule{};
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {
        return false;
    }
    *out_shader_module = shaderModule;
    return true;
}

VkResult VulkanRHI::WaitForCurrentFrame() {
    auto& cur_frame = frames_[frame_idx_];
    return vkWaitForFences(device_, 1, &cur_frame.render_fence, true, kTimeout);
}

void VulkanRHI::ImmediateSubmit(std::function<void(VkCommandBuffer)>&& func) {
    VkCommandBuffer cmd = upload_context_.command_buffer;

    // begin the command buffer recording. 
    // We will use this command buffer exactly once before resetting
    VkCommandBufferBeginInfo cmdBeginInfo = vk::BuildCommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    func(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));

    // submit command buffer to the queue and execute it.
    // _uploadFence will now block until the graphic commands finish execution
    VkSubmitInfo submit = vk::BuildSubmitInfo(&cmd);
    VK_CHECK(vkQueueSubmit(graphics_queue_, 1, &submit,
                           upload_context_.upload_fence));

    vkWaitForFences(device_, 1, &upload_context_.upload_fence, true, kTimeout);
    vkResetFences(device_, 1, &upload_context_.upload_fence);

    // reset the command buffers inside the command pool
    vkResetCommandPool(device_, upload_context_.command_pool, 0);
}

void VulkanRHI::UploadMesh(vk::Mesh& mesh) {
    {
        // vertex buffer
        const size_t buffer_size = mesh.vertices.size() * sizeof(vk::Vertex);

        // Create staging buffer & dst buffer
        auto staging_buffer =
            CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VMA_MEMORY_USAGE_CPU_ONLY, false);
        ScopeGuard guard = [this, &staging_buffer]() {
            DestroyBuffer(staging_buffer);
        };
        mesh.vertex_buffer = CreateBuffer(  //
            buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        // data -> staging buffer
        CopyBuffer(mesh.vertices.data(), staging_buffer, buffer_size);
        // staging buffer -> dst buffer
        CopyBuffer(staging_buffer, mesh.vertex_buffer, buffer_size);
    }
    {
        // index buffer
        const size_t buffer_size =
            mesh.indices.size() * sizeof(vk::Mesh::IndexType);

        // Create staging buffer & dst buffer
        auto staging_buffer =
            CreateBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                         VMA_MEMORY_USAGE_CPU_ONLY, false);
        ScopeGuard guard = [this, &staging_buffer]() {
            DestroyBuffer(staging_buffer);
        };
        mesh.index_buffer = CreateBuffer(  //
            buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY);

        // data -> staging buffer
        CopyBuffer(mesh.indices.data(), staging_buffer, buffer_size);
        // staging buffer -> dst buffer
        CopyBuffer(staging_buffer, mesh.index_buffer, buffer_size);
    }
}

void VulkanRHI::UploadTexture(vk::Texture& texture, const void* pixels,
                              int width, int height, int channels) {
    texture.image = CreateImage2D(
        width, height, texture.format,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // allocate temporary buffer for holding texture data to upload
    VkDeviceSize        imageSize = (VkDeviceSize)width * height * channels;
    vk::AllocatedBuffer staging_buffer =
        CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VMA_MEMORY_USAGE_CPU_ONLY, false);
    ScopeGuard guard = [this, &staging_buffer]() {
        DestroyBuffer(staging_buffer);
    };
    // copy data to buffer
    CopyBuffer(pixels, staging_buffer, imageSize);

    ImmediateSubmit([&texture, width, height,
                     &staging_buffer](VkCommandBuffer cmd) {
        VkImageSubresourceRange range{};
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer{};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image            = texture.image.image;
        imageBarrier_toTransfer.subresourceRange = range;
        imageBarrier_toTransfer.srcAccessMask    = 0;
        imageBarrier_toTransfer.dstAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;

        // barrier the image into the transfer-receive layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &imageBarrier_toTransfer);

        VkExtent3D imageExtent{};
        imageExtent.width  = (uint32_t)width;
        imageExtent.height = (uint32_t)height;
        imageExtent.depth  = 1;

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = imageExtent;

        // copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, staging_buffer.buffer, texture.image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &copyRegion);

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
        imageBarrier_toReadable.oldLayout =
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout =
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        // barrier the image into the shader readable layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
    });

    VkImageViewCreateInfo imageinfo = vk::BuildImageViewCreateInfo(
        texture.format, texture.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(device_, &imageinfo, nullptr,
                              &texture.image_view));
    destruction_queue_default_.Push([this, image_view = texture.image_view]() {
        vkDestroyImageView(device_, image_view, nullptr);
    });
}

vk::AllocatedBuffer VulkanRHI::CreateBuffer(size_t             alloc_size,
                                            VkBufferUsageFlags buffer_usage,
                                            VmaMemoryUsage     memory_usage,
                                            bool               auto_destroy) {
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

    if (auto_destroy) {
        destruction_queue_default_.Push([this, new_buffer]() {
            DestroyBuffer(new_buffer);
        });
    }
    return new_buffer;
}

void VulkanRHI::DestroyBuffer(vk::AllocatedBuffer buffer) {
    vmaDestroyBuffer(allocator_, buffer.buffer, buffer.allocation);
}

void VulkanRHI::CopyBuffer(const void* src, vk::AllocatedBuffer dst,
                           size_t size) {
    void* dst_data;
    vmaMapMemory(allocator_, dst.allocation, &dst_data);
    memcpy(dst_data, src, size);
    vmaUnmapMemory(allocator_, dst.allocation);
}

void VulkanRHI::CopyBuffer(const vk::AllocatedBuffer src,
                           vk::AllocatedBuffer dst, size_t size) {
    ImmediateSubmit([&src, &dst, size](VkCommandBuffer cmd) {
        VkBufferCopy copy{};
        copy.srcOffset = 0;
        copy.dstOffset = 0;
        copy.size      = size;
        vkCmdCopyBuffer(cmd, src.buffer, dst.buffer, 1, &copy);
    });
}

vk::AllocatedImage VulkanRHI::CreateImage2D(int               width,   //
                                            int               height,  //
                                            VkFormat          image_format,
                                            VkImageUsageFlags image_usage,
                                            VmaMemoryUsage    memory_usage,
                                            bool              auto_destroy) {
    vk::AllocatedImage new_image{};

    VkExtent3D imageExtent{};
    imageExtent.width  = (uint32_t)width;
    imageExtent.height = (uint32_t)height;
    imageExtent.depth  = 1;

    VkImageCreateInfo img_info =
        vk::BuildImageCreateInfo(image_format, image_usage, imageExtent);

    VmaAllocationCreateInfo img_allocinfo{};
    img_allocinfo.usage = memory_usage;

    VK_CHECK(vmaCreateImage(allocator_, &img_info, &img_allocinfo,
                            &new_image.image, &new_image.allocation, nullptr));

    if (auto_destroy) {
        destruction_queue_default_.Push(
            [this, new_image]() { DestroyImage(new_image); });
    }
    return new_image;
}

void VulkanRHI::DestroyImage(vk::AllocatedImage image) {
    vmaDestroyImage(allocator_, image.image, image.allocation);
}

}  // namespace lumi