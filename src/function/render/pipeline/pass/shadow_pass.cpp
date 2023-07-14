#include "shadow_pass.h"

namespace lumi {

void ShadowPass::PreInit() {
    attachments_.directional_shadow_map_ = std::make_shared<vk::Texture>();
    resource->RegisterTexture(kDirectionalShadowMapName,
                              attachments_.directional_shadow_map_);

    directional_shadow_pass_ = std::make_shared<DirectionalShadowSubpass>(this);
}

void ShadowPass::PostInit() {
    directional_shadow_pass_->Init(kSubpassDirectional);
}

void ShadowPass::Finalize() {
    dtor_queue_shadow_.Flush();
}

void ShadowPass::CreateAttachmentImages() {
    auto extent = rhi->extent();

    vk::TextureCreateInfo info{};
    info.width       = kShadowMapSize;
    info.height      = kShadowMapSize;
    info.format      = VK_FORMAT_D32_SFLOAT;
    info.image_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                       VK_IMAGE_USAGE_SAMPLED_BIT;
    info.memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;
    info.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    info.sampler_name = "linear";
    rhi->AllocateTexture2D(attachments_.directional_shadow_map_.get(), &info);

    dtor_queue_shadow_.Push([this]() {
        rhi->DestroyTexture(attachments_.directional_shadow_map_.get());
    });
}

void ShadowPass::CreateRenderPass() {
    auto attachment_descs =
        std::vector<VkAttachmentDescription>(kAttachmentCount);
    auto attachment_refs = std::vector<VkAttachmentReference>(kAttachmentCount);
    // directional attachment
    {
        auto& desc          = attachment_descs[kAttachmentDirectional];
        desc.flags          = 0;
        desc.format         = attachments_.directional_shadow_map_->format;
        desc.samples        = VK_SAMPLE_COUNT_1_BIT;
        desc.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        desc.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        desc.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        desc.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        auto& ref      = attachment_refs[kAttachmentDirectional];
        ref.attachment = kAttachmentDirectional;
        ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    auto subpass_descs = std::vector<VkSubpassDescription>(kSubpassCount);
    auto subpass_dependencies = std::vector<VkSubpassDependency>();
    // directional pass
    {
        auto& desc                   = subpass_descs[kSubpassDirectional];
        desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        desc.colorAttachmentCount    = 0;
        desc.pColorAttachments       = nullptr;
        desc.pDepthStencilAttachment = &attachment_refs[kAttachmentDirectional];

        // Use subpass dependencies for layout transitions
        auto& write_dependency        = subpass_dependencies.emplace_back();
        write_dependency.srcSubpass   = VK_SUBPASS_EXTERNAL;
        write_dependency.dstSubpass   = kSubpassDirectional;
        write_dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        write_dependency.dstStageMask =
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        write_dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        write_dependency.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        write_dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        auto& read_dependency        = subpass_dependencies.emplace_back();
        read_dependency.srcSubpass   = kSubpassDirectional;
        read_dependency.dstSubpass   = VK_SUBPASS_EXTERNAL;
        read_dependency.srcStageMask =
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        read_dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        read_dependency.srcAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        read_dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        read_dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
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

    dtor_queue_shadow_.Push([this]() {
        vkDestroyRenderPass(rhi->device(), vk_render_pass_, nullptr);
    });
}

void ShadowPass::CreateFrameBuffers() {
    auto fb_info = vk::BuildFramebufferCreateInfo(
        vk_render_pass_, {kShadowMapSize, kShadowMapSize});

    std::vector<VkImageView> attachments = {
        attachments_.directional_shadow_map_->image.image_view,
    };
    fb_info.attachmentCount = (uint32_t)attachments.size();
    fb_info.pAttachments    = attachments.data();
    VK_CHECK(
        vkCreateFramebuffer(rhi->device(), &fb_info, nullptr, &framebuffer_));

    dtor_queue_shadow_.Push([this]() {
        vkDestroyFramebuffer(rhi->device(), framebuffer_, nullptr);
    });
}

void ShadowPass::SetClearValues() {
    auto& clear_depth              = clear_values_.emplace_back();
    clear_depth.depthStencil.depth = 1.f;
}

VkExtent2D ShadowPass::GetExtent() { return {kShadowMapSize, kShadowMapSize}; }

VkFramebuffer ShadowPass::GetFramebuffer() {
    return framebuffer_;
}

}  // namespace lumi