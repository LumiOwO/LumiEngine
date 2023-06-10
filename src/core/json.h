#pragma once

#include <filesystem>
#include <fstream>

#pragma warning(push, 0)
#pragma warning(disable : 26819 28020)
#define JSON_HAS_FILESYSTEM 1
#include "nlohmann/json.hpp"
#pragma warning(pop)

#define SERIALIZABLE_INTRUSIVE(Type, ...) \
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Type, __VA_ARGS__)
#define SERIALIZABLE_NON_INTRUSIVE(Type, ...) \
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__)

namespace lumi {

using Json   = nlohmann::json;
namespace fs = std::filesystem;

inline bool LoadJson(Json &json, const fs::path& filepath) {
    auto absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto in = std::ifstream(absolute_path.c_str());
    if (!in) return false;

    json = Json::parse(in);
    return true;
}

inline bool SaveJson(const Json &json, const fs::path& filepath) {
    auto absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto out = std::ofstream(absolute_path.c_str());
    if (!out) return false;

    out << json.dump(2);
    return true;
}

}  // namespace lumi
