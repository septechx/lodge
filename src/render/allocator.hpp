#pragma once

#include <vulkan/vulkan.h>

struct AllocatedBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

AllocatedBuffer createBuffer(VkDevice device, VkPhysicalDevice physical,
                             VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags props);

struct AllocatedImage {
  VkImage image;
  VkDeviceMemory memory;
};

AllocatedImage createImage(VkDevice device, VkPhysicalDevice physical,
                           uint32_t width, uint32_t height, VkFormat format,
                           VkImageUsageFlags usage);
