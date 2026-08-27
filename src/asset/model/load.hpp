#pragma once

#include "../../render/init.hpp"
#include "../../render/render_object.hpp"

#include <filesystem>

std::vector<RenderObject> loadModel(const Device &dev,
                                    std::filesystem::path path);
