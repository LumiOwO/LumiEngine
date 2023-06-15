#include "cvar_system.h"

#include <memory>
#include <fstream>
#include <unordered_map>

#include "core/hash.h"
#include "core/json.h"
#include "core/singleton.h"

namespace lumi {

// storage for CVar
template <typename T>
struct CVarStorage {
    CVarDesc* p_desc{};
    T         value{};

    Json ToJson() {
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
};

struct CVarArrayBase {
    int32_t cnt = 0;
};

template <typename T, int32_t SIZE>
struct CVarArray : public CVarArrayBase {
    std::unique_ptr<CVarStorage<T>[]> values = nullptr;
    
    CVarArray() { values = std::make_unique<CVarStorage<T>[]>(SIZE); }

    const T& operator[](int32_t index) const { return Get(index); }

    const T& Get(int32_t index) const { return values[index].value; }

    T* GetPtr(int32_t index) { return &values[index].value; }

    CVarDesc* GetCVarDescPtr(int32_t index) { return values[index].p_desc; }

    void Set(int32_t index, const T& value) { values[index].value = value; }

    int Add(CVarDesc* p_desc, const T& value) {
        LOG_ASSERT(cnt < SIZE, "Adding {} to a full CVarArray", p_desc->name);

        int32_t index = cnt;
        cnt++;

        values[index].p_desc = p_desc;
        values[index].value  = value;
        p_desc->index_       = index;
        return index;
    }
};

struct CVarSystem : public ISingleton<CVarSystem> {
    constexpr static int kMaxCVarsCounts[CVarType::kNumOfTypes] = {
        500,  // kBool
        500,  // kInt
        500,  // kFloat
        200,  // kString
    };

    using CVarArrayBool =
        CVarArray<cvars::BoolType, kMaxCVarsCounts[CVarType::kBool]>;
    using CVarArrayInt =
        CVarArray<cvars::IntType, kMaxCVarsCounts[CVarType::kInt]>;
    using CVarArrayFloat =
        CVarArray<cvars::FloatType, kMaxCVarsCounts[CVarType::kFloat]>;
    using CVarArrayString =
        CVarArray<cvars::StringType, kMaxCVarsCounts[CVarType::kString]>;

    CVarArrayBool   cvar_arrays_bool   = CVarArrayBool();
    CVarArrayInt    cvar_arrays_int    = CVarArrayInt();
    CVarArrayFloat  cvar_arrays_float  = CVarArrayFloat();
    CVarArrayString cvar_arrays_string = CVarArrayString();

    CVarArrayBase* cvar_arrays_ptrs[CVarType::kNumOfTypes] = {
        &cvar_arrays_bool,
        &cvar_arrays_int,
        &cvar_arrays_float,
        &cvar_arrays_string,
    };

    std::unordered_map<uint32_t, CVarDesc> table{};

    // find by name
    template <typename T>
    CVar<T> CreateCVar(const std::string_view& name, const T& value,
                       const std::string_view& description, CVarFlags flags) {
        auto hash = StringHash(name);

        CVarDesc* p_found_desc = FindCVarDescInTable(hash);
        if (p_found_desc) {
            LOG_WARNING(
                "Console variable \"{}\" already exists. "
                "An invalid CVar handle will be returned\n"
                "This may caused by duplicate name or hash conflict",
                name);
            return CVar<T>(-1);
        }

        // Insert new CVarDesc
        CVarDesc& desc   = table[hash];
        desc.name        = name;
        desc.description = description.empty() ? name : description;
        desc.flags       = flags;
        desc.type        = CVar<T>::type();

        auto p_array = GetCVarArrayPtr<T>();
        p_array->Add(&desc, value);
        return CVar<T>(desc.index_);
    }

    template <typename T>
    CVar<T> GetCVar(const std::string_view& name) {
        CVarDesc* p_found_desc = FindCVarDescInTable(name);
        if (!p_found_desc) {
            LOG_WARNING(
                "Console variable \"{}\" does not exist. "
                "An invalid CVar handle will be returned\n",
                name);
            return CVar<T>(-1);
        }

        return CVar<T>(p_found_desc->index_);
    }

