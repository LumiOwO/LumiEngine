#include "rhi.h"
#include "function/cvars/cvar_system.h"

#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader/tiny_obj_loader.h"

namespace lumi {

bool LoadMeshFromObjFile(vk::Mesh& mesh, const fs::path& filepath) {
    auto absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto in = std::ifstream(absolute_path);

    // attrib will contain the vertex arrays of the file
    tinyobj::attrib_t attrib;
    // shapes contains the info for each separate object in the file
    std::vector<tinyobj::shape_t> shapes;
    // materials contains the information about the material of each shape, 
    // but we won't use it.
    std::vector<tinyobj::material_t> materials;

    //error and warning output from the load function
    std::string warn;
    std::string err;

    // load the OBJ file
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                     absolute_path.string().c_str(),
                     absolute_path.parent_path().string().c_str());
    // make sure to output the warnings to the console, 
    // in case there are issues with the file
    if (!warn.empty()) {
        LOG_WARNING(warn.c_str());
    }
    // if we have any error, print it to the console, and break the mesh loading.
    // This happens if the file can't be found or is malformed
    if (!err.empty()) {
        LOG_ERROR(err.c_str());
        return false;
    }

    // TODO: load shapes to index buffer
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            // hardcode loading to triangles
            int fv = 3;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // vertex position
                tinyobj::real_t vx = attrib.vertices[3LL * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3LL * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3LL * idx.vertex_index + 2];
                // vertex normal
                tinyobj::real_t nx = attrib.normals[3LL * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3LL * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3LL * idx.normal_index + 2];

                // copy it into our vertex
                vk::Vertex new_vert;
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;

                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;

                // we are setting the vertex color as the vertex normal. 
                // This is just for display purposes
                new_vert.color = new_vert.normal;

                mesh.vertices.emplace_back(new_vert);
            }
            index_offset += fv;
        }
    }

    return true;
}

void VulkanRHI::Init() {
    CreateVulkanInstance();
    CreateSwapchain();
    CreateCommands();
    CreateDefaultRenderPass();
    CreateFrameBuffers();
    CreateSyncStructures();
    CreatePipelines();

    ImGuiInit();

    LoadMeshes();
}

void VulkanRHI::CreateVulkanInstance() {
    vkb::InstanceBuilder builder;

    // make the Vulkan instance, with basic debug features
    auto vkb_inst = builder.set_app_name(LUMI_ENGINE_NAME)
                        .require_api_version(1, 1, 0)
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

    //initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo{};
    allocatorInfo.physicalDevice = physical_device_;
    allocatorInfo.device         = device_;
    allocatorInfo.instance       = instance_;
    vmaCreateAllocator(&allocatorInfo, &allocator_);
    
    destruction_queue_default_.Push([=]() {
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


    destruction_queue_swapchain_.Push([=]() {
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
    VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr,
                                 &command_pool_));
    // allocate the default command buffer that we will use for rendering
    auto cmdAllocInfo = vk::BuildCommandBufferAllocateInfo(command_pool_, 1);
    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdAllocInfo,
                                      &main_command_buffer_));

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

    destruction_queue_default_.Push([=]() {
        vkDestroyCommandPool(device_, upload_context_.command_pool, nullptr);
        vkDestroyCommandPool(device_, command_pool_, nullptr);
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
        [=]() { vkDestroyRenderPass(device_, render_pass_, nullptr); });
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

        destruction_queue_swapchain_.Push([=]() {
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
    VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &render_fence_));

    auto uploadFenceCreateInfo = vk::BuildFenceCreateInfo();
    VK_CHECK(vkCreateFence(device_, &uploadFenceCreateInfo, nullptr,
                           &upload_context_.upload_fence));

    // for the semaphores we don't need any flags
    auto semaphoreCreateInfo = vk::BuildSemaphoreCreateInfo();
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                               &present_semaphore_));
    VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr,
                               &render_semaphore_));

    destruction_queue_default_.Push([=]() {
        vkDestroyFence(device_, upload_context_.upload_fence, nullptr);
        vkDestroyFence(device_, render_fence_, nullptr);
        vkDestroySemaphore(device_, present_semaphore_, nullptr);
        vkDestroySemaphore(device_, render_semaphore_, nullptr);
    });
}

