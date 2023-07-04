#include "engine.h"
#include "function/cvars/cvar_system.h"

namespace lumi {

void Engine::Init() {
    cvars::Init();

    // Init window
    window_ = std::make_shared<Window>();
    window_->Init();

    // Init render system
    render_system_ = std::make_shared<RenderSystem>();
    render_system_->Init(window_);

    // Init user input
    user_input_ = std::make_shared<UserInput>(render_system_->scene, window_);

}

void Engine::Run() {
    LOG_INFO("LumiEngine v{} starts", LUMI_VERSION);

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

void Engine::TickLogic(float dt) {
    user_input_->Tick(dt);
}

void Engine::TickRender() {
    render_system_->Tick();
}

void Engine::Finalize() {
    render_system_->Finalize();

    window_->Finalize();

    cvars::SaveToDisk();
}
}  // namespace lumi