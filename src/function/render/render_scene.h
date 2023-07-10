#pragma once

#include "rhi/vulkan_rhi.h"
#include "render_resource.h"

namespace lumi {

struct Camera {
    Vec3f position   = Vec3f::kZero;
    Vec3f eulers_deg = Vec3f::kZero;
    float fovy_deg   = 70.f;
    float aspect     = 1700.f / 900.f;
    float near       = 0.1f;
    float far        = 200.0f;

    Mat4x4f rotation() const {
        return Mat4x4f::Rotation(ToRadians(eulers_deg));
    }

    Mat4x4f view() const {
        return rotation().Transpose() * Mat4x4f::Translation(-position);
    }

    Mat4x4f projection() const {
        return Mat4x4f::Perspective(ToRadians(fovy_deg), aspect, near, far);
    }
};

class RenderScene {
public:
    std::vector<RenderObject> renderables{};
    Camera                    camera{};

    std::shared_ptr<VulkanRHI>      rhi{};
    std::shared_ptr<RenderResource> resource{};

public:
    RenderScene(std::shared_ptr<VulkanRHI>      rhi,
                std::shared_ptr<RenderResource> resource)
        : rhi(rhi), resource(resource) {}

    void LoadScene();

    void UpdateVisibleObjects();

    void UploadPerFrameResource();
};

}  // namespace lumi