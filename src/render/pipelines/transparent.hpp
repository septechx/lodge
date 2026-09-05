#pragma once

#include "src/render/pipelines/pipeline.hpp"

#include <span>

GraphicsPipeline
createTransparentPipeline(VkDevice device, VkFormat colorFormat,
                          VkFormat depthFormat,
                          std::span<const VkDescriptorSetLayout> setLayouts);
