#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "json.h"

namespace lumi {

constexpr float kPosInf    = std::numeric_limits<float>::infinity();
constexpr float kNegInf    = -std::numeric_limits<float>::infinity();
constexpr float kInf       = kPosInf;
constexpr float kEps       = FLT_EPSILON;

constexpr float kPi        = 3.14159265358979323846264338327950288f;
constexpr float kTwoPi     = 2.0f * kPi;
constexpr float kHalfPi    = 0.5f * kPi;
constexpr float kOneOverPi = 1.0f / kPi;

constexpr float kDeg2Rad   = kPi / 180.0f;
constexpr float kRad2Deg   = 180.0f / kPi;

using Vec3f    = glm::vec3;

// using Vector2    = DirectX::SimpleMath::Vector2;
// using Vector3    = DirectX::SimpleMath::Vector3;
// using Vector4    = DirectX::SimpleMath::Vector4;
// using Color      = DirectX::SimpleMath::Color;
// using Quaternion = DirectX::SimpleMath::Quaternion;
// using Matrix     = DirectX::SimpleMath::Matrix;
// //using Plane      = DirectX::SimpleMath::Plane;
// //using Rectangle  = DirectX::SimpleMath::Rectangle;
// //using Ray        = DirectX::SimpleMath::Ray;
// //using Viewport   = DirectX::SimpleMath::Viewport;

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Vector2& v) {
//     return os << "Vector2( "         //
//               << v.x << ", " << v.y  //
//               << ")";
// }

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Vector3& v) {
//     return os << "Vector3("                         //
//               << v.x << ", " << v.y << ", " << v.z  //
//               << ")";
// }

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Vector4& v) {
//     return os << "Vector4("                                        //
//               << v.x << ", " << v.y << ", " << v.z << ", " << v.w  //
//               << ")";
// }

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Color& c) {
//     return os << "Color("                                          //
//               << c.x << ", " << c.y << ", " << c.z << ", " << c.w  //
//               << ")";
// }

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Quaternion& q) {
//     return os << "Quaternion("                                     //
//               << q.x << ", " << q.y << ", " << q.z << ", " << q.w  //
//               << ")";
// }

// template <typename OStream>
// inline OStream& operator<<(OStream& os, const Matrix& m) {
//     return os << "Matrix("  //
//               << m._11 << ", " << m._12 << ", " << m._13 << ", " << m._14
//               << "\n"  //
//               << m._21 << ", " << m._22 << ", " << m._23 << ", " << m._24
//               << "\n"  //
//               << m._31 << ", " << m._32 << ", " << m._33 << ", " << m._34
//               << "\n"  //
//               << m._41 << ", " << m._42 << ", " << m._43 << ", " << m._44
//               << "\n"  //
//               << ")";
// }

}  // namespace lumi

// // Json converter
// namespace DirectX {
// namespace SimpleMath {

// SERIALIZABLE_NON_INTRUSIVE(Vector2, x, y);
// SERIALIZABLE_NON_INTRUSIVE(Vector3, x, y, z);
// SERIALIZABLE_NON_INTRUSIVE(Vector4, x, y, z, w);
// SERIALIZABLE_NON_INTRUSIVE(Color, x, y, z, w);
// SERIALIZABLE_NON_INTRUSIVE(Quaternion, x, y, z, w);
// SERIALIZABLE_NON_INTRUSIVE(Matrix,              //
//                            _11, _12, _13, _14,  //
//                            _21, _22, _23, _24,  //
//                            _31, _32, _33, _34,  //
//                            _41, _42, _43, _44)

// }  // namespace SimpleMath
// }  // namespace DirectX
