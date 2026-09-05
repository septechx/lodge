#pragma once

#include "src/render/pipelines/selection.hpp"

#include <cstdint>
#include <span>
#include <vector>

struct DrawItem {
  MaterialKind kind;
  bool doubleSided = false;
  uint32_t texIdx = 0;
};

struct Draw {
  uint32_t objectIdx = 0;
  PipelineId pipeline{};
  uint32_t texIdx = 0;
};

std::vector<Draw> groupDraws(std::span<const DrawItem> items, Pass pass);
