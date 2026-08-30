#pragma once

#include "src/consts.hpp"
#include "src/math/Quat.hpp"
#include "src/render/render_object.hpp"

#include <optional>
#include <span>
#include <vector>

class AssetStore;
class Scene;

struct FrameLight {
  Vec3 pos;
  Vec3 color;
};

struct FrameCamera {
  Vec3 position{0.0f, 1.0f, 1.0f};
  Quat rotation = Quat::fromEuler(Vec3{-LDG_PI / 4.0f, 0.0f, 0.0f});
  float fovY = FOV_Y_DEGREES;
  float nearZ = 0.1f;
  float farZ = 20.0f;
};

struct FrameScene {
  std::optional<FrameCamera> camera;
  std::span<const FrameLight> lights;
  std::span<const RenderObject> objects;
};

FrameScene gatherFrameScene(const Scene &scene, const AssetStore &assets,
                            std::vector<RenderObject> &objectsOut,
                            std::vector<FrameLight> &lightsOut);
