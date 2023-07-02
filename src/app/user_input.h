#pragma once

#include <memory>

namespace lumi {

class Window;
class RenderScene;

class UserInput {
private:
    std::shared_ptr<RenderScene> scene_{};

    bool   pan_mode_        = false;
    bool   flythrough_mode_ = false;
    int    forward_         = 0;
    int    up_              = 0;
    int    right_           = 0;
    double last_x_          = 0;
    double last_y_          = 0;

public:
    void Init(std::shared_ptr<RenderScene> scene,
              std::shared_ptr<Window>      window);

    void Tick(float dt);

private:
    void OnKey(int key, int scancode, int action, int mods);
    void OnKeyPressed(int key, int scancode, int mods);
    void OnKeyRelease(int key, int scancode, int mods);

    void OnMouseButton(int button, int action, int mods);
    void OnMouseButtonPressed(int button, int mods);
    void OnMouseButtonRelease(int button, int mods);

    void OnCursorPos(double xpos, double ypos);

    void OnCursorEnter(int entered);

    void OnScroll(double xoffset, double yoffset);
};

}  // namespace lumi
