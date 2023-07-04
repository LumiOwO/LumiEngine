#include "user_input.h"

#include "window.h"
#include "function/cvars/cvar_system.h"
#include "function/render/render_scene.h"

namespace lumi {

UserInput::UserInput(std::shared_ptr<RenderScene> scene,
                     std::shared_ptr<Window>      window)
    : scene_(scene) {
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

void UserInput::Tick(float dt) {
    if (flythrough_mode_) {
        auto&   camera   = scene_->camera;
        Mat4x4f rotation = camera.rotation();
        Vec3f   right    = rotation[0];
        Vec3f   up       = rotation[1];
        Vec3f   forward  = rotation[2];

        float speed = cvars::GetFloat("view_speed.move").value() * 0.05f;
        if (forward_) {
            camera.position += speed * forward_ * forward;
        }
        if (up_) {
            camera.position += speed * up_ * up;
        }
        if (right_) {
            camera.position += speed * right_ * right;
        }
    }
}

void UserInput::OnKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        OnKeyPressed(key, scancode, mods);
    } else if (action == GLFW_RELEASE) {
        OnKeyRelease(key, scancode, mods);
    }

}

void UserInput::OnKeyPressed(int key, int scancode, int mods) {
    if (flythrough_mode_) {
        switch (key) {
            case GLFW_KEY_W:
                forward_ = 1;
                break;
            case GLFW_KEY_S:
                forward_ = -1;
                break;
            case GLFW_KEY_A:
                right_ = -1;
                break;
            case GLFW_KEY_D:
                right_ = 1;
                break;
            case GLFW_KEY_Q:
                up_ = 1;
                break;
            case GLFW_KEY_E:
                up_ = -1;
                break;
            default:
                break;
        }
    }
}

void UserInput::OnKeyRelease(int key, int scancode, int mods) {
    if (flythrough_mode_) {
        switch (key) {
            case GLFW_KEY_W:
                if (forward_ == 1) forward_ = 0;
                break;
            case GLFW_KEY_S:
                if (forward_ == -1) forward_ = 0;
                break;
            case GLFW_KEY_A:
                if (right_ == -1) right_ = 0;
                break;
            case GLFW_KEY_D:
                if (right_ == 1) right_ = 0;
                break;
            case GLFW_KEY_Q:
                if (up_ == 1) up_ = 0;
                break;
            case GLFW_KEY_E:
                if (up_ == -1) up_ = 0;
                break;
            default:
                break;
        }
    }
}

void UserInput::OnMouseButton(int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        OnMouseButtonPressed(button, mods);
    } else if (action == GLFW_RELEASE) {
        OnMouseButtonRelease(button, mods);
    }
}

void UserInput::OnMouseButtonPressed(int button, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        flythrough_mode_ = true;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        pan_mode_ = true;
    }
}

void UserInput::OnMouseButtonRelease(int button, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        flythrough_mode_ = false;
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        pan_mode_ = false;
    }
}

void UserInput::OnCursorPos(double xpos, double ypos) {
    //LOG_DEBUG("{}, {}", xpos, ypos);
    if (flythrough_mode_) {
        float speed  = cvars::GetFloat("view_speed.rotate").value();
        float y_deg  = float(xpos - last_x_) * 0.5f * speed;
        float x_deg  = float(ypos - last_y_) * 0.5f * speed;
        
        auto& camera = scene_->camera;
        camera.eulers_deg.x += x_deg;
        camera.eulers_deg.y += y_deg;
    }

    if (pan_mode_) {
        float speed = cvars::GetFloat("view_speed.pan").value();
        float dx    = float(last_x_ - xpos) * 0.04f * speed;
        float dy    = float(ypos - last_y_) * 0.04f * speed;

        auto& camera = scene_->camera;
        camera.position += Vec3f(dx, dy, 0);
    }

    last_x_ = xpos;
    last_y_ = ypos;
}

void UserInput::OnCursorEnter(int entered) {

}

void UserInput::OnScroll(double xoffset, double yoffset) {
    //LOG_DEBUG("{}, {}", xoffset, yoffset);
    if (flythrough_mode_) {
        auto  speed_var = cvars::GetFloat("view_speed.move");
        float new_speed = speed_var.value() + float(yoffset) * 0.125f;
        new_speed = std::clamp(new_speed, speed_var.min(), speed_var.max());
        speed_var.Set(new_speed);
    } else {
        auto& camera  = scene_->camera;
        Vec3f forward = camera.rotation()[2];
        camera.position += float(yoffset) *
                           cvars::GetFloat("view_speed.zoom").value() * forward;
    }
}

}  // namespace lumi