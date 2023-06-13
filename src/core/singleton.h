#pragma once

#include <type_traits>

namespace lumi {

template <typename T>
class ISingleton {
protected:
    ISingleton() = default;

public:
    virtual ~ISingleton() noexcept = default;

    ISingleton(const ISingleton&) = delete;
    ISingleton& operator=(const ISingleton&) = delete;

    static T& Instance() noexcept(std::is_nothrow_constructible<T>::value) {
        static T instance = T();
        return instance;
    }
};
}  // namespace lumi
