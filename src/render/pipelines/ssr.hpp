#pragma once

#include "src/render/pipelines/pipeline.hpp"

GraphicsPipeline createOpaqueGrabPipeline(VkDevice device, VkFormat colorFormat,
                                          VkFormat normalFormat,
                                          VkFormat depthFormat,
                                          const VkExtent2D &extent,
                                          VkDescriptorSetLayout setLayout);

GraphicsPipeline createOpaqueCompPipeline(VkDevice device, VkFormat colorFormat,
                                          VkFormat depthFormat,
                                          const VkExtent2D &extent,
                                          VkDescriptorSetLayout setLayout);

GraphicsPipeline createSkyGrabPipeline(VkDevice device, VkFormat colorFormat,
                                       VkFormat normalFormat,
                                       VkFormat depthFormat,
                                       const VkExtent2D &extent,
                                       VkDescriptorSetLayout setLayout);

ComputePipeline createSsrPipeline(VkDevice device,
                                  VkDescriptorSetLayout setLayout);
