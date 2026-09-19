#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

extern const uint32_t k_opaqueVert[];
extern const std::size_t k_opaqueVertWordCount;
extern const uint32_t k_opaqueFrag[];
extern const std::size_t k_opaqueFragWordCount;
extern const uint32_t k_opaqueGrabFrag[];
extern const std::size_t k_opaqueGrabFragWordCount;
extern const uint32_t k_opaqueCompFrag[];
extern const std::size_t k_opaqueCompFragWordCount;
extern const uint32_t k_transparentVert[];
extern const std::size_t k_transparentVertWordCount;
extern const uint32_t k_transparentFrag[];
extern const std::size_t k_transparentFragWordCount;
extern const uint32_t k_skyVert[];
extern const std::size_t k_skyVertWordCount;
extern const uint32_t k_skyFrag[];
extern const std::size_t k_skyFragWordCount;
extern const uint32_t k_ssrComp[];
extern const std::size_t k_ssrCompWordCount;

inline std::span<const uint32_t> embeddedWords(const uint32_t *data,
                                               std::size_t wordCount) {
  return {data, wordCount};
}

#define LODGE_SHADER_WORDS(name) embeddedWords(name, name##WordCount)
