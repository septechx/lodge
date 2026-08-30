#pragma once

#include "src/asset/handles.hpp"
#include "src/consts.hpp"
#include "src/math/Vec3.hpp"
#include "src/scene/transform.hpp"

#include <optional>
#include <string>

struct CameraParams {
  float fovY = FOV_Y_DEGREES;
  float nearZ = 0.1f;
  float farZ = 20.0f;
};

struct LightParams {
  Vec3 color{1.0f, 1.0f, 1.0f};
};

struct ModelRenderer {
  ModelHandle model;
};

struct GameObject {
  uint32_t id = 0;
  std::string name;

  Transform transform;

  std::optional<ModelRenderer> renderer;
  std::optional<CameraParams> camera;
  std::optional<LightParams> light;
};
