#pragma once

#include "core/json.h"

#ifdef _WIN32
#include <codeanalysis/warnings.h>
#pragma warning(push, 0)
#pragma warning(disable : ALL_CODE_ANALYSIS_WARNINGS)
#endif

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include "glm/glm.hpp"
#include "glm/gtc/color_space.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/hash.hpp"
#include "glm/gtx/quaternion.hpp"

#ifdef _WIN32
#pragma warning(pop)
#endif

namespace lumi {

inline constexpr float kPosInf    = std::numeric_limits<float>::infinity();
inline constexpr float kNegInf    = -std::numeric_limits<float>::infinity();
inline constexpr float kInf       = kPosInf;
inline constexpr float kEps       = FLT_EPSILON;
inline constexpr float kPi        = 3.14159265358979323846264338327950288f;
inline constexpr float kTwoPi     = 2.0f * kPi;
inline constexpr float kHalfPi    = 0.5f * kPi;
inline constexpr float kOneOverPi = 1.0f / kPi;

struct Vec2f : public glm::vec2 {
    using glm::vec2::vec2;

    constexpr Vec2f& operator+=(float s) {
        *this = glm::vec2(*this) + s;
        return *this;
    }

    constexpr Vec2f& operator+=(const Vec2f& v) {
        *this = glm::vec2(*this) + glm::vec2(v);
        return *this;
    }

    constexpr Vec2f& operator-=(float s) {
        *this = glm::vec2(*this) - s;
        return *this;
    }

    constexpr Vec2f& operator-=(const Vec2f& v) {
        *this = glm::vec2(*this) - glm::vec2(v);
        return *this;
    }

    constexpr Vec2f& operator*=(float s) {
        *this = glm::vec2(*this) * s;
        return *this;
    }

    constexpr Vec2f& operator*=(const Vec2f& v) {
        *this = glm::vec2(*this) * glm::vec2(v);
        return *this;
    }

    constexpr Vec2f& operator/=(float s) {
        *this = glm::vec2(*this) / s;
        return *this;
    }

    constexpr Vec2f& operator/=(const Vec2f& v) {
        *this = glm::vec2(*this) / glm::vec2(v);
        return *this;
    }

    static const Vec2f kZero;
    static const Vec2f kUnitX;
    static const Vec2f kUnitY;
    static const Vec2f kNegativeUnitX;
    static const Vec2f kNegativeUnitY;
    static const Vec2f kUnitScale;
};

constexpr Vec2f operator+(float s, const Vec2f& a) { return s + glm::vec2(a); }

constexpr Vec2f operator+(const Vec2f& a, float s) { return glm::vec2(a) + s; }

constexpr Vec2f operator+(const Vec2f& a, const Vec2f& b) {
    return glm::vec2(a) + glm::vec2(b);
}

constexpr Vec2f operator-(float s, const Vec2f& a) { return s - glm::vec2(a); }

constexpr Vec2f operator-(const Vec2f& a, float s) { return glm::vec2(a) - s; }

constexpr Vec2f operator-(const Vec2f& a, const Vec2f& b) {
    return glm::vec2(a) - glm::vec2(b);
}

constexpr Vec2f operator*(float s, const Vec2f& a) { return s * glm::vec2(a); }

constexpr Vec2f operator*(const Vec2f& a, float s) { return glm::vec2(a) * s; }

constexpr Vec2f operator*(const Vec2f& a, const Vec2f& b) {
    return glm::vec2(a) * glm::vec2(b);
}

constexpr Vec2f operator/(float s, const Vec2f& a) { return s / glm::vec2(a); }

constexpr Vec2f operator/(const Vec2f& a, float s) { return glm::vec2(a) / s; }

constexpr Vec2f operator/(const Vec2f& a, const Vec2f& b) {
    return glm::vec2(a) / glm::vec2(b);
}

inline constexpr Vec2f Vec2f::kZero          = Vec2f(0, 0);
inline constexpr Vec2f Vec2f::kUnitX         = Vec2f(1, 0);
inline constexpr Vec2f Vec2f::kUnitY         = Vec2f(0, 1);
inline constexpr Vec2f Vec2f::kNegativeUnitX = Vec2f(-1, 0);
inline constexpr Vec2f Vec2f::kNegativeUnitY = Vec2f(0, -1);
inline constexpr Vec2f Vec2f::kUnitScale     = Vec2f(1, 1);

struct Vec3f : public glm::vec3 {
    using glm::vec3::vec3;

    constexpr Vec3f& operator+=(float s) {
        *this = glm::vec3(*this) + s;
        return *this;
    }

