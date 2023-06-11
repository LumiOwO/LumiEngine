#include "engine.h"

namespace lumi {

void Engine::Init() {
    // Init window
    window_ = std::make_shared<Window>();
    window_->Init();

    // Init RHI
    rhi_ = std::make_shared<VulkanRHI>();
    VulkanRHI::InitInfo init_info{};
    window_->GetWindowSize(init_info.width, init_info.height);
    init_info.CreateSurface = [this](VkInstance    instance,
                                     VkSurfaceKHR* p_surface) {
        return window_->CreateSurface(instance, p_surface);
    };
    rhi_->Init(init_info);
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

void Engine::TickLogic(float dt) {}

void Engine::TickRender() {
    rhi_->Draw();
}

void Engine::Finalize() {
    rhi_->Finalize();
    rhi_ = nullptr;

    window_->Finalize();
    window_ = nullptr;
}
}  // namespace lumi