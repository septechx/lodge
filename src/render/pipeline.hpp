#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "../asset/texture/load.hpp"
#include "../math/Mat3.hpp"
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

struct LightData {
  Vec3 lightPos;
  float _pad0;
  Vec3 lightColor;
  float _pad1;
  Vec3 viewPos;
  float _pad2;
};

struct PushConstants {
  Mat4 model;
  Mat4 normal;
};

constexpr Mat4 mat4FromNormalMat3(const Mat3 &n) {
  Mat4 r = Mat4::IDENTITY;
  r(0, 0) = n(0, 0);
  r(1, 0) = n(1, 0);
  r(2, 0) = n(2, 0);
  r(0, 1) = n(0, 1);
  r(1, 1) = n(1, 1);
  r(2, 1) = n(2, 1);
  r(0, 2) = n(0, 2);
  r(1, 2) = n(1, 2);
  r(2, 2) = n(2, 2);
  return r;
}

struct LightUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  LightData *mapped;
};

LightUniformBuffer createLightUniformBuffer(VkDevice device,
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
                                        CameraUniformBuffer *cameras,
                                        LightUniformBuffer *lights);

struct GraphicsPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
};

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
