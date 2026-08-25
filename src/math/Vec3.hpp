#pragma once

#include <cmath>

struct Vec3 {
  float x, y, z;

  constexpr Vec3() = default;
  constexpr Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

  constexpr float dot(const Vec3 &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  constexpr Vec3 cross(const Vec3 &other) const {
    return {
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x,
    };
  }

  constexpr float length() const { return std::sqrt(dot(*this)); }

  constexpr Vec3 normalize() const {
    float len = length();
    return {
        x / len,
        y / len,
        z / len,
    };
  }

  constexpr Vec3 operator+(const Vec3 &other) const {
    return {
        x + other.x,
        y + other.y,
        z + other.z,
    };
  }

  constexpr Vec3 operator-(const Vec3 &other) const {
    return {
        x - other.x,
        y - other.y,
        z - other.z,
    };
  }

  constexpr Vec3 operator*(float scalar) const {
    return {
        x * scalar,
        y * scalar,
        z * scalar,
    };
  }
};

constexpr Vec3 operator*(float scalar, const Vec3 &v) { return v * scalar; }
