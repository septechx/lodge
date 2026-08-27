#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "../asset/texture/load.hpp"
#include "../math/Mat4.hpp"
#include "allocator.hpp"

AllocatedBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physical,
                                   const void *data, VkDeviceSize size);

AllocatedBuffer createIndexBuffer(VkDevice device, VkPhysicalDevice physical,
                                  const void *data, VkDeviceSize size);

struct CameraData {
  Mat4 viewproj;
};

struct CameraUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  CameraData *mapped;
};

CameraUniformBuffer createCameraUniformBuffer(VkDevice device,
                                              VkPhysicalDevice physical);

struct DepthBuffer {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkFormat format;
};

DepthBuffer createDepthBuffer(VkDevice device, VkPhysicalDevice physical,
                              VkFormat format, uint32_t width, uint32_t height);

struct SceneDescriptors {
  VkDescriptorSetLayout layout;
  VkDescriptorPool pool;
  std::vector<VkDescriptorSet> sets;
  uint32_t textureCount = 0;

  VkDescriptorSet get(uint32_t frame, uint32_t texIdx) const {
    return sets[frame * textureCount + texIdx];
  }
};

SceneDescriptors createSceneDescriptors(VkDevice device,
                                        const std::vector<Texture> &textures,
                                        CameraUniformBuffer *cameras);

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
