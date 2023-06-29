#pragma once

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/color_space.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/hash.hpp"

#ifdef _WIN32
#pragma warning(pop)
#endif

#include "core/json.h"

namespace lumi {

struct Vec2f : public glm::vec2 {
    using glm::vec2::vec2;

    static const Vec2f kZero;
    static const Vec2f kUnitX;
    static const Vec2f kUnitY;
    static const Vec2f kNegativeUnitX;
    static const Vec2f kNegativeUnitY;
    static const Vec2f kUnitScale;
};

inline constexpr Vec2f Vec2f::kZero          = Vec2f(0, 0);
inline constexpr Vec2f Vec2f::kUnitX         = Vec2f(1, 0);
inline constexpr Vec2f Vec2f::kUnitY         = Vec2f(0, 1);
inline constexpr Vec2f Vec2f::kNegativeUnitX = Vec2f(-1, 0);
inline constexpr Vec2f Vec2f::kNegativeUnitY = Vec2f(0, -1);
inline constexpr Vec2f Vec2f::kUnitScale     = Vec2f(1, 1);

struct Vec3f : public glm::vec3 {
    using glm::vec3::vec3;

    static const Vec3f kZero;
    static const Vec3f kUnitX;
    static const Vec3f kUnitY;
    static const Vec3f kUnitZ;
    static const Vec3f kNegativeUnitX;
    static const Vec3f kNegativeUnitY;
    static const Vec3f kNegativeUnitZ;
    static const Vec3f kUnitScale;
};

inline constexpr Vec3f Vec3f::kZero          = Vec3f(0, 0, 0);
inline constexpr Vec3f Vec3f::kUnitX         = Vec3f(1, 0, 0);
inline constexpr Vec3f Vec3f::kUnitY         = Vec3f(0, 1, 0);
inline constexpr Vec3f Vec3f::kUnitZ         = Vec3f(0, 0, 1);
inline constexpr Vec3f Vec3f::kNegativeUnitX = Vec3f(-1, 0, 0);
inline constexpr Vec3f Vec3f::kNegativeUnitY = Vec3f(0, -1, 0);
inline constexpr Vec3f Vec3f::kNegativeUnitZ = Vec3f(0, 0, -1);
inline constexpr Vec3f Vec3f::kUnitScale     = Vec3f(1, 1, 1);

struct Vec4f : public glm::vec4 {
    using glm::vec4::vec4;

    static const Vec4f kZero;
    static const Vec4f kUnitX;
    static const Vec4f kUnitY;
    static const Vec4f kUnitZ;
    static const Vec4f kUnitW;
    static const Vec4f kNegativeUnitX;
    static const Vec4f kNegativeUnitY;
    static const Vec4f kNegativeUnitZ;
    static const Vec4f kNegativeUnitW;
    static const Vec4f kUnitScale;
};

inline constexpr Vec4f Vec4f::kZero          = Vec4f(0, 0, 0, 0);
inline constexpr Vec4f Vec4f::kUnitX         = Vec4f(1, 0, 0, 0);
inline constexpr Vec4f Vec4f::kUnitY         = Vec4f(0, 1, 0, 0);
inline constexpr Vec4f Vec4f::kUnitZ         = Vec4f(0, 0, 1, 0);
inline constexpr Vec4f Vec4f::kUnitW         = Vec4f(0, 0, 0, 1);
inline constexpr Vec4f Vec4f::kNegativeUnitX = Vec4f(-1, 0, 0, 0);
inline constexpr Vec4f Vec4f::kNegativeUnitY = Vec4f(0, -1, 0, 0);
inline constexpr Vec4f Vec4f::kNegativeUnitZ = Vec4f(0, 0, -1, 0);
inline constexpr Vec4f Vec4f::kNegativeUnitW = Vec4f(0, 0, 0, -1);
inline constexpr Vec4f Vec4f::kUnitScale     = Vec4f(1, 1, 1, 1);

struct Mat3x3f : public glm::mat3x3 {
    using glm::mat3x3::mat3x3;

