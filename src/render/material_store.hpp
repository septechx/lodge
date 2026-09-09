#pragma once

#include "src/scene/material.hpp"

#include <cstdint>
#include <span>
#include <vector>

bool materialsEqual(const Material &a, const Material &b);

std::vector<Material>
collectUniqueMaterials(std::span<const Material> materials);
bool materialsContain(std::span<const Material> keys, const Material &m);
bool uniqueMaterialsNeedRebuild(std::span<const Material> keys,
                                std::span<const Material> currentUnique);

class MaterialStore {
public:
  uint32_t intern(const Material &m);
  size_t size() const { return m_unique.size(); }
  const Material &at(uint32_t id) const { return m_unique.at(id); }

private:
  std::vector<Material> m_unique;
};
