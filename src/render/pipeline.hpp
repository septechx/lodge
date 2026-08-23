#pragma once

#include <vulkan/vulkan.h>

#include <cstring>

#include "../math/Mat4.hpp"
#include "src/consts.hpp"
#include "texture.hpp"
#include "utils.hpp"

struct VertexBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

template <typename T>
VertexBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physical,
                                const T &data) {
  VkBufferCreateInfo bci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizeof(T),
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  VkBuffer buffer;
  CHECK_VK(vkCreateBuffer(device, &bci, nullptr, &buffer),
           "create vertex buffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, buffer, &req);
  uint32_t type = findMemoryType(physical, req.memoryTypeBits,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkMemoryAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = type,
  };
  VkDeviceMemory memory;
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory),
           "alloc vertex memory");
  CHECK_VK(vkBindBufferMemory(device, buffer, memory, 0),
           "bind vertex buffer memory");

  void *dst = nullptr;
  CHECK_VK(vkMapMemory(device, memory, 0, sizeof(T), 0, &dst),
           "map vertex memory");
  memcpy(dst, data, sizeof(T));
  vkUnmapMemory(device, memory);

  return VertexBuffer{buffer, memory};
}

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
