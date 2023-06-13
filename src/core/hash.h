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

}  // namespace lumi