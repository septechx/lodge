#pragma once

#include "src/render/pipelines/pipeline.hpp"

GraphicsPipeline createOpaquePipeline(VkDevice device, VkFormat colorFormat,
                                      VkFormat depthFormat,
                                      const VkExtent2D &extent,
                                      VkDescriptorSetLayout setLayout);
