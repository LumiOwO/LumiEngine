#pragma once

#include <memory>

#include "window.h"

namespace lumi {

class UserInput {
public:
    void Init(std::shared_ptr<Window> window);

private:
    void OnKey(int key, int scancode, int action, int mods);
    void OnKeyPressed(int key, int scancode, int mods);
    void OnKeyRelease(int key, int scancode, int mods);

    void OnMouseButton(int button, int action, int mods);

    void OnCursorPos(double xpos, double ypos);

    void OnCursorEnter(int entered);

    void OnScroll(double xoffset, double yoffset);
};

}  // namespace lumi
