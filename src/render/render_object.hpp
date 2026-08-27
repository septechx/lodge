#pragma once

#include "../math/Mat4.hpp"
#include "../math/Vec4.hpp"
#include "allocator.hpp"

struct RenderObject {
  Mat4 worldMat;
  AllocatedBuffer vbuf;
  AllocatedBuffer ibuf;
  uint32_t indexCount;
  VkIndexType indexType;
  uint32_t textureIndex = 0;
  Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  bool doubleSided = false;
};
