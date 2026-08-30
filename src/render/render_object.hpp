#pragma once

#include "src/math/Mat4.hpp"
#include "src/render/allocator.hpp"
#include "src/scene/material.hpp"

struct RenderObject {
  Mat4 worldMat;
  AllocatedBuffer vbuf;
  AllocatedBuffer ibuf;
  uint32_t indexCount;
  VkIndexType indexType;
  Material material;
};
