#pragma once

#include "../math/Quat.hpp"
#include "../math/Vec3.hpp"

struct RenderObject {
  Vec3 pos;
  Quat rotation;
  float scale;
};
