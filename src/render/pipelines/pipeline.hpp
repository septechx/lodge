#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>

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
