#pragma once

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#include "rttr/registration"

#ifdef _WIN32
#pragma warning(pop)
#endif

#define META(Type)                                                            \
    static void rttr_auto_register_reflection_function_##Type();              \
    namespace {                                                               \
    struct rttr__auto__register__##Type {                                     \
        rttr__auto__register__##Type() {                                      \
            rttr_auto_register_reflection_function_##Type();                  \
        }                                                                     \
    };                                                                        \
    }                                                                         \
    static const rttr__auto__register__##Type RTTR_CAT(auto_register__##Type, \
                                                       __LINE__);             \
    static void rttr_auto_register_reflection_function_##Type()

#define META_ENABLE(...) RTTR_ENABLE(__VA_ARGS__)

namespace lumi {

namespace meta {

using Meta = rttr::registration;
using Type = rttr::type;
using Policy = rttr::policy;

}

using Meta = meta::Meta;

};  // namespace lumi