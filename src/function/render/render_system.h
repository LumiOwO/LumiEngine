#pragma once

#include "rhi/vulkan_rhi.h"
#include "pipeline/render_pipeline.h"
#include "render_resource.h"
#include "render_scene.h"

namespace lumi {

class Window;

class RenderSystem {
public:
    std::shared_ptr<VulkanRHI>      rhi{};
    std::shared_ptr<RenderResource> resource{};
    std::shared_ptr<RenderPipeline> pipeline{};
    std::shared_ptr<RenderScene>    scene{};

public:
    void Init(std::shared_ptr<Window> window);

    void Tick();

    void Finalize();
};

}  // namespace lumi