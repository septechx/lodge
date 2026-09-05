#pragma once

#include "src/scene/material.hpp"

#include <cstdint>
#include <vector>

// Deepened Material module (C5): deduped store keyed on the full Material
// including texture. Device-free; Renderer writes the deduped array once,
// ObjectStore (per-object cubeInv + materialId) is updated per frame.
bool materialsEqual(const Material &a, const Material &b);

class MaterialStore {
public:
  // Returns stable id for m, interning on first sight.
  uint32_t intern(const Material &m);
  size_t size() const { return m_unique.size(); }
  const Material &at(uint32_t id) const { return m_unique.at(id); }

private:
  std::vector<Material> m_unique;
};
