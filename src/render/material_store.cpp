#include "src/render/material_store.hpp"

bool materialsEqual(const Material &a, const Material &b) {
  return a.texture == b.texture &&
         a.metallicRoughness == b.metallicRoughness && a.normal == b.normal &&
         a.baseColorFactor.x == b.baseColorFactor.x &&
         a.baseColorFactor.y == b.baseColorFactor.y &&
         a.baseColorFactor.z == b.baseColorFactor.z &&
         a.baseColorFactor.w == b.baseColorFactor.w &&
         a.doubleSided == b.doubleSided && a.kind == b.kind &&
         a.metallicFactor == b.metallicFactor &&
         a.roughnessFactor == b.roughnessFactor;
}

uint32_t MaterialStore::intern(const Material &m) {
  for (uint32_t i = 0; i < m_unique.size(); ++i) {
    if (materialsEqual(m_unique[i], m)) {
      return i;
    }
  }
  m_unique.push_back(m);
  return static_cast<uint32_t>(m_unique.size() - 1);
}
