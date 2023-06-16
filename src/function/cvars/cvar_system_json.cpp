#include "cvar_system_private.h"

namespace lumi {

template <typename T>
Json CVarStorage<T>::ToJson() {
    Json res = Json::object();

    std::string_view name      = p_desc->name;
    size_t           start_pos = 0;

    // nesting
    Json* p_json = &res;
    do {
        size_t end_pos = name.find('.', start_pos);
        if (end_pos == name.npos) break;

        std::string_view level_name =
            name.substr(start_pos, end_pos - start_pos);
        Json& json = (*p_json)[level_name];
        json       = Json::object();
        p_json     = &json;
        start_pos  = end_pos + 1;
    } while (true);

    std::string_view level_name =
        name.substr(start_pos, name.size() - start_pos);
    Json& json = (*p_json)[level_name];

    const std::string& description =
        (p_desc->description != p_desc->name) ? p_desc->description : "";
    CVarFlags flags = p_desc->flags;

    if (description.empty() && flags == CVarFlags::kNone) {
        // primitive
        json = value;
    } else {
        // object
        json["#value"] = value;
        if (!description.empty()) {
            json["#description"] = p_desc->description;
        }
        if (flags != CVarFlags::kNone) {
            json["#flags"] = p_desc->flags;
        }
    }

    return res;
}

Json CVarSystem::ToJson() {
    Json res = Json::object();
    for (int32_t i = 0; i < cvar_arrays_bool.cnt; i++) {
        Json json = cvar_arrays_bool.values[i].ToJson();
        UpdateNestingJsonCVar(res, json);
    }
    for (int32_t i = 0; i < cvar_arrays_int.cnt; i++) {
        Json json = cvar_arrays_int.values[i].ToJson();
        UpdateNestingJsonCVar(res, json);
    }
    for (int32_t i = 0; i < cvar_arrays_float.cnt; i++) {
        Json json = cvar_arrays_float.values[i].ToJson();
        UpdateNestingJsonCVar(res, json);
    }
    for (int32_t i = 0; i < cvar_arrays_string.cnt; i++) {
        Json json = cvar_arrays_string.values[i].ToJson();
        UpdateNestingJsonCVar(res, json);
    }
    return res;
}

void CVarSystem::UpdateNestingJsonCVar(Json& root, const Json& j_cvar) {
    // Note: Assume all cvars names are different
    LOG_ASSERT(j_cvar.is_object());

    Json*       p_json   = &root;
    const Json* p_j_cvar = &j_cvar;

    while (true) {
        Json& json  = (*p_json);
        auto& key   = p_j_cvar->begin().key();
        auto& value = p_j_cvar->begin().value();

        if (!json.contains(key)) {
            json[key] = value;
            break;
        }

        if (!json[key].is_object()) {
            Json j_value = json[key];
            json[key]    = {
                   {"#value", j_value},
            };
        }

        if (!value.is_object() || value.contains("#value")) {
            // leaf
            json[key]["#value"] = value;
            break;
        }
        // nesting object
        p_json   = &json[key];
        p_j_cvar = &value;
    }
}

void CVarSystem::CreateCVarsFromJson(const Json&        json,
                                     const std::string& prefix) {
    if (json.empty()) return;

    for (auto& [key, value] : json.items()) {
        std::string name = prefix.empty() ? key : prefix + "." + key;

        if (key.empty()) {
            LOG_WARNING("Prefix \"{}\" contains an empty key", prefix);
        } else if (key == "#value") {
            CreateCVarsFromJsonLeaf(json, prefix);
        } else if (key[0] == '#') {
            ;
        } else if (value.is_primitive()) {
            CreateCVarsFromJsonLeaf(value, name);
        } else if (value.is_object()) {
            // parse nesting objects
            CreateCVarsFromJson(value, name);
        } else {
            LOG_WARNING(
                "Ignore console variable \"{}\" due to invalid value type",
                name);
        }
    }
}

void CVarSystem::CreateCVarsFromJsonLeaf(const Json&        leaf,
                                         const std::string& name) {
    if (name.empty()) {
        LOG_WARNING("Ignore console variable \"\" due to empty name");
        return;
    }
    // Use value's json object for auto type convert
    const Json&        j_value = leaf.is_object() ? leaf["#value"] : leaf;
    const std::string& description =
        (leaf.is_object() && leaf.contains("#description"))
            ? leaf["#description"]
            : "";
    CVarFlags flags = (leaf.is_object() && leaf.contains("#flags"))
                          ? leaf["#flags"]
                          : CVarFlags::kNone;

    if (j_value.is_boolean()) {
        CreateCVar<cvars::BoolType>(name, j_value, description, flags);
    } else if (j_value.is_number_integer()) {
        CreateCVar<cvars::IntType>(name, j_value, description,  //
                                   flags);
    } else if (j_value.is_number_float()) {
        CreateCVar<cvars::FloatType>(name, j_value, description, flags);
    } else if (j_value.is_string()) {
        CreateCVar<cvars::StringType>(name, j_value, description, flags);
    } else {
        LOG_WARNING("Ignore console variable \"{}\" due to invalid value type",
                    name);
    }
};

}  // namespace lumi
