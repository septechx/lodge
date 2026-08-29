#pragma once

#include "src/consts.hpp"
#include "src/math/Mat4.hpp"
#include "src/math/Vec3.hpp"

struct Camera {
  Vec3 eye{0.0f, 1.0f, 1.0f};
  Vec3 center{0.0f, 0.0f, 0.0f};
  Vec3 up{0.0f, 1.0f, 0.0f};
  float fovY = FOV_Y_DEGREES;
  float nearZ = 0.1f;
  float farZ = 20.0f;

  Mat4 getViewProj(float aspect) const {
    Mat4 view = Mat4::lookAt(eye, center, up);
    Mat4 proj = Mat4::perspective(fovY, aspect, nearZ, farZ);
    return proj * view;
  }

  void reset() { *this = Camera{}; }
};
