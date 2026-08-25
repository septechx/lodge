#pragma once

#include "../consts.hpp"
#include "../math/Mat4.hpp"
#include "allocator.hpp"
#include "texture.hpp"
#include <vulkan/vulkan.h>

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
  VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
};

SceneDescriptors createSceneDescriptors(VkDevice device, Texture tex,
                                        CameraUniformBuffer *cameras);

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
