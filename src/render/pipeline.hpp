#pragma once

#include <vulkan/vulkan.h>

#include "../consts.hpp"
#include "../math/Mat4.hpp"
#include "allocator.hpp"
#include "texture.hpp"

AllocatedBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physical,
                                   const void *data, VkDeviceSize size);

AllocatedBuffer createIndexBuffer(VkDevice device, VkPhysicalDevice physical,
                                  const void *data, VkDeviceSize size);

struct UBO {
  Mat4 viewproj;
};

struct UniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  UBO *mapped;
};

UniformBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physical);

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
  VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
};

SceneDescriptors createSceneDescriptors(VkDevice device, Texture tex,
                                        UniformBuffer *ubos);

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
