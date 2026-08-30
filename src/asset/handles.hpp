#pragma once

#include <cstdint>

struct MeshHandle {
  uint32_t index = 0;

  constexpr bool operator==(const MeshHandle &) const = default;
};

struct TextureHandle {
  uint32_t index = 0;

  constexpr bool operator==(const TextureHandle &) const = default;
};

struct ModelHandle {
  uint32_t index = 0;

  constexpr bool operator==(const ModelHandle &) const = default;
};
