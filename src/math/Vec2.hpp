#pragma once

#include <cmath>

struct Vec2 {
  float x, y;

  constexpr Vec2() = default;
  constexpr Vec2(float x, float y) : x(x), y(y) {}

  constexpr float dot(const Vec2 &other) const {
    return x * other.x + y * other.y;
  }

  constexpr float length() const { return std::sqrt(dot(*this)); }

  constexpr Vec2 normalize() const {
    float len = length();
    return {
        x / len,
        y / len,
    };
  }

  constexpr Vec2 operator+(const Vec2 &other) const {
    return {
        x + other.x,
        y + other.y,
    };
  }

  constexpr Vec2 operator-(const Vec2 &other) const {
    return {
        x - other.x,
        y - other.y,
    };
  }

  constexpr Vec2 operator*(float scalar) const {
    return {
        x * scalar,
        y * scalar,
    };
  }
};

constexpr Vec2 operator*(float scalar, const Vec2 &v) { return v * scalar; }
