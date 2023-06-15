#include "window.h"

namespace lumi {

void Window::Init() {
    glfwInit();
    if (glfwVulkanSupported() == GLFW_FALSE) {
        LOG_ERROR("Vulkan is not supported for glfw");
        glfwTerminate();
        exit(1);
    }

    // Compute window width & height
    int count;
    int windowWidth, windowHeight;
    int monitorX, monitorY;

    GLFWmonitor**      monitors  = glfwGetMonitors(&count);
    const GLFWvidmode* videoMode = glfwGetVideoMode(monitors[0]);
    windowWidth                  = int(videoMode->width * 0.8);
    windowHeight                 = windowWidth / 16 * 9;
    glfwGetMonitorPos(monitors[0], &monitorX, &monitorY);

    // Set the visibility window hint to false for subsequent window creation
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // This tells GLFW to not create an OpenGL context with the window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Create window
    glfw_window_ = glfwCreateWindow(windowWidth, windowHeight, LUMI_ENGINE_NAME,
                                    nullptr, nullptr);
    if (glfw_window_ == nullptr) {
        LOG_ERROR("Failed to create GLFW window");
        glfwTerminate();
        exit(1);
    }

    // Setup input callbacks
    glfwSetWindowUserPointer(glfw_window_, this);
    glfwSetKeyCallback(glfw_window_, KeyCallback);
    glfwSetMouseButtonCallback(glfw_window_, MouseButtonCallback);
    glfwSetCursorPosCallback(glfw_window_, CursorPosCallback);
    glfwSetCursorEnterCallback(glfw_window_, CursorEnterCallback);
    glfwSetScrollCallback(glfw_window_, ScrollCallback);
    glfwSetDropCallback(glfw_window_, DropCallback);
    glfwSetWindowSizeCallback(glfw_window_, WindowSizeCallback);
    glfwSetWindowCloseCallback(glfw_window_, WindowCloseCallback);

    // Reset the window hints to default
    glfwDefaultWindowHints();
    glfwSetWindowPos(glfw_window_,
                     monitorX + (videoMode->width - windowWidth) / 2,
                     monitorY + (videoMode->height - windowHeight) / 2);

    // Show the window
    glfwShowWindow(glfw_window_);
}

void Window::Tick() {
    glfwPollEvents();
}

void Window::Finalize() {
    glfwDestroyWindow(glfw_window_);
    glfwTerminate();
}

bool Window::ShouldClose() const { return glfwWindowShouldClose(glfw_window_); }

VkResult Window::CreateSurface(VkInstance instance, VkSurfaceKHR* p_surface) {
    return glfwCreateWindowSurface(instance, glfw_window_, NULL, p_surface);
}

void Window::GetWindowSize(int& width, int& height) {
    glfwGetWindowSize(glfw_window_, &width, &height);
}

}  // namespace lumi