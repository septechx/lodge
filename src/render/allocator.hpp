#pragma once

#include "src/render/device.hpp"

#include <vulkan/vulkan.h>

struct AllocatedBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

AllocatedBuffer createBuffer(Device device, VkDeviceSize size,
                             VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags props);

struct AllocatedImage {
  VkImage image;
  VkDeviceMemory memory;
};

AllocatedImage createImage(Device device, uint32_t width, uint32_t height,
                           VkFormat format, VkImageUsageFlags usage);
