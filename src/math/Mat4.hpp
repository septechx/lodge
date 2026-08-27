#pragma once

#include "Vec3.hpp"
#include "math.h"

#include <array>
#include <cmath>

struct Quat;

// Helpful matrix diagram:
// 0  1  2  3
// 4  5  6  7
// 8  9  10 11
// 12 13 14 15

struct Mat4 {
public:
  static const Mat4 IDENTITY;

  constexpr Mat4() = default;
  constexpr Mat4(std::array<float, 4> col0, std::array<float, 4> col1,
                 std::array<float, 4> col2, std::array<float, 4> col3)
      : m_data{
            col0[0], col0[1], col0[2], col0[3], col1[0], col1[1],
            col1[2], col1[3], col2[0], col2[1], col2[2], col2[3],
            col3[0], col3[1], col3[2], col3[3],
        } {}

  static constexpr Mat4 perspective(float fovYDeg, float aspect, float nearZ,
                                    float farZ) {
    const float fovYRad = fovYDeg * LDG_PI / 180.0f;
    const float f = 1.0f / std::tan(fovYRad / 2.0f);
    Mat4 r;
    r(0, 0) = f / aspect;
    r(1, 1) = -f;
    r(2, 2) = farZ / (nearZ - farZ);
    r(2, 3) = farZ * nearZ / (nearZ - farZ);
    r(3, 2) = -1.0f;
    return r;
  }

  static constexpr Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 back = (eye - center).normalize();
    Vec3 right = up.cross(back).normalize();
    Vec3 camUp = back.cross(right);
    return {
        {right.x, camUp.x, back.x, 0},
        {right.y, camUp.y, back.y, 0},
        {right.z, camUp.z, back.z, 0},
        {-right.dot(eye), -camUp.dot(eye), -back.dot(eye), 1},
    };
  }

  static constexpr Mat4 translate(Vec3 pos) {
    Mat4 r = IDENTITY;
    r(0, 3) = pos.x;
    r(1, 3) = pos.y;
    r(2, 3) = pos.z;
    return r;
  }

  static constexpr Mat4 scale(Vec3 scale) {
    Mat4 r;
    r(0, 0) = scale.x;
    r(1, 1) = scale.y;
    r(2, 2) = scale.z;
    r(3, 3) = 1.0f;
    return r;
  }

  static constexpr Mat4 rotX(float rad) {
    float c = std::cos(rad);
    float s = std::sin(rad);
    Mat4 r = IDENTITY;
    r(1, 1) = c;
    r(1, 2) = -s;
    r(2, 1) = s;
    r(2, 2) = c;
    return r;
  }

  static constexpr Mat4 rotY(float rad) {
    float c = std::cos(rad);
    float s = std::sin(rad);
    Mat4 r = IDENTITY;
    r(0, 0) = c;
    r(0, 2) = s;
    r(2, 0) = -s;
    r(2, 2) = c;
    return r;
  }

  static constexpr Mat4 rotZ(float rad) {
    float c = std::cos(rad);
    float s = std::sin(rad);
    Mat4 r = IDENTITY;
    r(0, 0) = c;
    r(0, 1) = -s;
    r(1, 0) = s;
    r(1, 1) = c;
    return r;
  }

  static constexpr Mat4 rotEuler(Vec3 euler);

  static constexpr Mat4 fromQuat(Quat q);

  constexpr float &operator()(std::size_t row, std::size_t col) {
    return m_data[col * 4 + row];
  }

  constexpr const float &operator()(std::size_t row, std::size_t col) const {
    return m_data[col * 4 + row];
  }

private:
  float m_data[16]{};
};

inline constexpr Mat4 Mat4::IDENTITY{
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
};

constexpr Mat4 operator*(const Mat4 &a, const Mat4 &b) {
  Mat4 result;

  for (std::size_t row = 0; row < 4; ++row) {
    for (std::size_t col = 0; col < 4; ++col) {
      float value = 0.0f;

      for (std::size_t k = 0; k < 4; ++k) {
        value += a(row, k) * b(k, col);
      }

      result(row, col) = value;
    }
  }

  return result;
}

constexpr Mat4 Mat4::rotEuler(Vec3 euler) {
  return Mat4::rotX(euler.x) * Mat4::rotY(euler.y) * Mat4::rotZ(euler.z);
}
