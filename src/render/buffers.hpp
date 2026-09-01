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

struct LightUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  LightData *mapped;
};

LightUniformBuffer createLightUniformBuffer(Device device);

#define MAX_MATERIALS 512

struct MaterialData {
  Vec4 baseColor;

  // Maybe move this to transparent-specific descriptor?
  float thickness;
  float ior;

  float _pad[2];
};

struct MaterialsBlock {
  MaterialData data[MAX_MATERIALS];
};

struct MaterialUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  MaterialsBlock *mapped;
};

MaterialUniformBuffer createMaterialUniformBuffer(Device device);

struct PushConstants {
  Mat4 model;
  uint32_t materialIdx;
  uint32_t _pad0;
  uint32_t _pad1;
  uint32_t _pad2;
};

struct DepthBuffer {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkFormat format;
};

DepthBuffer createDepthBuffer(Device device, VkFormat format, uint32_t width,
                              uint32_t height);

struct SceneGrab {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

SceneGrab createSceneGrab(Device device, VkFormat format, uint32_t width,
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

SceneDescriptors
createSceneDescriptors(VkDevice device, const std::vector<Texture> &textures,
                       CameraUniformBuffer *cameras, LightUniformBuffer *lights,
                       MaterialUniformBuffer *materials, VkSampler sceneSampler,
                       VkImageView sceneView);
