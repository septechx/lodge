#include "selection.hpp"

std::optional<PipelineId> pipelineFor(MaterialKind kind, Pass pass) {
  if (kind == MaterialKind::Transparent) {
    if (pass == Pass::Main) {
      return PipelineId::Transparent;
    }
    return std::nullopt;
  }
  switch (pass) {
  case Pass::Grab:
    return PipelineId::OpaqueGrab;
  case Pass::Main:
    return PipelineId::OpaqueComp;
  case Pass::Bake:
    return PipelineId::OpaqueBake;
  }
  return std::nullopt;
}

Cull cullFor(bool doubleSided) { return doubleSided ? Cull::None : Cull::Back; }
