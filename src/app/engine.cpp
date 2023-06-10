#include "engine.h"

#include "core/log.h"

namespace lumi {

void Engine::Init() {
    // Init window
    window_ = std::make_unique<Window>();
    window_->Init();
}

void Engine::Run() {
    LOG_INFO("LumiEngine v{} starts.", LUMI_VERSION);

    // Main loop
    while (!window_->ShouldClose()) {
        // get current time
        using namespace std::chrono;
        steady_clock::time_point cur_time_point = steady_clock::now();

        // calculate delta time
        float dt =
            duration_cast<duration<float>>(cur_time_point - last_time_point_)
                .count();
        last_time_point_ = cur_time_point;

        Tick(dt);
    }
}

void Engine::Tick(float dt) {
    TickLogic(dt);
    TickRender();

    // handle window messages
    window_->Tick();
}

void Engine::TickLogic(float dt) {}

void Engine::TickRender() {}

void Engine::Finalize() {
    // finalize window
    window_->Finalize();
    window_ = nullptr;
}
}  // namespace lumi