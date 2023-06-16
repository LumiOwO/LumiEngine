#pragma warning(push, 0)
#pragma warning(disable : 4661)

#include "cvar_system_private.h"

#pragma warning(pop)

namespace lumi {

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
#pragma warning(push, 0)
#pragma warning(disable : 4661)

template struct CVarStorage<cvars::BoolType>;
template struct CVarStorage<cvars::IntType>;
template struct CVarStorage<cvars::FloatType>;
template struct CVarStorage<cvars::StringType>;

template class CVar<cvars::BoolType>;
template class CVar<cvars::IntType>;
template class CVar<cvars::FloatType>;
template class CVar<cvars::StringType>;

#pragma warning(pop)

}  // namespace lumi
