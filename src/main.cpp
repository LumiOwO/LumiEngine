#include "app/engine.h"

using namespace lumi;

int main() {
    auto& engine = Engine::Instance();

    engine.Init();
    engine.Run();
    engine.Finalize();

    return 0;
}
