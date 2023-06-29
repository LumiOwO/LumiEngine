#include "render_system.h"
#include "app/window.h"

namespace lumi {

void RenderSystem::Init() {
    rhi_ = std::make_shared<VulkanRHI>();

    rhi_->CreateSurface = [this](VkInstance instance, VkSurfaceKHR* p_surface) {
        return window_->CreateSurface(instance, p_surface);
    };
    rhi_->GetWindowExtent = [this]() {
        int width, height;
        window_->GetWindowSize(width, height);
        return VkExtent2D{(uint32_t)width, (uint32_t)height};
    };
    rhi_->ImGuiWindowInit     = [this]() { window_->ImGuiWindowInit(); };
    rhi_->ImGuiWindowShutdown = [this]() { window_->ImGuiWindowShutdown(); };
    rhi_->ImGuiWindowNewFrame = [this]() { window_->ImGuiWindowNewFrame(); };

    rhi_->Init();

    render_scene_ = std::make_shared<RenderScene>(rhi_);
    render_scene_->LoadScene();
}

void RenderSystem::Tick() { 
    rhi_->Render(render_scene_->renderables); 
}

void RenderSystem::Finalize() {
    rhi_->Finalize();
    rhi_ = nullptr;

    window_ = nullptr;
}

}  // namespace lumi