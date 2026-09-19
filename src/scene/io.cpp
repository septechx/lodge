#include "src/scene/io.hpp"

#include "src/asset/store.hpp"
#include "src/scene/scene.hpp"
#include "src/serialize/json.hpp"
#include "src/utils.hpp"

#include <spdlog/spdlog.h>

#include <fstream>

bool loadSceneFromFile(Scene &scene, AssetStore &assets,
                       const std::filesystem::path &scenePath) {

  auto text = readFileToString(scenePath);
  if (!text.has_value()) {
    spdlog::error("cannot open scene file {}", scenePath.string());
    return false;
  }

  auto value = ser::parseJson(*text);
  if (!value.has_value()) {
    spdlog::error("scene {}: JSON parse failed", scenePath.string());
    return false;
  }

  const std::filesystem::path baseDir = scenePath.parent_path();

  auto handleFor =
      [&](const std::string &modelRef) -> std::optional<ModelHandle> {
    if (modelRef == Scene::BUILTIN_GIZMO)
      return assets.gizmoModel();

    std::filesystem::path refPath(modelRef);
    std::filesystem::path candidate;
    if (refPath.is_absolute()) {
      candidate = refPath;
    } else if (!baseDir.empty()) {
      const std::filesystem::path sceneRelative = baseDir / refPath;
      std::error_code ec;
      if (std::filesystem::exists(sceneRelative, ec))
        candidate = sceneRelative;
      else if (std::filesystem::exists(refPath, ec))
        candidate = refPath;
      else
        candidate = sceneRelative;
    } else {
      candidate = refPath;
    }

    std::error_code ec;
    if (!std::filesystem::exists(candidate, ec)) {
      spdlog::warn("scene {}: model '{}' not found (tried '{}'), skipping "
                   "renderer",
                   scenePath.string(), modelRef, candidate.string());
      return std::nullopt;
    }

    return assets.loadModelCached(candidate);
  };

  scene.deserializeWithModels(*value, handleFor);

  for (GameObject &object : scene.objects()) {
    for (ScriptRef &ref : object.scripts) {
      std::filesystem::path refPath(ref.path);
      if (refPath.is_absolute() || baseDir.empty())
        continue;
      const std::filesystem::path sceneRelative = baseDir / refPath;
      std::error_code ec;
      if (std::filesystem::exists(sceneRelative, ec)) {
        ref.path = sceneRelative.lexically_normal().generic_string();
      } else if (std::filesystem::exists(refPath, ec)) {
        continue;
      } else {
        ref.path = sceneRelative.lexically_normal().generic_string();
      }
    }
  }

  spdlog::info("loaded scene {} with {} objects", scenePath.string(),
               scene.objects().size());
  return true;
}

bool saveSceneToFile(const Scene &scene, const AssetStore &assets,
                     const std::filesystem::path &scenePath) {
  auto pathFor = [&](ModelHandle handle) -> std::optional<std::string> {
    if (handle == assets.gizmoModel())
      return std::string(Scene::BUILTIN_GIZMO);
    return assets.pathOf(handle);
  };

  const std::string text = ser::toJson(scene.serializeWithModels(pathFor));

  std::ofstream out(scenePath);
  if (!out) {
    spdlog::error("cannot write scene file {}", scenePath.string());
    return false;
  }
  out << text;
  return static_cast<bool>(out);
}
