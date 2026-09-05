#pragma once

#include "src/render/pipelines/pipeline.hpp"

#include <span>

GraphicsPipeline
createSkyPipeline(VkDevice device, VkFormat colorFormat, VkFormat depthFormat,
                  std::span<const VkDescriptorSetLayout> setLayouts);

GraphicsPipeline
createSkyGrabPipeline(VkDevice device, VkFormat colorFormat,
                      VkFormat normalFormat, VkFormat depthFormat,
                      std::span<const VkDescriptorSetLayout> setLayouts);
