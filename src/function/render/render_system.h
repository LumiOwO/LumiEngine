#pragma once

#include "rhi/vulkan_rhi.h"
#include "render_scene.h"

namespace lumi {

class Window;

class RenderSystem {
private:
    std::shared_ptr<Window>      window_{};
    std::shared_ptr<VulkanRHI>   rhi_{};
    std::shared_ptr<RenderScene> scene_{};

public:
    void Init(std::shared_ptr<Window> window);

    void Tick();

    void Finalize();

    std::shared_ptr<RenderScene> scene() { return scene_; }
};

}  // namespace lumi