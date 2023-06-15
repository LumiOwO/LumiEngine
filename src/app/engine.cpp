#include "engine.h"
#include "function/cvars/cvar_system.h"

namespace lumi {

void Engine::Init() {
    cvars::Init();

    // Init window
    window_ = std::make_shared<Window>();
    window_->Init();

    // Init RHI
    VulkanRHI::CreateInfo info{};
    info.CreateSurface = [=](VkInstance instance, VkSurfaceKHR* p_surface) {
        return window_->CreateSurface(instance, p_surface);
    };
    info.GetWindowExtent = [=]() {
        int width, height;
        window_->GetWindowSize(width, height);
        return VkExtent2D{(uint32_t)width, (uint32_t)height};
    };
    rhi_ = std::make_shared<VulkanRHI>();
    rhi_->Init(info);

    // Init user input
    user_input_ = std::make_shared<UserInput>();
    user_input_->Init(window_);
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
    rhi_->Render();
}

void Engine::Finalize() {
    rhi_->Finalize();
    rhi_ = nullptr;

    window_->Finalize();
    window_ = nullptr;

    cvars::SaveToDisk();
}
}  // namespace lumi