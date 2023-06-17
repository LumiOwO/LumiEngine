#pragma once

#include "function/render/vulkan_utils.h"
#include "GLFW/glfw3.h"

namespace lumi {

class Window {
public:
    using OnKeyFunc         = std::function<void(int, int, int, int)>;
    using OnMouseButtonFunc = std::function<void(int, int, int)>;
    using OnCursorPosFunc   = std::function<void(double, double)>;
    using OnCursorEnterFunc = std::function<void(int)>;
    using OnScrollFunc      = std::function<void(double, double)>;
    using OnDropFunc        = std::function<void(int, const char**)>;
    using OnWindowSizeFunc  = std::function<void(int, int)>;
    using OnWindowCloseFunc = std::function<void()>;

private:
    GLFWwindow* glfw_window_ = nullptr;

    std::vector<OnKeyFunc>         on_key_func_list_{};
    std::vector<OnMouseButtonFunc> on_mouse_button_func_list_{};
    std::vector<OnCursorPosFunc>   on_cursor_pos_func_list_{};
    std::vector<OnCursorEnterFunc> on_cursor_enter_func_list_{};
    std::vector<OnScrollFunc>      on_scroll_func_list_{};
    std::vector<OnDropFunc>        on_drop_func_list_{};
    std::vector<OnWindowSizeFunc>  on_window_size_func_list_{};
    std::vector<OnWindowCloseFunc> on_window_close_func_list_{};

public:
    void Init();

    void Tick();

    void Finalize();

    bool ShouldClose() const;

    VkResult CreateSurface(VkInstance instance, VkSurfaceKHR* p_surface);

    void GetWindowSize(int& width, int& height);

    void ImGuiWindowInit();
    void ImGuiWindowShutdown();
    void ImGuiWindowNewFrame();

    void RegisterOnKeyFunc(OnKeyFunc func) {
        on_key_func_list_.emplace_back(func);
    }
    void RegisterOnMouseButtonFunc(OnMouseButtonFunc func) {
        on_mouse_button_func_list_.emplace_back(func);
    }
    void RegisterOnCursorPosFunc(OnCursorPosFunc func) {
        on_cursor_pos_func_list_.emplace_back(func);
    }
    void RegisterOnCursorEnterFunc(OnCursorEnterFunc func) {
        on_cursor_enter_func_list_.emplace_back(func);
    }
    void RegisterOnScrollFunc(OnScrollFunc func) {
        on_scroll_func_list_.emplace_back(func);
    }
    void RegisterOnDropFunc(OnDropFunc func) {
        on_drop_func_list_.emplace_back(func);
    }
    void RegisterOnWindowSizeFunc(OnWindowSizeFunc func) {
        on_window_size_func_list_.emplace_back(func);
    }
    void RegisterOnWindowCloseFunc(OnWindowCloseFunc func) {
        on_window_close_func_list_.emplace_back(func);
    }

private:
    // window event callbacks
    static void KeyCallback(GLFWwindow* window, int key, int scancode,
                            int action, int mods) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_key_func_list_) {
            func(key, scancode, action, mods);
        }
    }
    static void MouseButtonCallback(GLFWwindow* window, int button, int action,
                                    int mods) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_mouse_button_func_list_) {
            func(button, action, mods);
        }
    }
    static void CursorPosCallback(GLFWwindow* window, double xpos,
                                  double ypos) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_cursor_pos_func_list_) {
            func(xpos, ypos);
        }
    }
    static void CursorEnterCallback(GLFWwindow* window, int entered) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_cursor_enter_func_list_) {
            func(entered);
        }
    }
    static void ScrollCallback(GLFWwindow* window, double xoffset,
                               double yoffset) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_scroll_func_list_) {
            func(xoffset, yoffset);
        }
    }
    static void DropCallback(GLFWwindow* window, int count,
                             const char** paths) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_drop_func_list_) {
            func(count, paths);
        }
    }
    static void WindowSizeCallback(GLFWwindow* window, int width, int height) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        for (auto& func : app->on_window_size_func_list_) {
            func(width, height);
        }
    }
    static void WindowCloseCallback(GLFWwindow* window) {
        Window* app = (Window*)glfwGetWindowUserPointer(window);
        if (!app) return;

        // callbacks before close
        for (auto& func : app->on_window_close_func_list_) {
            func();
        }
        // tell glfw that window should close
        glfwSetWindowShouldClose(window, true);
    }
};

}  // namespace lumi
