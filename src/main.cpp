#include "app/engine.h"

using namespace lumi;

int main() {
    auto& engine = Engine::Instance();

    try {
        engine.Init();
        engine.Run();
        engine.Finalize();
    } catch (const std::exception& e) {
        LOG_ERROR("Caught exception with message: {}", e.what());
    }

    return 0;
}
