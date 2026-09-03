#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct GraphicsPipelines {
  GraphicsPipeline opaque;
  GraphicsPipeline transparent;
  GraphicsPipeline sky;
};

struct ShaderModules {
  VkShaderModule vert, frag;
};

ShaderModules loadShaders(VkDevice device, const std::filesystem::path &vert,
                          const std::filesystem::path &frag);
