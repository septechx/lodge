#pragma once

#include "../../asset/texture/load.hpp"
#include "../../render/render_object.hpp"

#include <filesystem>
#include <vector>

struct LoadedModel {
  std::vector<RenderObject> objects;
  std::vector<Texture> textures;
};

LoadedModel loadModel(const Device &dev, std::filesystem::path path);
