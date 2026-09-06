#pragma once

#include "src/asset/handles.hpp"
#include "src/math/Vec4.hpp"

enum class MaterialKind {
  Opaque,
  Transparent,
};

struct Material {
  TextureHandle texture;
  TextureHandle metallicRoughness{0};
  TextureHandle normal{0};
  Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  bool doubleSided = false;
  MaterialKind kind = MaterialKind::Opaque;
  float metallicFactor = 0.0f;
  float roughnessFactor = 1.0f;
};
