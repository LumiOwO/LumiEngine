#include <functional>

namespace lumi {

class ScopeGuard {
public:
    template <class Callable>
    ScopeGuard(Callable&& undo_func) try
        : f(std::forward<Callable>(undo_func)) {
    } catch (...) {
        undo_func();
        throw;
    }

    ScopeGuard(ScopeGuard&& other) noexcept : f(std::move(other.f)) {
        other.f = nullptr;
    }

    ~ScopeGuard() noexcept {
        if (f) f();
    }

    ScopeGuard(const ScopeGuard&) = delete;

    void operator=(const ScopeGuard&) = delete;

    void Dismiss() noexcept { f = nullptr; }

private:
    std::function<void()> f;
};

}  // namespace lumi