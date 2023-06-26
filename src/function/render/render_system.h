#pragma once

#include "rhi/vulkan_rhi.h"
#include "render_scene.h"

namespace lumi {

class Window;

class RenderSystem {
private:
    std::shared_ptr<Window>      window_{};
    std::shared_ptr<VulkanRHI>   rhi_{};
    std::shared_ptr<RenderScene> render_scene_{};

public:
    RenderSystem(std::shared_ptr<Window> window) : window_(window) {}

    void Init();

    void Tick();

    void Finalize();
};

}  // namespace lumi