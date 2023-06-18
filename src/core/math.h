#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

#include "core/log.h"
#include "core/json.h"

namespace lumi {

using Vec1f = glm::vec1;
using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;

using Quaternion = glm::quat;

using Mat2x2f = glm::mat2x2;
using Mat2x3f = glm::mat2x3;
using Mat2x4f = glm::mat2x4;
using Mat3x2f = glm::mat3x2;
using Mat3x3f = glm::mat3x3;
using Mat3x4f = glm::mat3x4;
using Mat4x2f = glm::mat4x2;
using Mat4x3f = glm::mat4x3;
using Mat4x4f = glm::mat4x4;

constexpr float kPosInf = std::numeric_limits<float>::infinity();
constexpr float kNegInf = -std::numeric_limits<float>::infinity();
constexpr float kInf    = kPosInf;
constexpr float kEps    = FLT_EPSILON;

constexpr float kPi        = 3.14159265358979323846264338327950288f;
constexpr float kTwoPi     = 2.0f * kPi;
constexpr float kHalfPi    = 0.5f * kPi;
constexpr float kOneOverPi = 1.0f / kPi;

constexpr float kDeg2Rad = kPi / 180.0f;
constexpr float kRad2Deg = 180.0f / kPi;

}  // namespace lumi


// formatter for spdlog

inline std::ostream& operator<<(std::ostream& os, const lumi::Vec1f& v) {
    return os << "Vec1f( "  //
              << v.x        //
              << ")";
}

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

// Json converter
namespace glm {

inline void to_json(lumi::Json& j, const lumi::Vec1f& v) {
    j = lumi::Json::array({v.x});
}

inline void from_json(const lumi::Json& j, lumi::Vec1f& v) {
    v.x = j[0];  //
}

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