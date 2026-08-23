#pragma once

#include <vulkan/vulkan.h>

#include <filesystem>

struct DecodedImage {
  int w, h;
  VkDeviceSize texSize;
  unsigned char *pixels;
};

DecodedImage decodePNG(std::filesystem::path path);

struct StagingBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
};

StagingBuffer createStaging(VkDevice device, VkPhysicalDevice physical,
                            VkDeviceSize size);

struct Texture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  uint32_t width, height;
};

Texture createTexture(VkDevice device, VkPhysicalDevice physical, int width,
                      int height);

void uploadPixels(VkDevice device, VkQueue queue, uint32_t queueFamily,
                  Texture tex, VkBuffer staging);
