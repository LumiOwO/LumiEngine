#include "render_system.h"

#include "app/window.h"
#include "pipeline/forward_pipeline.h"

namespace lumi {

void RenderSystem::Init(std::shared_ptr<Window> window) {
    rhi = std::make_shared<VulkanRHI>();

    rhi->CreateSurface = [window](VkInstance    instance,
                                   VkSurfaceKHR* p_surface) {
        return window->CreateSurface(instance, p_surface);
    };
    rhi->GetWindowExtent = [window]() {
        int width, height;
        window->GetWindowSize(width, height);
        return VkExtent2D{(uint32_t)width, (uint32_t)height};
    };
    rhi->ImGuiWindowInit     = [window]() { window->ImGuiWindowInit(); };
    rhi->ImGuiWindowShutdown = [window]() { window->ImGuiWindowShutdown(); };
    rhi->ImGuiWindowNewFrame = [window]() { window->ImGuiWindowNewFrame(); };
    rhi->Init();

    resource = std::make_shared<RenderResource>(rhi);
    resource->Init();

    pipeline = std::make_shared<ForwardPipeline>(rhi, resource);
    pipeline->Init();

    scene = std::make_shared<RenderScene>(rhi, resource);
    scene->LoadScene();
}

void RenderSystem::Tick() { 
    resource->ResetMappedPointers();

    scene->UpdateVisibleObjects();

    scene->UploadGlobalResource();

    pipeline->Render();
}

void RenderSystem::Finalize() {
    rhi->WaitForAllFrames();

    pipeline->Finalize();

    resource->Finalize();

    rhi->Finalize();
}

}  // namespace lumi