    constexpr Vec3f& operator+=(const Vec3f& v) {
        *this = glm::vec3(*this) + glm::vec3(v);
        return *this;
    }

    constexpr Vec3f& operator-=(float s) {
        *this = glm::vec3(*this) - s;
        return *this;
    }

    constexpr Vec3f& operator-=(const Vec3f& v) {
        *this = glm::vec3(*this) - glm::vec3(v);
        return *this;
    }

    constexpr Vec3f& operator*=(float s) {
        *this = glm::vec3(*this) * s;
        return *this;
    }

    constexpr Vec3f& operator*=(const Vec3f& v) {
        *this = glm::vec3(*this) * glm::vec3(v);
        return *this;
    }

    constexpr Vec3f& operator/=(float s) {
        *this = glm::vec3(*this) / s;
        return *this;
    }

    constexpr Vec3f& operator/=(const Vec3f& v) {
        *this = glm::vec3(*this) / glm::vec3(v);
        return *this;
    }

    static const Vec3f kZero;
    static const Vec3f kUnitX;
    static const Vec3f kUnitY;
    static const Vec3f kUnitZ;
    static const Vec3f kNegativeUnitX;
    static const Vec3f kNegativeUnitY;
    static const Vec3f kNegativeUnitZ;
    static const Vec3f kUnitScale;

    static const Vec3f kBlack;
    static const Vec3f kWhite;
    static const Vec3f kRed;
    static const Vec3f kGreen;
    static const Vec3f kBlue;
    static const Vec3f kCyan;
    static const Vec3f kMagenta;
    static const Vec3f kYellow;
};

constexpr Vec3f operator+(float s, const Vec3f& a) { return s + glm::vec3(a); }

constexpr Vec3f operator+(const Vec3f& a, float s) { return glm::vec3(a) + s; }

constexpr Vec3f operator+(const Vec3f& a, const Vec3f& b) {
    return glm::vec3(a) + glm::vec3(b);
}

constexpr Vec3f operator-(float s, const Vec3f& a) { return s - glm::vec3(a); }

constexpr Vec3f operator-(const Vec3f& a, float s) { return glm::vec3(a) - s; }

constexpr Vec3f operator-(const Vec3f& a, const Vec3f& b) {
    return glm::vec3(a) - glm::vec3(b);
}

constexpr Vec3f operator*(float s, const Vec3f& a) { return s * glm::vec3(a); }

constexpr Vec3f operator*(const Vec3f& a, float s) { return glm::vec3(a) * s; }

constexpr Vec3f operator*(const Vec3f& a, const Vec3f& b) {
    return glm::vec3(a) * glm::vec3(b);
}

constexpr Vec3f operator/(float s, const Vec3f& a) { return s / glm::vec3(a); }

constexpr Vec3f operator/(const Vec3f& a, float s) { return glm::vec3(a) / s; }

constexpr Vec3f operator/(const Vec3f& a, const Vec3f& b) {
    return glm::vec3(a) / glm::vec3(b);
}

using Color3f = Vec3f;

inline constexpr Vec3f Vec3f::kZero          = Vec3f(0, 0, 0);
inline constexpr Vec3f Vec3f::kUnitX         = Vec3f(1, 0, 0);
inline constexpr Vec3f Vec3f::kUnitY         = Vec3f(0, 1, 0);
inline constexpr Vec3f Vec3f::kUnitZ         = Vec3f(0, 0, 1);
inline constexpr Vec3f Vec3f::kNegativeUnitX = Vec3f(-1, 0, 0);
inline constexpr Vec3f Vec3f::kNegativeUnitY = Vec3f(0, -1, 0);
inline constexpr Vec3f Vec3f::kNegativeUnitZ = Vec3f(0, 0, -1);
inline constexpr Vec3f Vec3f::kUnitScale     = Vec3f(1, 1, 1);

inline constexpr Color3f Color3f::kBlack   = Color3f(0, 0, 0);
inline constexpr Color3f Color3f::kWhite   = Color3f(1, 1, 1);
inline constexpr Color3f Color3f::kRed     = Color3f(1, 0, 0);
inline constexpr Color3f Color3f::kGreen   = Color3f(0, 1, 0);
inline constexpr Color3f Color3f::kBlue    = Color3f(0, 0, 1);
inline constexpr Color3f Color3f::kCyan    = Color3f(0, 1, 1);
inline constexpr Color3f Color3f::kMagenta = Color3f(1, 0, 1);
inline constexpr Color3f Color3f::kYellow  = Color3f(1, 1, 0);

struct Vec4f : public glm::vec4 {
    using glm::vec4::vec4;