    static const Mat3x3f kZero;
    static const Mat3x3f kIdentity;
};

inline constexpr Mat3x3f Mat3x3f::kZero     = Mat3x3f(0.0f);
inline constexpr Mat3x3f Mat3x3f::kIdentity = Mat3x3f(1.0f);

struct Mat4x4f : public glm::mat4x4 {
    using glm::mat4x4::mat4x4;

    static const Mat4x4f kZero;
    static const Mat4x4f kIdentity;

    static Mat4x4f Scale(const Vec3f& v) { return glm::scale(Mat4x4f(1.f), v); }

    static Mat4x4f Rotation(float angle, const Vec3f& v) {
        return glm::rotate(Mat4x4f(1.f), angle, v);
    }

    static Mat4x4f Translation(const Vec3f& v) {
        return glm::translate(Mat4x4f(1.f), v);
    }

    static Mat4x4f Perspective(float fovy, float aspect, float near, float far) {
        return glm::perspective(fovy, aspect, near, far);
    }
};

inline constexpr Mat4x4f Mat4x4f::kZero     = Mat4x4f(0.0f);
inline constexpr Mat4x4f Mat4x4f::kIdentity = Mat4x4f(1.0f);

struct Quaternion : public glm::quat {
    using glm::quat::quat;

    static const Quaternion kZero;
    static const Quaternion kIdentity;

    Mat4x4f ToMatrix() const { return glm::toMat4(*this); }
};

inline constexpr Quaternion Quaternion::kZero     = Quaternion(0, 0, 0, 0);
inline constexpr Quaternion Quaternion::kIdentity = Quaternion(1, 0, 0, 0);

inline constexpr float kPosInf    = std::numeric_limits<float>::infinity();
inline constexpr float kNegInf    = -std::numeric_limits<float>::infinity();
inline constexpr float kInf       = kPosInf;
inline constexpr float kEps       = FLT_EPSILON;
inline constexpr float kPi        = 3.14159265358979323846264338327950288f;
inline constexpr float kTwoPi     = 2.0f * kPi;
inline constexpr float kHalfPi    = 0.5f * kPi;
inline constexpr float kOneOverPi = 1.0f / kPi;

inline float ToRadians(float degrees) { return glm::radians(degrees); }
inline float ToDegrees(float radians) { return glm::degrees(radians); }

inline Vec3f ToLinear(Vec3f srgb) { return glm::convertSRGBToLinear(srgb); }
inline Vec4f ToLinear(Vec4f srgb) { return glm::convertSRGBToLinear(srgb); }

inline Vec3f ToSRGB(Vec3f linear) { return glm::convertLinearToSRGB(linear); }
inline Vec4f ToSRGB(Vec4f linear) { return glm::convertLinearToSRGB(linear); }

}  // namespace lumi


