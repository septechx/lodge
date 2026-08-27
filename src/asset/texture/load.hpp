#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>

struct Device;

struct Texture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  uint32_t width, height;
};

Texture loadTexture(const Device &dev, std::filesystem::path path);