    constexpr Vec4f& operator+=(float s) {
        *this = glm::vec4(*this) + s;
        return *this;
    }

    constexpr Vec4f& operator+=(const Vec4f& v) {
        *this = glm::vec4(*this) + glm::vec4(v);
        return *this;
    }

    constexpr Vec4f& operator-=(float s) {
        *this = glm::vec4(*this) - s;
        return *this;
    }

    constexpr Vec4f& operator-=(const Vec4f& v) {
        *this = glm::vec4(*this) - glm::vec4(v);
        return *this;
    }

    constexpr Vec4f& operator*=(float s) {
        *this = glm::vec4(*this) * s;
        return *this;
    }

    constexpr Vec4f& operator*=(const Vec4f& v) {
        *this = glm::vec4(*this) * glm::vec4(v);
        return *this;
    }

    constexpr Vec4f& operator/=(float s) {
        *this = glm::vec4(*this) / s;
        return *this;
    }

    constexpr Vec4f& operator/=(const Vec4f& v) {
        *this = glm::vec4(*this) / glm::vec4(v);
        return *this;
    }

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

    static const Vec4f kBlack;
    static const Vec4f kWhite;
    static const Vec4f kRed;
    static const Vec4f kGreen;
    static const Vec4f kBlue;
    static const Vec4f kCyan;
    static const Vec4f kMagenta;
    static const Vec4f kYellow;
};

constexpr Vec4f operator+(float s, const Vec4f& a) { return s + glm::vec4(a); }

constexpr Vec4f operator+(const Vec4f& a, float s) { return glm::vec4(a) + s; }

constexpr Vec4f operator+(const Vec4f& a, const Vec4f& b) {
    return glm::vec4(a) + glm::vec4(b);
}

constexpr Vec4f operator-(float s, const Vec4f& a) { return s - glm::vec4(a); }

constexpr Vec4f operator-(const Vec4f& a, float s) { return glm::vec4(a) - s; }

constexpr Vec4f operator-(const Vec4f& a, const Vec4f& b) {
    return glm::vec4(a) - glm::vec4(b);
}

constexpr Vec4f operator*(float s, const Vec4f& a) { return s * glm::vec4(a); }

constexpr Vec4f operator*(const Vec4f& a, float s) { return glm::vec4(a) * s; }

constexpr Vec4f operator*(const Vec4f& a, const Vec4f& b) {
    return glm::vec4(a) * glm::vec4(b);
}

constexpr Vec4f operator/(float s, const Vec4f& a) { return s / glm::vec4(a); }

constexpr Vec4f operator/(const Vec4f& a, float s) { return glm::vec4(a) / s; }

constexpr Vec4f operator/(const Vec4f& a, const Vec4f& b) {
    return glm::vec4(a) / glm::vec4(b);
}

using Color4f = Vec4f;

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

// clang-format off
inline constexpr Color4f Color4f::kBlack   = Color4f(Color3f::kBlack,   1);
inline constexpr Color4f Color4f::kWhite   = Color4f(Color3f::kWhite,   1);
inline constexpr Color4f Color4f::kRed     = Color4f(Color3f::kRed,     1);
inline constexpr Color4f Color4f::kGreen   = Color4f(Color3f::kGreen,   1);
inline constexpr Color4f Color4f::kBlue    = Color4f(Color3f::kBlue,    1);
inline constexpr Color4f Color4f::kCyan    = Color4f(Color3f::kCyan,    1);
inline constexpr Color4f Color4f::kMagenta = Color4f(Color3f::kMagenta, 1);
inline constexpr Color4f Color4f::kYellow  = Color4f(Color3f::kYellow,  1);
// clang-format on

struct Color3u8 : public glm::u8vec3 {
    using glm::u8vec3::u8vec3;

    static const Color3u8 kBlack;
    static const Color3u8 kWhite;
    static const Color3u8 kRed;
    static const Color3u8 kGreen;
    static const Color3u8 kBlue;
    static const Color3u8 kCyan;
    static const Color3u8 kMagenta;
    static const Color3u8 kYellow;
};

// clang-format off
inline constexpr Color3u8 Color3u8::kBlack   = Color3u8(0,   0,   0  );
inline constexpr Color3u8 Color3u8::kWhite   = Color3u8(255, 255, 255);
inline constexpr Color3u8 Color3u8::kRed     = Color3u8(255, 0,   0  );
inline constexpr Color3u8 Color3u8::kGreen   = Color3u8(0,   255, 0  );
inline constexpr Color3u8 Color3u8::kBlue    = Color3u8(0,   0,   255);
inline constexpr Color3u8 Color3u8::kCyan    = Color3u8(0,   255, 255);
inline constexpr Color3u8 Color3u8::kMagenta = Color3u8(255, 0,   255);
inline constexpr Color3u8 Color3u8::kYellow  = Color3u8(255, 255, 0  );
// clang-format on

struct Color4u8 : public glm::u8vec4 {
    using glm::u8vec4::u8vec4;

