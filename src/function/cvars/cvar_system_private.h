#pragma once

#include "cvar_system.h"

#include <fstream>
#include <memory>
#include <map>
#include <set>
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

    Json ToJson();
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

        auto& storage  = values[index];
        storage.p_desc = p_desc;
        storage.value  = value;

        p_desc->index_ = index;
        return index;
    }
};

struct CVarSystem : public ISingleton<CVarSystem> {
    constexpr static int kMaxCVarsCounts[CVarType::kNumOfTypes] = {
        100,  // kBool
        100,  // kInt
        100,  // kFloat
        50,   // kString
        30,   // kVec2f
        30,   // kVec3f
        30,   // kVec4f
    };

    using CVarArrayBool =
        CVarArray<cvars::BoolType, kMaxCVarsCounts[CVarType::kBool]>;
    using CVarArrayInt =
        CVarArray<cvars::IntType, kMaxCVarsCounts[CVarType::kInt]>;
    using CVarArrayFloat =
        CVarArray<cvars::FloatType, kMaxCVarsCounts[CVarType::kFloat]>;
    using CVarArrayString =
        CVarArray<cvars::StringType, kMaxCVarsCounts[CVarType::kString]>;
    using CVarArrayVec2f =
        CVarArray<cvars::Vec2fType, kMaxCVarsCounts[CVarType::kVec2f]>;
    using CVarArrayVec3f =
        CVarArray<cvars::Vec3fType, kMaxCVarsCounts[CVarType::kVec3f]>;
    using CVarArrayVec4f =
        CVarArray<cvars::Vec4fType, kMaxCVarsCounts[CVarType::kVec4f]>;

    CVarArrayBool   cvar_arrays_bool   = CVarArrayBool();
    CVarArrayInt    cvar_arrays_int    = CVarArrayInt();
    CVarArrayFloat  cvar_arrays_float  = CVarArrayFloat();
    CVarArrayString cvar_arrays_string = CVarArrayString();
    CVarArrayVec2f  cvar_arrays_vec2f  = CVarArrayVec2f();
    CVarArrayVec3f  cvar_arrays_vec3f  = CVarArrayVec3f();
    CVarArrayVec4f  cvar_arrays_vec4f  = CVarArrayVec4f();

    CVarArrayBase* cvar_arrays_ptrs[CVarType::kNumOfTypes] = {
        &cvar_arrays_bool,    //
        &cvar_arrays_int,     //
        &cvar_arrays_float,   //
        &cvar_arrays_string,  //
        &cvar_arrays_vec2f,   //
        &cvar_arrays_vec3f,   //
        &cvar_arrays_vec4f,   //
    };

    std::unordered_map<uint32_t, CVarDesc> table{};

    // ImGui context
    struct ImGuiCVarTreeNode {
        std::map<std::string, ImGuiCVarTreeNode> children{};
        std::string                              name{};
        std::set<CVarDesc*>                      descs{};
    };
    struct ImGuiContext {
        ImGuiCVarTreeNode cached_cvars_root{};
        std::string       search_text{};
        bool              show_advanced = false;
        bool              show_readonly = true;
        bool              inited        = false;
    };
    ImGuiContext imgui_ctx;

    // find by name
    template <typename T>
    CVar<T> CreateCVar(const std::string_view& name, const T& value,
                       const std::string_view& description, CVarFlags flags,
                       float min, float max) {
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
        desc.min         = min;
        desc.max         = max;

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

    Json ToJson();

    void UpdateNestingJsonCVar(Json& root, const Json& j_cvar);

    void CreateCVarsFromJson(const Json& json, const std::string& prefix = "");

    void CreateCVarsFromJsonLeaf(const Json& leaf, const std::string& name);
};
}