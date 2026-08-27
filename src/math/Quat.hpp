#pragma once

#include "Mat4.hpp"
#include "Vec3.hpp"

#include <cmath>

struct Quat {
  float w, x, y, z;

  static const Quat IDENTITY;

  constexpr Quat() = default;
  constexpr Quat(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

  static constexpr Quat fromAxisAngle(Vec3 axis, float rad) {
    Vec3 naxis = axis.normalize();
    float hrad = rad / 2;
    float s = std::sin(hrad);
    return {std::cos(hrad), naxis.x * s, naxis.y * s, naxis.z * s};
  }

  static constexpr Quat fromEuler(Vec3 euler) {
    float cx = std::cos(euler.x / 2), sx = std::sin(euler.x / 2);
    float cy = std::cos(euler.y / 2), sy = std::sin(euler.y / 2);
    float cz = std::cos(euler.z / 2), sz = std::sin(euler.z / 2);
    Quat qx{cx, sx, 0, 0}, qy{cy, 0, sy, 0}, qz{cz, 0, 0, sz};
    return qx * qy * qz;
  }

  constexpr float length() const {
    return std::sqrt(w * w + x * x + y * y + z * z);
  }

  constexpr Quat normalize() const {
    float l = length();
    return {w / l, x / l, y / l, z / l};
  }

  constexpr Quat conjugate() const { return {w, -x, -y, -z}; }

  constexpr Quat operator*(Quat r) const {
    return {
        w * r.w - x * r.x - y * r.y - z * r.z,
        w * r.x + x * r.w + y * r.z - z * r.y,
        w * r.y - x * r.z + y * r.w + z * r.x,
        w * r.z + x * r.y - y * r.x + z * r.w,
    };
  }

  constexpr Vec3 rotate(Vec3 v) const {
    Quat qv{0, v.x, v.y, v.z};
    Quat res = *this * qv * conjugate();
    return {res.x, res.y, res.z};
  }
};

inline constexpr Quat Quat::IDENTITY{1, 0, 0, 0};

constexpr Mat4 Mat4::fromQuat(Quat q) {
  Quat nq = q.normalize();
  float xx = nq.x * nq.x, yy = nq.y * nq.y, zz = nq.z * nq.z;
  float xy = nq.x * nq.y, xz = nq.x * nq.z, yz = nq.y * nq.z;
  float wx = nq.w * nq.x, wy = nq.w * nq.y, wz = nq.w * nq.z;
  return {
      {1 - 2 * (yy + zz), 2 * (xy + wz), 2 * (xz - wy), 0},
      {2 * (xy - wz), 1 - 2 * (xx + zz), 2 * (yz + wx), 0},
      {2 * (xz + wy), 2 * (yz - wx), 1 - 2 * (xx + yy), 0},
      {0, 0, 0, 1},
  };
}
