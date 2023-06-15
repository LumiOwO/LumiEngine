#pragma once

#include <chrono>
#include <memory>

#include "core/singleton.h"
#include "function/render/rhi.h"

#include "window.h"
#include "user_input.h"


namespace lumi {

class Engine final : public ISingleton<Engine> {
private:
    std::chrono::steady_clock::time_point last_time_point_{
        std::chrono::steady_clock::now()};

    std::shared_ptr<Window>    window_{};
    std::shared_ptr<VulkanRHI> rhi_{};
    std::shared_ptr<UserInput> user_input_{};

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