    template <typename T>
    CVar<T> SetCVar(const std::string_view& name, const T& value) {
        CVarDesc* p_found_desc = FindCVarDescInTable(name);
        if (!p_found_desc) {
            LOG_WARNING(
                "Console variable \"{}\" does not exist. "
                "An invalid CVar handle will be returned\n",
                name);
            return CVar<T>(-1);
        }

        int  index   = p_found_desc->index_;
        auto p_array = GetCVarArrayPtr<T>();
        p_array->Set(index, value);
        return CVar<T>(index);
    }

    CVarDesc* GetCVarDescPtr(const std::string_view& name) {
        return FindCVarDescInTable(name);
    }

    // find by type & index
    template <typename T>
    const T& GetValue(int32_t index) {
        auto p_array = GetCVarArrayPtr<T>();
        return p_array->Get(index);
    }

    template <typename T>
    T* GetPtr(int32_t index) {
        auto p_array = GetCVarArrayPtr<T>();
        return p_array->GetPtr(index);
    }

    template <typename T>
    void SetValue(int32_t index, const T& value) {
        auto p_array = GetCVarArrayPtr<T>();
        return p_array->Set(index, value);
    }

    template <typename T>
    CVarDesc* GetCVarDescPtr(int32_t index) {
        auto p_array = GetCVarArrayPtr<T>();
        return p_array->GetCVarDescPtr(index);
    }

    template <typename T>
    auto GetCVarArrayPtr() {
        constexpr int arr_idx = CVar<T>::type();
        using CVarArrayType   = CVarArray<T, kMaxCVarsCounts[arr_idx]>;
        CVarArrayType* p_array =
            static_cast<CVarArrayType*>(cvar_arrays_ptrs[arr_idx]);
        return p_array;
    }

    CVarDesc* FindCVarDescInTable(StringHash hash) {
        auto it = table.find(hash);
        if (it == table.end()) return nullptr;
        return &it->second;
    }

