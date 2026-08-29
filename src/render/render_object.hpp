#pragma once

#include "src/math/Mat4.hpp"
#include "src/math/Vec4.hpp"
#include "src/render/allocator.hpp"

struct Material {
  uint32_t textureIndex = 0;
  Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  bool doubleSided = false;
  bool isOpaque = true;
};

struct RenderObject {
  Mat4 worldMat;
  AllocatedBuffer vbuf;
  AllocatedBuffer ibuf;
  uint32_t indexCount;
  VkIndexType indexType;
  Material material;
};
