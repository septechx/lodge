#pragma once

#include "src/math/Mat4.hpp"
#include "src/math/Vec3.hpp"

#include <algorithm>

inline Mat4 boxWorldForFallback(const Mat4 &worldMat, Vec3 localMin,
                                Vec3 localMax) {
  float kMinHalfExtent = 1e-4f;
  Vec3 center{
      (localMin.x + localMax.x) * 0.5f,
      (localMin.y + localMax.y) * 0.5f,
      (localMin.z + localMax.z) * 0.5f,
  };
  Vec3 half{
      (localMax.x - localMin.x) * 0.5f,
      (localMax.y - localMin.y) * 0.5f,
      (localMax.z - localMin.z) * 0.5f,
  };
  half.x = std::max(half.x, kMinHalfExtent);
  half.y = std::max(half.y, kMinHalfExtent);
  half.z = std::max(half.z, kMinHalfExtent);
  return worldMat * Mat4::translate(center) * Mat4::scale(half);
}
