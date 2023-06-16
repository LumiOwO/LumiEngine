#pragma once

#include <filesystem>
#include <fstream>

#include "log.h"

#pragma warning(push, 0)
#pragma warning(disable : 26819 28020 26495)
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
    auto in = std::ifstream(absolute_path);
    if (!in) return false;

    try {
        json = Json::parse(in,
                           /* callback */ nullptr,
                           /* allow exceptions */ true,
                           /* ignore_comments */ true);
    } catch (const std::exception& e) {
        LOG_ERROR(e.what());
        return false;
    }
    return true;
}

inline bool SaveJson(const Json &json, const fs::path& filepath) {
    auto absolute_path =
        filepath.is_absolute() ? filepath : LUMI_ASSETS_DIR / filepath;
    auto out = std::ofstream(absolute_path);
    if (!out) return false;

    try {
        out << json.dump(2);
    } catch (const std::exception& e) {
        LOG_ERROR(e.what());
        return false;
    }
    return true;
}

}  // namespace lumi
