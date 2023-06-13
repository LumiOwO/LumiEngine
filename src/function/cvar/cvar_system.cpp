#include "cvar_system.h"

#include <memory>
#include <unordered_map>

#include "core/hash.h"
#include "core/log.h"
#include "core/singleton.h"

namespace lumi {

// storage for CVar
template <typename T>
struct CVarStorage {
    CVarDesc* p_desc{};
    T         value{};
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

        int index = cnt;
        cnt++;

        values[index].p_desc = p_desc;
        values[index].value  = value;
        p_desc->index_       = index;
        return index;
    }
};

class CVarSystem : public ISingleton<CVarSystem> {
private:
    constexpr static int kMaxCVarsCounts[CVarType::kNumOfTypes] = {
        500,  // kBool
        500,  // kInt
        500,  // kFloat
        200,  // kString
    };

    using CVarArrayBool  = CVarArray<bool, kMaxCVarsCounts[CVarType::kBool]>;
    using CVarArrayInt   = CVarArray<int32_t, kMaxCVarsCounts[CVarType::kInt]>;
    using CVarArrayFloat = CVarArray<float, kMaxCVarsCounts[CVarType::kFloat]>;
    using CVarArrayString =
        CVarArray<std::string, kMaxCVarsCounts[CVarType::kString]>;

    CVarArrayBool   cvar_arrays_bool_   = CVarArrayBool();
    CVarArrayInt    cvar_arrays_int_    = CVarArrayInt();
    CVarArrayFloat  cvar_arrays_float_  = CVarArrayFloat();
    CVarArrayString cvar_arrays_string_ = CVarArrayString();

    CVarArrayBase* cvar_arrays_ptrs_[CVarType::kNumOfTypes] = {
        &cvar_arrays_bool_,
        &cvar_arrays_int_,
        &cvar_arrays_float_,
        &cvar_arrays_string_,
    };

    std::unordered_map<uint32_t, CVarDesc> table_{};

public:
    // find by name
    template <typename T>
    CVar<T> CreateCVar(const std::string_view& name, const T& value,
                       const std::string_view& description, CVarFlags flags) {
        auto hash = StringHash(name);

        CVarDesc* p_found_desc = FindCVarDescInTable(hash);
        if (p_found_desc) {
            LOG_WARNING(
                "Console variable \"{}\" already exists. "
                "An invalid CVar handle will be returned.\n"
                "This may caused by duplicate name or hash conflict.",
                name);
            return CVar<T>(-1);
        }

        // Insert new CVarDesc
        CVarDesc& desc   = table_[hash];
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
                "An invalid CVar handle will be returned.\n",
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
                "An invalid CVar handle will be returned.\n",
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

private:
    template <typename T>
    auto GetCVarArrayPtr() {
        constexpr int arr_idx = CVar<T>::type();
        using CVarArrayType   = CVarArray<T, kMaxCVarsCounts[arr_idx]>;
        CVarArrayType* p_array =
            static_cast<CVarArrayType*>(cvar_arrays_ptrs_[arr_idx]);
        return p_array;
    }

    CVarDesc* FindCVarDescInTable(StringHash hash) {
        auto it = table_.find(hash);
        if (it == table_.end()) return nullptr;
        return &it->second;
    }
};

// definitions of cvars namespace functions
namespace cvars {

void Init() {
    CVarBool("test.a", false);
    CVarBool("test.b", true);
    CVarBool("test.b", true);
}

void SaveToDisk() {}

CVarBool CreateBool(const std::string_view& name, bool value,
                    const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<bool>(name, value, description,
                                                   flags);
}

CVarInt CreateInt(const std::string_view& name, int32_t value,
                  const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<int32_t>(name, value, description,
                                                      flags);
}
CVarFloat CreateFloat(const std::string_view& name, float value,
                      const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<float>(name, value, description,
                                                    flags);
}

CVarString CreateString(const std::string_view& name, const std::string& value,
                        const std::string_view& description, CVarFlags flags) {
    return CVarSystem::Instance().CreateCVar<std::string>(name, value,
                                                          description, flags);
}

CVarBool GetBool(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<bool>(name);
}

CVarInt GetInt(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<int32_t>(name);
}

CVarFloat GetFloat(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<float>(name);
}

CVarString GetString(const std::string_view& name) {
    return CVarSystem::Instance().GetCVar<std::string>(name);
}

CVarBool SetBool(const std::string_view& name, bool value) {
    return CVarSystem::Instance().SetCVar<bool>(name, value);
}

CVarInt SetInt(const std::string_view& name, int32_t value) {
    return CVarSystem::Instance().SetCVar<int32_t>(name, value);
}

CVarFloat SetFloat(const std::string_view& name, float value) {
    return CVarSystem::Instance().SetCVar<float>(name, value);
}

CVarString SetString(const std::string_view& name, const std::string& value) {
    return CVarSystem::Instance().SetCVar<std::string>(name, value);
}

const CVarDesc& GetCVarDesc(const std::string_view& name) {
    return *CVarSystem::Instance().GetCVarDescPtr(name);
}

}  // namespace cvars

// definitions of CVar<T> member functions
template<>
CVarBool::CVar(const std::string_view& name, const bool& value,
               const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateBool(name, value, description, flags);
}

template <>
CVarInt::CVar(const std::string_view& name, const int32_t& value,
              const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateInt(name, value, description, flags);
}

template <>
CVarFloat::CVar(const std::string_view& name, const float& value,
                const std::string_view& description, CVarFlags flags) {
    *this = cvars::CreateFloat(name, value, description, flags);
}

template <>
CVarString::CVar(const std::string_view& name, const std::string& value,
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

template <>
constexpr CVarType CVarBool::type() { return CVarType::kBool; }

template <>
constexpr CVarType CVarInt::type() { return CVarType::kInt; }

template <>
constexpr CVarType CVarFloat::type() { return CVarType::kFloat; }

template <>
constexpr CVarType CVarString::type() { return CVarType::kString; }

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
template class CVar<bool>;
template class CVar<int32_t>;
template class CVar<float>;
template class CVar<std::string>;

}  // namespace lumi