void VulkanRHI::CreatePipelines() {
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

    VkShaderModule meshTriangleFragShader;
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/meshtriangle.frag.spv",
                          &meshTriangleFragShader)) {
        LOG_ERROR("Error when building the triangle fragment shader module");
    } else {
        LOG_INFO("Mesh Triangle fragment shader successfully loaded");
    }

    VkShaderModule meshTriangleVertexShader;
    if (!LoadShaderModule(LUMI_SHADERS_DIR "/meshtriangle.vert.spv",
                          &meshTriangleVertexShader)) {
        LOG_ERROR("Error when building the triangle vertex shader module");

    } else {
        LOG_INFO("Mesh Triangle vertex shader successfully loaded");
    }

    // build the pipeline layout that controls the inputs/outputs of the shader
    // we are not using descriptor sets or other systems yet, 
    // so no need to use anything other than empty default
    auto pipeline_layout_info = vk::BuildPipelineLayoutCreateInfo();
    VK_CHECK(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr,
                                    &triangle_pipeline_layout_));

    auto mesh_pipeline_layout_info = vk::BuildPipelineLayoutCreateInfo();
    // setup push constants VkPushConstantRange push_constant;
    VkPushConstantRange push_constant;
    push_constant.offset = 0;
    push_constant.size = sizeof(vk::MeshPushConstants);
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    mesh_pipeline_layout_info.pPushConstantRanges    = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;
    VK_CHECK(vkCreatePipelineLayout(device_, &mesh_pipeline_layout_info,
                                    nullptr, &mesh_pipeline_layout_));
    
    // build the stage-create-info for both vertex and fragment stages.
    // This lets the pipeline know the shader modules per stage
    vk::PipelineBuilder pipeline_builder{};

    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               triangleVertexShader));
    pipeline_builder.shader_stages.emplace_back(
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
    
    // no need to init viewport & scissor
    // we use dynamic viewport
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
    pipeline_builder.dynamic_states.emplace_back(VK_DYNAMIC_STATE_SCISSOR);
    
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

    // default depthtesting
    pipeline_builder.depth_stencil =
        vk::BuildPipelineDepthStencilStateCreateInfo(
            true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

    // finally build the pipeline
    triangle_pipeline_ = pipeline_builder.Build(device_, render_pass_);

    // build the mesh triangle pipeline
    pipeline_builder.shader_stages.clear();
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT,
                                               meshTriangleVertexShader));
    pipeline_builder.shader_stages.emplace_back(
        vk::BuildPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT,
                                               meshTriangleFragShader));
    vk::VertexInputDescription vertexDescription =
        vk::GetVertexInputDescription();
    pipeline_builder.vertex_input_info.pVertexAttributeDescriptions =
        vertexDescription.attributes.data();
    pipeline_builder.vertex_input_info.vertexAttributeDescriptionCount =
        (uint32_t)vertexDescription.attributes.size();
    pipeline_builder.vertex_input_info.pVertexBindingDescriptions =
        vertexDescription.bindings.data();
    pipeline_builder.vertex_input_info.vertexBindingDescriptionCount =
        (uint32_t)vertexDescription.bindings.size();
    pipeline_builder.pipeline_layout = mesh_pipeline_layout_;
    mesh_pipeline_ = pipeline_builder.Build(device_, render_pass_);

    //destroy all shader modules, outside of the queue
    vkDestroyShaderModule(device_, triangleFragShader, nullptr);
    vkDestroyShaderModule(device_, triangleVertexShader, nullptr);
    vkDestroyShaderModule(device_, meshTriangleFragShader, nullptr);
    vkDestroyShaderModule(device_, meshTriangleVertexShader, nullptr);

    destruction_queue_default_.Push([=]() {
        vkDestroyPipeline(device_, triangle_pipeline_, nullptr);
        vkDestroyPipeline(device_, mesh_pipeline_, nullptr);
        vkDestroyPipelineLayout(device_, triangle_pipeline_layout_, nullptr);
        vkDestroyPipelineLayout(device_, mesh_pipeline_layout_, nullptr);
    });
}

void VulkanRHI::Finalize() {
    // make sure the GPU has stopped doing its things
    WaitForLastFrame();
    destruction_queue_swapchain_.Flush();
    destruction_queue_default_.Flush();
}

void VulkanRHI::RecreateSwapChain() {
    WaitForLastFrame();

    VkExtent2D extent = GetWindowExtent();
    if (extent.width == 0 || extent.height == 0) return;

    destruction_queue_swapchain_.Flush();
    CreateSwapchain();
    CreateFrameBuffers();
}

