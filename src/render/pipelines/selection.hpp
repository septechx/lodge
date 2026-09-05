#pragma once

#include "src/scene/material.hpp"

#include <optional>

enum class Pass {
  Grab,
  Main,
  Bake,
};

enum class PipelineId {
  Sky,
  OpaqueGrab,
  OpaqueComp,
  OpaqueBake,
  Transparent,
};

enum class Cull {
  None,
  Back,
};

std::optional<PipelineId> pipelineFor(MaterialKind kind, Pass pass);

Cull cullFor(bool doubleSided);
