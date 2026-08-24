#pragma once

#include <vulkan/vulkan.h>

#include "../math/Mat4.hpp"
#include "src/consts.hpp"
#include "texture.hpp"

struct VertexBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

VertexBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physical,
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

GraphicsPipeline createPipeline(VkDevice device, VkFormat format,
                                const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout);
