#pragma once

#include <cstdint>
#include <string>

namespace lumi {

// FNV-1a 32bit hashing algorithm.
constexpr uint32_t fnv1a_32(const char* s, size_t count) {
    return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) *
           16777619u;
}

constexpr size_t const_strlen(const char* s) {
    size_t size = 0;
    while (s[size]) {
        size++;
    };
    return size;
}

struct StringHash {
    uint32_t value = 0;

    constexpr StringHash(uint32_t hash) noexcept { value = hash; }

    constexpr StringHash(const char* s) noexcept {
        value = fnv1a_32(s, const_strlen(s));
    }
    constexpr StringHash(const char* s, size_t count) noexcept {
        value = fnv1a_32(s, count);
    }
    constexpr StringHash(std::string_view s) noexcept {
        value = fnv1a_32(s.data(), s.size());
    }

    StringHash(const StringHash& other) = default;

    constexpr operator uint32_t() noexcept { return value; }
};

template <class T>
inline void hash_combine(std::size_t& s, const T& v) {
    std::hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

}  // namespace lumi