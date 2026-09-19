#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>
#include <span>

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct GraphicsPipelines {
  GraphicsPipeline opaque;
  GraphicsPipeline opaqueGrab;
  GraphicsPipeline opaqueComp;
  GraphicsPipeline transparent;
  GraphicsPipeline sky;
  GraphicsPipeline skyGrab;
};

struct ComputePipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct ShaderModules {
  VkShaderModule vert, frag;
};

ShaderModules loadShaders(VkDevice device, const std::filesystem::path &vert,
                          const std::filesystem::path &frag);

void loadShader(VkDevice device, const std::filesystem::path path,
                VkShaderModule &module);

void loadShaderFromWords(VkDevice device, std::span<const uint32_t> words,
                         VkShaderModule &module);

ShaderModules loadShadersFromWords(VkDevice device,
                                   std::span<const uint32_t> vert,
                                   std::span<const uint32_t> frag);
