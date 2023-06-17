#pragma once

#include <string>
#include <cstdint>

#include "core/math.h"

namespace lumi {

enum CVarFlags {
    kNone     = 0,
    kReadOnly = 1 << 0,
    kAdvanced = 1 << 1,
    kIsUnit   = 1 << 2,
    kIsColor  = 1 << 3,
};

inline CVarFlags operator|(const CVarFlags& a, const CVarFlags& b) {
    return static_cast<CVarFlags>(uint32_t(a) | uint32_t(b));
}

inline CVarFlags& operator|=(CVarFlags& a, const CVarFlags& b) {
    return (a = a | b);
}

enum CVarType {
    kBool = 0,
    kInt,
    kFloat,
    kString,
    kVec2f,
    kVec3f,
    kVec4f,

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

using BoolType    = bool;
using IntType     = int32_t;
using FloatType   = float;
using StringType  = std::string;
using Vec2fType   = Vec2f;
using Vec3fType   = Vec3f;
using Vec4fType   = Vec4f;

}  // namespace cvars

using CVarBool   = CVar<cvars::BoolType>;
using CVarInt    = CVar<cvars::IntType>;
using CVarFloat  = CVar<cvars::FloatType>;
using CVarString = CVar<cvars::StringType>;
using CVarVec2f  = CVar<cvars::Vec2fType>;
using CVarVec3f  = CVar<cvars::Vec3fType>;
using CVarVec4f  = CVar<cvars::Vec4fType>;

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
CVarVec2f  CreateVec2f(const std::string_view& name, const Vec2fType& value,
                       const std::string_view& description = "",
                       CVarFlags               flags       = CVarFlags::kNone);
CVarVec3f  CreateVec3f(const std::string_view& name, const Vec3fType& value,
                       const std::string_view& description = "",
                       CVarFlags               flags       = CVarFlags::kNone);
CVarVec4f  CreateVec4f(const std::string_view& name, const Vec4fType& value,
                       const std::string_view& description = "",
                       CVarFlags               flags       = CVarFlags::kNone);

CVarBool   GetBool(const std::string_view& name);
CVarInt    GetInt(const std::string_view& name);
CVarFloat  GetFloat(const std::string_view& name);
CVarString GetString(const std::string_view& name);
CVarVec2f  GetVec2f(const std::string_view& name);
CVarVec3f  GetVec3f(const std::string_view& name);
CVarVec4f  GetVec4f(const std::string_view& name);

CVarBool   SetBool(const std::string_view& name, BoolType value);
CVarInt    SetInt(const std::string_view& name, IntType value);
CVarFloat  SetFloat(const std::string_view& name, FloatType value);
CVarString SetString(const std::string_view& name, const StringType& value);
CVarVec2f  SetVec2f(const std::string_view& name, const StringType& value);
CVarVec3f  SetVec3f(const std::string_view& name, const StringType& value);
CVarVec4f  SetVec4f(const std::string_view& name, const StringType& value);

const CVarDesc& GetCVarDesc(const std::string_view& name);

void ImGuiRender();

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

template <>
constexpr CVarType CVarVec2f::type() {
    return CVarType::kVec2f;
}

template <>
constexpr CVarType CVarVec3f::type() {
    return CVarType::kVec3f;
}

template <>
constexpr CVarType CVarVec4f::type() {
    return CVarType::kVec4f;
}

}  // namespace lumi