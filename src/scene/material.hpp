#pragma once

#include "src/asset/handles.hpp"
#include "src/math/Vec4.hpp"

enum class MaterialKind {
  Opaque,
  Transparent,
};

struct Material {
  TextureHandle texture;
  Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
  bool doubleSided = false;
  MaterialKind kind = MaterialKind::Opaque;
};
