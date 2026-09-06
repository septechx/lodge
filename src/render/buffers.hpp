#pragma once

#include "src/asset/texture/load.hpp"
#include "src/math/Mat4.hpp"
#include "src/math/Vec4.hpp"
#include "src/render/allocator.hpp"
#include "src/scene/material.hpp"

#include <span>
#include <vector>

#define MAX_ENVS 16
#define MAX_PROBE_BOXES 16

// TODO: Instead of `createX` we should have constructors

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

  float thickness;
  float ior;

  float metallic;
  float roughness;
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

struct ObjectData {
  Mat4 cubeInv;
  uint32_t materialId = 0;
  float _pad[3]{};
};

struct ObjectsBlock {
  ObjectData data[MAX_MATERIALS];
};

struct ObjectUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  ObjectsBlock *mapped;
};

ObjectUniformBuffer createObjectUniformBuffer(Device device);

struct PushConstants {
  Mat4 model;
  uint32_t objectIdx;
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

VkFormat findDepthFormat(VkPhysicalDevice physical);

DepthBuffer createDepthBuffer(Device device, VkFormat format, uint32_t width,
                              uint32_t height, bool sampled = false);

struct SceneGrab {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

SceneGrab createSceneGrab(Device device, VkFormat format, uint32_t width,
                          uint32_t height);

#define GRAB_NORMAL_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT
SceneGrab createFloatTarget(Device device, VkFormat format, uint32_t width,
                            uint32_t height);

#define SSR_FORMAT VK_FORMAT_R16G16B16A16_SFLOAT
struct SsrTarget {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

SsrTarget createSsrTarget(Device device, uint32_t width, uint32_t height);

#define SSR_MAXDIST 6.0f
#define SSR_THICKNESS 0.03f
#define SSR_STEPS 24
#define SSR_REFINE 6

struct SsrData {
  Mat4 viewProj;
  Mat4 invViewProj;
  Mat4 view;
  Vec4 camPos;
  Vec4 march; // maxDist, thickness, steps, refine
};

struct SsrUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  SsrData *mapped;
};

SsrUniformBuffer createSsrUniformBuffer(Device device);

struct ProbeData {
  Vec4 probe;
  uint32_t count = 0;
  float _pad[3] = {};
};

struct ProbeUniformBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  ProbeData *mapped;
};

ProbeUniformBuffer createProbeUniformBuffer(Device device);

struct EnvCube {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView cubeView;
  VkImageView faceViews[6];
};

EnvCube createEnvCube(Device device, VkFormat format);
void destroyEnvCube(VkDevice device, EnvCube &env);

struct FrameSets {
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorPool pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> sets;
  std::vector<VkDescriptorSet> bakeSets;
};

struct MaterialSets {
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorPool pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> sets;
  std::vector<VkDescriptorSet> bakeSets;
  uint32_t materialCount = 0;
  std::vector<Material> keys;

  VkDescriptorSet get(uint32_t frame, uint32_t matIdx) const {
    return sets[frame * materialCount + matIdx];
  }
  VkDescriptorSet getBake(uint32_t matIdx) const { return bakeSets[matIdx]; }
  uint32_t find(const Material &m) const;
};

struct EnvSets {
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorPool pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> sets;
};

struct PassSets {
  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  VkDescriptorPool pool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> sets;
};

struct SplitDescriptors {
  FrameSets frame;
  MaterialSets material;
  EnvSets env;
  PassSets pass;
  uint32_t textureCount = 0;
  uint32_t envCount = 0;
};

SplitDescriptors createSplitDescriptors(
    VkDevice device, const std::vector<Texture> &textures,
    std::span<const Material> uniqueMaterials, CameraUniformBuffer *cameras,
    LightUniformBuffer *lights, MaterialUniformBuffer *materials,
    ObjectUniformBuffer *objects, SsrUniformBuffer *ssrUbos,
    std::span<const ProbeUniformBuffer> probes,
    const CameraUniformBuffer &bakeCamera, const LightUniformBuffer &bakeLights,
    const MaterialUniformBuffer &bakeMaterials,
    const ObjectUniformBuffer &bakeObjects, VkSampler sceneSampler,
    VkImageView sceneView, VkSampler envSampler,
    std::span<const VkImageView> envViews, VkSampler grabSampler,
    VkImageView normalView, VkSampler depthSampler, VkImageView depthView,
    VkSampler ssrSampler, VkImageView ssrView);

void updatePassResizeDescriptors(VkDevice device, SplitDescriptors &descriptors,
                                 VkSampler grabSampler, VkImageView normalView,
                                 VkSampler depthSampler, VkImageView depthView,
                                 VkSampler ssrSampler, VkImageView ssrView,
                                 VkSampler sceneSampler, VkImageView sceneView);

void destroySplitDescriptors(VkDevice device, SplitDescriptors &descriptors);
