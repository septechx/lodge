#pragma once

#include <vulkan/vulkan.h>

struct Device;

struct Texture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  uint32_t width, height;
};

Texture loadTexture(const Device &dev, const char *path);
