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

using CVarBool   = CVar<bool>;
using CVarInt    = CVar<int32_t>;
using CVarFloat  = CVar<float>;
using CVarString = CVar<std::string>;

namespace cvars {

void Init();

void SaveToDisk();

CVarBool   CreateBool(const std::string_view& name, bool value,
                      const std::string_view& description = "",
                      CVarFlags               flags       = CVarFlags::kNone);
CVarInt    CreateInt(const std::string_view& name, int32_t value,
                     const std::string_view& description = "",
                     CVarFlags               flags       = CVarFlags::kNone);
CVarFloat  CreateFloat(const std::string_view& name, float value,
                       const std::string_view& description = "",
                       CVarFlags               flags       = CVarFlags::kNone);
CVarString CreateString(const std::string_view& name, const std::string& value,
                        const std::string_view& description = "",
                        CVarFlags               flags       = CVarFlags::kNone);

CVarBool   GetBool(const std::string_view& name);
CVarInt    GetInt(const std::string_view& name);
CVarFloat  GetFloat(const std::string_view& name);
CVarString GetString(const std::string_view& name);

CVarBool   SetBool(const std::string_view& name, bool value);
CVarInt    SetInt(const std::string_view& name, int32_t value);
CVarFloat  SetFloat(const std::string_view& name, float value);
CVarString SetString(const std::string_view& name, const std::string& value);

const CVarDesc& GetCVarDesc(const std::string_view& name);

};  // namespace cvars

}  // namespace lumi