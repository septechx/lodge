#pragma once

#include <filesystem>

class AssetStore;
class Scene;

bool loadSceneFromFile(Scene &scene, AssetStore &assets,
                       const std::filesystem::path &scenePath);

bool saveSceneToFile(const Scene &scene, const AssetStore &assets,
                     const std::filesystem::path &scenePath);
