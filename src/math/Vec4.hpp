#pragma once

#include <cmath>

struct Vec4 {
  float x, y, z, w;

  constexpr Vec4() = default;
  constexpr Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

  constexpr float dot(const Vec4 &other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
  }

  constexpr float length() const { return std::sqrt(dot(*this)); }

  constexpr Vec4 normalize() const {
    float len = length();
    return {
        x / len,
        y / len,
        z / len,
        w / len,
    };
  }

  constexpr Vec4 operator+(const Vec4 &other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z,
        w + other.w,
    };
  }

  constexpr Vec4 operator-(const Vec4 &other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z,
        w - other.w,
    };
  }

  constexpr Vec4 operator*(float scalar) const {
    return {
        x * scalar,
        y * scalar,
        z * scalar,
        w * scalar,
    };
  };
};
