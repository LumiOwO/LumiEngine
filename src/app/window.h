#pragma once

#include "render/vulkan_utils.h"
#include "GLFW/glfw3.h"

namespace lumi {

class Window {
private:
    GLFWwindow* glfw_window_ = nullptr;

public:
    void Init();

    void Tick();

    void Finalize();

    bool ShouldClose() const;

    VkResult CreateSurface(VkInstance instance, VkSurfaceKHR* p_surface);

    void GetWindowSize(int& width, int& height);
};

}  // namespace lumi