// formatter for spdlog
namespace std {

inline std::ostream& operator<<(std::ostream& os, const lumi::Vec2f& v) {
    return os << "Vec2f( "           //
              << v.x << ", " << v.y  //
              << ")";
}

inline std::ostream& operator<<(std::ostream& os, const lumi::Vec3f& v) {
    return os << "Vec3f("                           //
              << v.x << ", " << v.y << ", " << v.z  //
              << ")";
}

inline std::ostream& operator<<(std::ostream& os, const lumi::Vec4f& v) {
    return os << "Vec4f("                                          //
              << v.x << ", " << v.y << ", " << v.z << ", " << v.w  //
              << ")";
}

inline std::ostream& operator<<(std::ostream& os, const lumi::Quaternion& q) {
    // order: WXYZ
    return os << "Quaternion("                                     //
              << q.w << ", " << q.x << ", " << q.y << ", " << q.z  //
              << ")";
}

// !!Important: GLM matrix memory layout is column-major,
// !!           but we format the matrix in row-major
// !!           in order to keep the same as math notations.
inline std::ostream& operator<<(std::ostream& os, const lumi::Mat3x3f& m) {
    // clang-format off
    return os << "Mat3x3f(\n"
              << "        " << m[0][0] << ", " << m[1][0] << ", " 
                            << m[2][0] << ", \n"
              << "        " << m[0][1] << ", " << m[1][1] << ", " 
                            << m[2][1] << ", \n"
              << "        " << m[0][2] << ", " << m[1][2] << ", " 
                            << m[2][2] << ")";
    // clang-format on
}

inline std::ostream& operator<<(std::ostream& os, const lumi::Mat4x4f& m) {
    // clang-format off
    return os << "Mat4x4f(\n"
              << "        " << m[0][0] << ", " << m[1][0] << ", " 
                            << m[2][0] << ", " << m[3][0] << ", \n"
              << "        " << m[0][1] << ", " << m[1][1] << ", " 
                            << m[2][1] << ", " << m[3][1] << ", \n"
              << "        " << m[0][2] << ", " << m[1][2] << ", " 
                            << m[2][2] << ", " << m[3][2] << ", \n"
              << "        " << m[0][3] << ", " << m[1][3] << ", " 
                            << m[2][3] << ", " << m[3][3] << ")";
    // clang-format on
}

}  // namespace std

// Json converter
namespace glm {

inline void to_json(lumi::Json& j, const lumi::Vec2f& v) {
    j = lumi::Json::array({v.x, v.y});
}

inline void from_json(const lumi::Json& j, lumi::Vec2f& v) {
    v.x = j[0];
    v.y = j[1];
}

inline void to_json(lumi::Json& j, const lumi::Vec3f& v) {
    j = lumi::Json::array({v.x, v.y, v.z});
}

inline void from_json(const lumi::Json& j, lumi::Vec3f& v) {
    v.x = j[0];
    v.y = j[1];
    v.z = j[2];
}

inline void to_json(lumi::Json& j, const lumi::Vec4f& v) {
    j = lumi::Json::array({v.x, v.y, v.z, v.w});
}

inline void from_json(const lumi::Json& j, lumi::Vec4f& v) {
    v.x = j[0];
    v.y = j[1];
    v.z = j[2];
    v.w = j[3];
}

// !!Important: GLM matrix memory layout is column-major,
// !!           but we format the matrix in row-major
// !!           in order to keep the same as math notations.

inline void to_json(lumi::Json& j, const lumi::Mat3x3f& m) {
    // clang-format off
    j = lumi::Json::array({
       { m[0][0], m[1][0], m[2][0], },    
       { m[0][1], m[1][1], m[2][1], },  
       { m[0][2], m[1][2], m[2][2], },
    });
    // clang-format on
}

inline void from_json(const lumi::Json& j, lumi::Mat3x3f& m) {
    // clang-format off
    auto a = j.get<std::vector<std::vector<float>>>();
    m = {
       { a[0][0], a[1][0], a[2][0], },    
       { a[0][1], a[1][1], a[2][1], },  
       { a[0][2], a[1][2], a[2][2], },
    };
    // clang-format on
}

inline void to_json(lumi::Json& j, const lumi::Mat4x4f& m) {
    // clang-format off
    j = lumi::Json::array({
       { m[0][0], m[1][0], m[2][0], m[3][0], },    
       { m[0][1], m[1][1], m[2][1], m[3][1], },  
       { m[0][2], m[1][2], m[2][2], m[3][2], },
       { m[0][3], m[1][3], m[2][3], m[3][3], }, 
    });
    // clang-format on
}

inline void from_json(const lumi::Json& j, lumi::Mat4x4f& m) {
    // clang-format off
    auto a = j.get<std::vector<std::vector<float>>>();
    m = {
       { a[0][0], a[1][0], a[2][0], a[3][0], },    
       { a[0][1], a[1][1], a[2][1], a[3][1], },  
       { a[0][2], a[1][2], a[2][2], a[3][2], },
       { a[0][3], a[1][3], a[2][3], a[3][3], }, 
    };
    // clang-format on
}

}  // namespace glm