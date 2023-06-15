#include "user_input.h"
#include "function/cvars/cvar_system.h"

namespace lumi {
void UserInput::Init(std::shared_ptr<Window> window) {
    window->RegisterOnKeyFunc(  //
        [this](auto... params) { OnKey(params...); });
    window->RegisterOnMouseButtonFunc(
        [this](auto... params) { OnMouseButton(params...); });
    window->RegisterOnCursorPosFunc(
        [this](auto... params) { OnCursorPos(params...); });
    window->RegisterOnCursorEnterFunc(
        [this](auto... params) { OnCursorEnter(params...); });
    window->RegisterOnScrollFunc(
        [this](auto... params) { OnScroll(params...); });
}

void UserInput::OnKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        OnKeyPressed(key, scancode, mods);
    } else if (action == GLFW_RELEASE) {
        OnKeyRelease(key, scancode, mods);
    }
}

void UserInput::OnKeyPressed(int key, int scancode, int mods) {
    if (key == GLFW_KEY_SPACE) {
        auto colored = cvars::GetBool("colored");
        colored.Set(!colored.value());
    }
}

void UserInput::OnKeyRelease(int key, int scancode, int mods) {

}

void UserInput::OnMouseButton(int button, int action, int mods) {

}

void UserInput::OnCursorPos(double xpos, double ypos) {

}

void UserInput::OnCursorEnter(int entered) {

}

void UserInput::OnScroll(double xoffset, double yoffset) {

}

}  // namespace lumi