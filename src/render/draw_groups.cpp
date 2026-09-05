#include "src/render/draw_groups.hpp"

#include <algorithm>

std::vector<Draw> groupDraws(std::span<const DrawItem> items, Pass pass) {
  std::vector<Draw> out;
  out.reserve(items.size());
  for (uint32_t i = 0; i < items.size(); ++i) {
    auto id = pipelineFor(items[i].kind, pass);
    if (!id.has_value()) {
      continue;
    }
    out.push_back(Draw{
        .objectIdx = i,
        .pipeline = *id,
        .texIdx = items[i].texIdx,
    });
  }
  std::stable_sort(out.begin(), out.end(), [](const Draw &a, const Draw &b) {
    if (a.pipeline != b.pipeline) {
      return a.pipeline < b.pipeline;
    }
    if (a.texIdx != b.texIdx) {
      return a.texIdx < b.texIdx;
    }
    return a.objectIdx < b.objectIdx;
  });
  return out;
}
