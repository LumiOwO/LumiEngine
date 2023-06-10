#pragma once

#include <string>

struct GLFWwindow;

namespace lumi {

class Window {
private:
    GLFWwindow* glfw_window_ = nullptr;

public:
    void Init();

    void Tick();

    void Finalize();

    bool ShouldClose() const;
};

}  // namespace lumi