void VulkanRHI::Render() {
    // wait until the GPU has finished rendering the last frame.
    VK_CHECK(WaitForLastFrame());

    // request image from the swapchain
    uint32_t swapchainImageIndex;
    VkResult acquire_swapchain_image_result =
        vkAcquireNextImageKHR(device_, swapchain_, kTimeout, present_semaphore_,
                              nullptr, &swapchainImageIndex);
    if (acquire_swapchain_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else {
        VK_CHECK(acquire_swapchain_image_result);
    }

    // now that we are sure that the commands finished executing, 
    // we can safely reset the command buffer to begin recording again.
    VK_CHECK(vkResetFences(device_, 1, &render_fence_));
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

    std::vector<VkClearValue> clear_values{};
    // make a clear-color from frame number. This will flash with a 120*pi frame period.
    VkClearValue clearValue{};
    float        flash = std::abs(std::sin(frame_number_ / 120.f));
    clearValue.color   = {{flash, flash, flash, 1.0f}};
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
    vkCmdBeginRenderPass(main_command_buffer_, &rpInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    BindPipeline();
    
    RenderPass();

    GUIPass();

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
    frame_number_++;
}

void VulkanRHI::BindPipeline() {
    if (cvars::GetBool("colored").value()) {
        vkCmdBindPipeline(main_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          triangle_pipeline_);
    } else {
        vkCmdBindPipeline(main_command_buffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          mesh_pipeline_);
    }
    

    // set viewport & scissor when swapchain recreated
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (float)extent_.width;
    viewport.height   = (float)extent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(main_command_buffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent_;
    vkCmdSetScissor(main_command_buffer_, 0, 1, &scissor);
}

void VulkanRHI::RenderPass() { 
    if (cvars::GetBool("colored").value()) {
        vkCmdDraw(main_command_buffer_, 3, 1, 0, 0); 
    } else {
        // bind the mesh vertex buffer with offset 0
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(main_command_buffer_, 0, 1,
                               &triangle_mesh_.vertex_buffer.buffer, &offset);

        // make a model view matrix for rendering the object camera position
        Vec3f camPos = {0.f, 0.f, -2.f};

        Mat4x4f view = glm::translate(Mat4x4f(1.f), camPos);
        // camera projection
        Mat4x4f projection =
            glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
        projection[1][1] *= -1;
        // model rotation
        Mat4x4f model = glm::rotate(
            Mat4x4f{1.0f}, glm::radians(frame_number_ * 0.4f), Vec3f(0, 1, 0));
        // calculate final mesh matrix
        Mat4x4f mvp = projection * view * model;

        vk::MeshPushConstants constants{};
        constants.mvp = mvp;
        constants.color = Vec4f(cvars::GetVec3f("color").value(), 1.0f);

        // upload the matrix to the GPU via push constants
        vkCmdPushConstants(main_command_buffer_, mesh_pipeline_layout_,
                           VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(vk::MeshPushConstants), &constants);
        vkCmdDraw(main_command_buffer_,
                  (uint32_t)triangle_mesh_.vertices.size(), 1, 0, 0);
    }
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
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {
        return false;
    }
    *out_shader_module = shaderModule;
    return true;
}

VkResult VulkanRHI::WaitForLastFrame() {
    return vkWaitForFences(device_, 1, &render_fence_, true, kTimeout);
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

void VulkanRHI::LoadMeshes() {
    //// make the array 3 vertices long
    //triangle_mesh_.vertices.resize(3);

    //// vertex positions
    //triangle_mesh_.vertices[0].position = {1.f, 1.f, 0.0f};
    //triangle_mesh_.vertices[1].position = {-1.f, 1.f, 0.0f};
    //triangle_mesh_.vertices[2].position = {0.f, -1.f, 0.0f};

    //// vertex colors, all green
    //triangle_mesh_.vertices[0].color = {0.f, 1.f, 0.0f};  //pure green
    //triangle_mesh_.vertices[1].color = {0.f, 1.f, 0.0f};  //pure green
    //triangle_mesh_.vertices[2].color = {0.f, 1.f, 0.0f};  //pure green

    if (!LoadMeshFromObjFile(triangle_mesh_ , "models/monkey_smooth.obj")) {
        LOG_ERROR("Loading .obj file failed");
    }

    UploadMesh(triangle_mesh_);
}

void VulkanRHI::UploadMesh(vk::Mesh& vk_mesh) {
    // allocate vertex buffer
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size  = vk_mesh.vertices.size() * sizeof(vk::Vertex);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    // let the VMA library know that this data should be writeable by CPU,
    // but also readable by GPU
    VmaAllocationCreateInfo vmaAllocInfo{};
    vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    VK_CHECK(vmaCreateBuffer(allocator_, &bufferInfo, &vmaAllocInfo,
                             &vk_mesh.vertex_buffer.buffer,
                             &vk_mesh.vertex_buffer.allocation, nullptr));

    // add the destruction of triangle mesh buffer to the deletion queue
    destruction_queue_default_.Push([=]() {
        vmaDestroyBuffer(allocator_, vk_mesh.vertex_buffer.buffer,
                         vk_mesh.vertex_buffer.allocation);
    });

    // copy vertex data
    void* data;
    vmaMapMemory(allocator_, vk_mesh.vertex_buffer.allocation, &data);
    memcpy(data, vk_mesh.vertices.data(),
           vk_mesh.vertices.size() * sizeof(vk::Vertex));
    vmaUnmapMemory(allocator_, vk_mesh.vertex_buffer.allocation);
}

}  // namespace lumi