#pragma once

#include "src/render/pipelines/pipeline.hpp"

ComputePipeline createSsrPipeline(VkDevice device,
                                  VkDescriptorSetLayout frameLayout,
                                  VkDescriptorSetLayout passLayout);
