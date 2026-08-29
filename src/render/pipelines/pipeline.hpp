#pragma once

#include <vulkan/vulkan.h>

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

struct ShaderModules {
  VkShaderModule vert, frag;
};

ShaderModules loadShaders(VkDevice device);
