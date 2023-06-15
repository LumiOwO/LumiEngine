#pragma once

#include <string>
#include <cstdint>

namespace lumi {

enum CVarFlags {
    kNone          = 0,
    kNoEdit        = 1 << 1,
    kEditReadOnly  = 1 << 2,
    kAdvanced      = 1 << 3,
    kEditCheckbox  = 1 << 8,
    kEditFloatDrag = 1 << 9,
};

enum CVarType {
    kBool = 0,
    kInt,
    kFloat,
    kString,

    kNumOfTypes,
};

struct CVarDesc {
    std::string name{};
    std::string description{};
    CVarFlags   flags{};
    CVarType    type{};
    int32_t     index_{};
};

template <typename T>
class CVar {
public:
    using Type = T;

    CVar(const std::string_view& name, const T& value,
         const std::string_view& description = "",
         CVarFlags               flags       = CVarFlags::kNone);

    explicit CVar(int32_t index) : index_(index) {}

    const CVarDesc& GetCVarDesc() const;

    CVar& Set(const T& value);

    constexpr static CVarType type();

    const T& value() const;

    T* ptr();

    const std::string& name() const;

    const std::string& description() const;

    CVarFlags flags() const;

private:
    int32_t index_ = -1;
};

namespace cvars {

using BoolType   = bool;
using IntType    = int32_t;
using FloatType  = float;
using StringType = std::string;

}  // namespace cvars

using CVarBool   = CVar<cvars::BoolType>;
using CVarInt    = CVar<cvars::IntType>;
using CVarFloat  = CVar<cvars::FloatType>;
using CVarString = CVar<cvars::StringType>;

namespace cvars {

void Init();

void SaveToDisk();

CVarBool   CreateBool(const std::string_view& name, BoolType value,
                      const std::string_view& description = "",
                      CVarFlags               flags       = CVarFlags::kNone);
CVarInt    CreateInt(const std::string_view& name, IntType value,
                     const std::string_view& description = "",
                     CVarFlags               flags       = CVarFlags::kNone);
CVarFloat  CreateFloat(const std::string_view& name, FloatType value,
                       const std::string_view& description = "",
                       CVarFlags               flags       = CVarFlags::kNone);
CVarString CreateString(const std::string_view& name, const StringType& value,
                        const std::string_view& description = "",
                        CVarFlags               flags       = CVarFlags::kNone);

CVarBool   GetBool(const std::string_view& name);
CVarInt    GetInt(const std::string_view& name);
CVarFloat  GetFloat(const std::string_view& name);
CVarString GetString(const std::string_view& name);

CVarBool   SetBool(const std::string_view& name, BoolType value);
CVarInt    SetInt(const std::string_view& name, IntType value);
CVarFloat  SetFloat(const std::string_view& name, FloatType value);
CVarString SetString(const std::string_view& name, const StringType& value);

const CVarDesc& GetCVarDesc(const std::string_view& name);

};  // namespace cvars

template <>
constexpr CVarType CVarBool::type() {
    return CVarType::kBool;
}

template <>
constexpr CVarType CVarInt::type() {
    return CVarType::kInt;
}

template <>
constexpr CVarType CVarFloat::type() {
    return CVarType::kFloat;
}

template <>
constexpr CVarType CVarString::type() {
    return CVarType::kString;
}

}  // namespace lumi