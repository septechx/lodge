#pragma once

#include "src/asset/store.hpp"

#include <filesystem>

ModelHandle loadModel(AssetStore &store, std::filesystem::path path);
