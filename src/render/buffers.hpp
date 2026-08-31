#pragma once

#include "src/asset/texture/load.hpp"
#include "src/math/Mat4.hpp"
#include "src/math/Vec4.hpp"
#include "src/render/allocator.hpp"

#include <vector>

AllocatedBuffer createVertexBuffer(Device device, const void *data,
                                   VkDeviceSize size);

AllocatedBuffer createIndexBuffer(Device device, const void *data,
                                  VkDeviceSize size);

struct CameraData {
  Mat4 viewProj;
  Vec3 viewPos;
  float _pad0;
};

struct CameraUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  CameraData *mapped;
};

CameraUniformBuffer createCameraUniformBuffer(Device device);

struct LightData {
  Vec3 lightPos;
  float _pad0;
  Vec3 lightColor;
  float _pad1;
};

struct PushConstants {
  Mat4 model;
  Vec4 normal0;
  Vec4 normal1;
  Vec4 normal2;
  Vec4 baseColor;
};

struct LightUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  LightData *mapped;
};

LightUniformBuffer createLightUniformBuffer(Device device);

struct DepthBuffer {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkFormat format;
};

DepthBuffer createDepthBuffer(Device device, VkFormat format, uint32_t width,
                              uint32_t height);

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
