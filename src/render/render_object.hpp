#pragma once

#include "../math/Mat4.hpp"
#include "allocator.hpp"

struct RenderObject {
  Mat4 worldMat;
  AllocatedBuffer vbuf;
  AllocatedBuffer ibuf;
  uint32_t indexCount;
  VkIndexType indexType;
};
