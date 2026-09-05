#pragma once

#include "src/render/pipelines/pipeline.hpp"

#include <span>

GraphicsPipeline
createOpaquePipeline(VkDevice device, VkFormat colorFormat,
                     VkFormat depthFormat,
                     std::span<const VkDescriptorSetLayout> setLayouts);

GraphicsPipeline
createOpaqueGrabPipeline(VkDevice device, VkFormat colorFormat,
                         VkFormat normalFormat, VkFormat depthFormat,
                         std::span<const VkDescriptorSetLayout> setLayouts);

GraphicsPipeline
createOpaqueCompPipeline(VkDevice device, VkFormat colorFormat,
                         VkFormat depthFormat,
                         std::span<const VkDescriptorSetLayout> setLayouts);
