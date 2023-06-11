#pragma once

#include <chrono>
#include <memory>

#include "core/ISingleton.h"
#include "window.h"
#include "render/rhi.h"


namespace lumi {

class Engine final : public ISingleton<Engine> {
private:
    std::chrono::steady_clock::time_point last_time_point_{
        std::chrono::steady_clock::now()};

    std::shared_ptr<Window> window_{};
    std::shared_ptr<VulkanRHI> rhi_{};

public:
    void Init();

    void Run();

    void Finalize();

private:
    void Tick(float dt);

    void TickLogic(float dt);

    void TickRender();
};

}  // namespace lumi