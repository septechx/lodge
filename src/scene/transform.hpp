#pragma once

#include "src/math/Mat4.hpp"
#include "src/math/Quat.hpp"
#include "src/math/Vec3.hpp"

struct Transform {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Quat rotation = Quat::IDENTITY;
  Vec3 scale{1.0f, 1.0f, 1.0f};

  Mat4 matrix() const {
    return Mat4::translate(position) * Mat4::fromQuat(rotation) *
           Mat4::scale(scale);
  }
};