    static const Color4u8 kBlack;
    static const Color4u8 kWhite;
    static const Color4u8 kRed;
    static const Color4u8 kGreen;
    static const Color4u8 kBlue;
    static const Color4u8 kCyan;
    static const Color4u8 kMagenta;
    static const Color4u8 kYellow;
};

// clang-format off
inline constexpr Color4u8 Color4u8::kBlack   = Color4u8(Color3u8::kBlack,   255);
inline constexpr Color4u8 Color4u8::kWhite   = Color4u8(Color3u8::kWhite,   255);
inline constexpr Color4u8 Color4u8::kRed     = Color4u8(Color3u8::kRed,     255);
inline constexpr Color4u8 Color4u8::kGreen   = Color4u8(Color3u8::kGreen,   255);
inline constexpr Color4u8 Color4u8::kBlue    = Color4u8(Color3u8::kBlue,    255);
inline constexpr Color4u8 Color4u8::kCyan    = Color4u8(Color3u8::kCyan,    255);
inline constexpr Color4u8 Color4u8::kMagenta = Color4u8(Color3u8::kMagenta, 255);
inline constexpr Color4u8 Color4u8::kYellow  = Color4u8(Color3u8::kYellow,  255);
// clang-format on

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

    static Mat4x4f Scale(const Vec3f& v) { return glm::scale(kIdentity, v); }

    static Mat4x4f Rotation(float angle, const Vec3f& v) {
        return glm::rotate(kIdentity, angle, v);
    }

    static Mat4x4f Rotation(Vec3f eulers) {
        // yaw   <--> y
        // pitch <--> x
        // roll  <--> z
        return glm::yawPitchRoll(eulers.y, eulers.x, eulers.z);
    }

    static Mat4x4f Translation(const Vec3f& v) {
        return glm::translate(kIdentity, v);
    }

    static Mat4x4f Perspective(float fovy, float aspect, float near,
                               float far) {
        return glm::perspective(fovy, aspect, near, far);
    }

    Mat4x4f Transpose() const { return glm::transpose(*this); }
};

inline constexpr Mat4x4f Mat4x4f::kZero     = Mat4x4f(0.0f);
inline constexpr Mat4x4f Mat4x4f::kIdentity = Mat4x4f(1.0f);

struct Quaternion : public glm::quat {
    using glm::quat::quat;

    static const Quaternion kZero;
    static const Quaternion kIdentity;

    explicit operator Mat4x4f() const { return glm::toMat4(*this); }

    static Quaternion Rotation(float angle, const Vec3f& v) {
        return glm::rotate(kIdentity, angle, v);
    }

    static Quaternion Rotation(Vec3f eulers) { return Quaternion(eulers); }
};

inline constexpr Quaternion Quaternion::kZero     = Quaternion(0, 0, 0, 0);
inline constexpr Quaternion Quaternion::kIdentity = Quaternion(1, 0, 0, 0);

inline float ToRadians(float degrees) { return glm::radians(degrees); }
inline Vec2f ToRadians(Vec2f degrees) {
    return glm::radians((glm::vec2)degrees);
}
inline Vec3f ToRadians(Vec3f degrees) {
    return glm::radians((glm::vec3)degrees);
}
inline Vec4f ToRadians(Vec4f degrees) {
    return glm::radians((glm::vec4)degrees);
}

inline float ToDegrees(float radians) { return glm::degrees(radians); }
inline Vec2f ToDegrees(Vec2f radians) {
    return glm::degrees((glm::vec2)radians);
}
inline Vec3f ToDegrees(Vec3f radians) {
    return glm::degrees((glm::vec3)radians);
}
inline Vec4f ToDegrees(Vec4f radians) {
    return glm::degrees((glm::vec4)radians);
}

inline Vec3f ToLinear(const Vec3f& srgb) {
    return glm::convertSRGBToLinear(srgb);
}
inline Vec4f ToLinear(const Vec4f& srgb) {
    return glm::convertSRGBToLinear(srgb);
}

inline Vec3f ToSRGB(const Vec3f& linear) {
    return glm::convertLinearToSRGB(linear);
}
inline Vec4f ToSRGB(const Vec4f& linear) {
    return glm::convertLinearToSRGB(linear);
}

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