#include "present_pass.h"

namespace lumi {

void PresentPass::PreInit() {
    depth_attachment_ = std::make_shared<vk::Texture>();
    resource->RegisterTexture("_depth", depth_attachment_);

    mesh_lighting_pass_ = std::make_shared<MeshLightingSubpass>(this);

    skybox_pass_ = std::make_shared<SkyboxSubpass>(this);

    imgui_pass_ = std::make_shared<ImGuiSubpass>(this);
}

void PresentPass::PostInit() {
    resource->SetDefaultRenderPass(vk_render_pass_, kSubpassMeshLighting);

    mesh_lighting_pass_->Init(kSubpassMeshLighting);

    skybox_pass_->Init(kSubpassSkybox);

    imgui_pass_->Init(kSubpassImGui);
}

void PresentPass::Finalize() {
    dtor_queue_swapchain_.Flush();
    dtor_queue_present_.Flush();

    imgui_pass_->DestroyImGuiContext();
}

void PresentPass::CreateAttachmentImages() {
    auto extent = rhi->extent();

    vk::TextureCreateInfo info{};
    info.width        = extent.width;
    info.height       = extent.height;
    info.format       = VK_FORMAT_D32_SFLOAT;
    info.image_usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    rhi->AllocateTexture2D(depth_attachment_.get(), &info);

    dtor_queue_swapchain_.Push(
        [this]() { rhi->DestroyTexture(depth_attachment_.get()); });
}

void PresentPass::CreateRenderPass() {
    auto attachment_descs =
        std::vector<VkAttachmentDescription>(kAttachmentCount);
    auto attachment_refs = std::vector<VkAttachmentReference>(kAttachmentCount);
    // swapchain attachment
    {
        auto& desc          = attachment_descs[kAttachmentSwapchain];
        desc.flags          = 0;
        desc.format         = rhi->swapchain_image_format();
        desc.samples        = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        auto& ref      = attachment_refs[kAttachmentSwapchain];
        ref.attachment = kAttachmentSwapchain;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    // depth attachment
    {
        auto& desc          = attachment_descs[kAttachmentDepth];
        desc.flags          = 0;
        desc.format         = depth_attachment_->format;
        desc.samples        = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        auto& ref      = attachment_refs[kAttachmentDepth];
        ref.attachment = kAttachmentDepth;
        ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    auto subpass_descs = std::vector<VkSubpassDescription>(kSubpassCount);
    auto subpass_dependencies = std::vector<VkSubpassDependency>();
    // mesh lighting pass
    {
        auto& desc                   = subpass_descs[kSubpassMeshLighting];
        desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        desc.colorAttachmentCount    = 1;
        desc.pColorAttachments       = &attachment_refs[kAttachmentSwapchain];
        desc.pDepthStencilAttachment = &attachment_refs[kAttachmentDepth];

        // color subpass dependency
        auto& dependency        = subpass_dependencies.emplace_back();
        dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass   = kSubpassMeshLighting;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // depth subpass dependency
        auto& depth_dependency      = subpass_dependencies.emplace_back();
        depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        depth_dependency.dstSubpass = kSubpassMeshLighting;
        depth_dependency.srcStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.srcAccessMask = 0;
        depth_dependency.dstStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    // skybox pass
    {
        auto& desc                   = subpass_descs[kSubpassSkybox];
        desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        desc.colorAttachmentCount    = 1;
        desc.pColorAttachments       = &attachment_refs[kAttachmentSwapchain];
        desc.pDepthStencilAttachment = &attachment_refs[kAttachmentDepth];

        // color subpass dependency
        auto& dependency        = subpass_dependencies.emplace_back();
        dependency.srcSubpass   = kSubpassMeshLighting;
        dependency.dstSubpass   = kSubpassSkybox;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // depth subpass dependency
        auto& depth_dependency      = subpass_dependencies.emplace_back();
        depth_dependency.srcSubpass = kSubpassMeshLighting;
        depth_dependency.dstSubpass = kSubpassSkybox;
        depth_dependency.srcStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.srcAccessMask = 0;
        depth_dependency.dstStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        depth_dependency.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    // imgui pass
    {
        auto& desc                   = subpass_descs[kSubpassImGui];
        desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        desc.colorAttachmentCount    = 1;
        desc.pColorAttachments       = &attachment_refs[kAttachmentSwapchain];
        desc.pDepthStencilAttachment = nullptr;

        // color subpass dependency
        auto& dependency        = subpass_dependencies.emplace_back();
        dependency.srcSubpass   = kSubpassSkybox;
        dependency.dstSubpass   = kSubpassImGui;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }

    // create render pass
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = (uint32_t)attachment_descs.size();
    render_pass_info.pAttachments    = attachment_descs.data();
    render_pass_info.subpassCount    = (uint32_t)subpass_descs.size();
    render_pass_info.pSubpasses      = subpass_descs.data();
    render_pass_info.dependencyCount = (uint32_t)subpass_dependencies.size();
    render_pass_info.pDependencies   = subpass_dependencies.data();
    VK_CHECK(vkCreateRenderPass(rhi->device(), &render_pass_info, nullptr,
                                &vk_render_pass_));

    dtor_queue_present_.Push([this]() {
        vkDestroyRenderPass(rhi->device(), vk_render_pass_, nullptr);
    });
}

void PresentPass::CreateFrameBuffers() {
    auto fb_info =
        vk::BuildFramebufferCreateInfo(vk_render_pass_, rhi->extent());

    const size_t swapchain_imagecount = rhi->swapchain_images().size();
    framebuffers_.resize(swapchain_imagecount);

    for (size_t i = 0; i < swapchain_imagecount; i++) {
        std::vector<VkImageView> attachments = {
            rhi->swapchain_image_views()[i],
            depth_attachment_->image.image_view,
        };
        fb_info.attachmentCount = (uint32_t)attachments.size();
        fb_info.pAttachments    = attachments.data();
        VK_CHECK(vkCreateFramebuffer(rhi->device(), &fb_info, nullptr,
                                     &framebuffers_[i]));

        dtor_queue_swapchain_.Push([this, i]() {
            vkDestroyFramebuffer(rhi->device(), framebuffers_[i], nullptr);
        });
    }
}

void PresentPass::SetClearValues() {
    // screen clear value
    auto& clear_color = clear_values_.emplace_back();
    clear_color.color = {{0.047f, 0.047f, 0.047f, 1.0f}};

    // clear depth at 1
    auto& clear_depth              = clear_values_.emplace_back();
    clear_depth.depthStencil.depth = 1.f;
}

VkExtent2D PresentPass::GetExtent() { return rhi->extent(); }

VkFramebuffer PresentPass::GetFramebuffer() {
    return framebuffers_[rhi->swapchain_image_idx()];
}

void PresentPass::RecreateSwapchain() {
    rhi->WaitForAllFrames();

    VkExtent2D extent = rhi->GetWindowExtent();
    if (extent.width == 0 || extent.height == 0) return;

    dtor_queue_swapchain_.Flush();

    rhi->RecreateSwapchain();
    CreateAttachmentImages();
    CreateFrameBuffers();
}

}  // namespace lumi