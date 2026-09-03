#pragma once

#include "src/math/Vec3.hpp"

#define CUBE_SIZE 256

struct CubeFace {
  int layer;
  Vec3 fwd;
  Vec3 up;
};

static const CubeFace CUBE_FACES[6] = {
    {0, {1, 0, 0}, {0, -1, 0}}, {1, {-1, 0, 0}, {0, -1, 0}},
    {2, {0, 1, 0}, {0, 0, 1}},  {3, {0, -1, 0}, {0, 0, -1}},
    {4, {0, 0, 1}, {0, -1, 0}}, {5, {0, 0, -1}, {0, -1, 0}},
};
