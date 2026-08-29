#pragma once

#include <vulkan/vulkan.h>

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