    Json ToJson() {
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

    void UpdateNestingJsonCVar(Json& root, const Json& j_cvar) {
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

    void CreateCVarsFromJson(const Json& json, const std::string& prefix = "") {
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

    void CreateCVarsFromJsonLeaf(const Json& leaf, const std::string& name) {
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
            LOG_WARNING(
                "Ignore console variable \"{}\" due to invalid value type",
                name);
        }
    };
};

// definitions of cvars namespace functions
namespace cvars {

void Init() {
    Json cvars_json{};
    if (!LoadJson(cvars_json, "cvars.json")) {
        LOG_ERROR("Failed to load console variables from "  //
                  LUMI_ASSETS_DIR "/cvars.json");
        exit(1);
    }

    auto& cvars_system = CVarSystem::Instance();
    cvars_system.CreateCVarsFromJson(cvars_json);

#ifdef LUMI_ENABLE_DEBUG_LOG
    std::string cvars_info{};
    cvars_info += "Loaded console variables:";

    std::vector<CVarDesc*> descs;
    for (auto& [hash, desc] : cvars_system.table) {
        descs.emplace_back(&desc);
    }
    std::sort(
        descs.begin(), descs.end(),
        [](const CVarDesc* a, const CVarDesc* b) { return a->name < b->name; });

    for (CVarDesc* desc : descs) {
        std::string value_string{};
        if (desc->type == CVarType::kBool) {
            auto p_array = cvars_system.GetCVarArrayPtr<BoolType>();
            value_string += p_array->Get(desc->index_) ? "true" : "false";
        } else if (desc->type == CVarType::kInt) {
            auto p_array = cvars_system.GetCVarArrayPtr<IntType>();
            value_string += std::to_string(p_array->Get(desc->index_));
        } else if (desc->type == CVarType::kFloat) {
            auto p_array = cvars_system.GetCVarArrayPtr<FloatType>();
            value_string += std::to_string(p_array->Get(desc->index_));
        } else if (desc->type == CVarType::kString) {
            auto p_array = cvars_system.GetCVarArrayPtr<StringType>();
            value_string += '\"'; 
            value_string += p_array->Get(desc->index_);
            value_string += '\"';
        }
        cvars_info += "\n- ";
        cvars_info += desc->name;
        cvars_info += " = ";
        cvars_info += value_string;
    }

    LOG_DEBUG(cvars_info.c_str());
#endif
}

void SaveToDisk() {
    Json cvars_json = CVarSystem::Instance().ToJson();
    if (!SaveJson(cvars_json, "cvars.json")) {
        LOG_WARNING("Failed to save console variables to "  //
                    LUMI_ASSETS_DIR "/cvars.json");
    }
}

CVarBool CreateBool(const std::string_view& name, BoolType value,
                    const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<BoolType>(name, value, description,
                                                       flags);
}

CVarInt CreateInt(const std::string_view& name, IntType value,
                  const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<IntType>(name, value, description,
                                                      flags);
}
CVarFloat CreateFloat(const std::string_view& name, FloatType value,
                      const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<FloatType>(name, value,
                                                        description, flags);
}

CVarString CreateString(const std::string_view& name, const StringType& value,
                        const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<StringType>(name, value,
                                                         description, flags);
}

CVarBool GetBool(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<BoolType>(name);
}

CVarInt GetInt(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<IntType>(name);
}

CVarFloat GetFloat(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<FloatType>(name);
}

CVarString GetString(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<StringType>(name);
}

CVarBool SetBool(const std::string_view& name, BoolType value) {
    return CVarSystem::Instance().SetCVar<BoolType>(name, value);
}

CVarInt SetInt(const std::string_view& name, IntType value) {
    return CVarSystem::Instance().SetCVar<IntType>(name, value);
}

CVarFloat SetFloat(const std::string_view& name, FloatType value) {
    return CVarSystem::Instance().SetCVar<FloatType>(name, value);
}

CVarString SetString(const std::string_view& name, const StringType& value) {
    return CVarSystem::Instance().SetCVar<StringType>(name, value);
}

const CVarDesc& GetCVarDesc(const std::string_view& name) {
    return *CVarSystem::Instance().GetCVarDescPtr(name);
}

}  // namespace cvars

// definitions of CVar<T> member functions
template<>
CVarBool::CVar(const std::string_view& name, const cvars::BoolType& value,
               const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateBool(name, value, description, flags);
}

template <>
CVarInt::CVar(const std::string_view& name, const cvars::IntType& value,
              const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateInt(name, value, description, flags);
}

template <>
CVarFloat::CVar(const std::string_view& name, const cvars::FloatType& value,
                const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateFloat(name, value, description, flags);
}

template <>
CVarString::CVar(const std::string_view& name, const cvars::StringType& value,
                 const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateString(name, value, description, flags);
}

template <typename T>
const CVarDesc& CVar<T>::GetCVarDesc() const {
    CVarDesc* p_desc = CVarSystem::Instance().GetCVarDescPtr<T>(index_);
    return *p_desc;
}

template <typename T>
CVar<T>& CVar<T>::Set(const T& value) {
    CVarSystem::Instance().SetValue<T>(index_, value);
    return *this;
}

template <typename T>
const T& CVar<T>::value() const {
    return CVarSystem::Instance().GetValue<T>(index_);
}

template <typename T>
T* CVar<T>::ptr() {
    return CVarSystem::Instance().GetPtr<T>(index_);
}

template <typename T>
const std::string& CVar<T>::name() const {
    CVarDesc* p_desc = CVarSystem::Instance().GetCVarDescPtr<T>(index_);
    return p_desc->name;
}

template <typename T>
const std::string& CVar<T>::description() const {
    CVarDesc* p_desc = CVarSystem::Instance().GetCVarDescPtr<T>(index_);
    return p_desc->description;
}

template <typename T>
CVarFlags CVar<T>::flags() const {
    CVarDesc* p_desc = CVarSystem::Instance().GetCVarDescPtr<T>(index_);
    return p_desc->flags;
}

// Explicitly instantiate only the classes you want to be defined.
template class CVar<cvars::BoolType>;
template class CVar<cvars::IntType>;
template class CVar<cvars::FloatType>;
template class CVar<cvars::StringType>;

}  // namespace lumi
