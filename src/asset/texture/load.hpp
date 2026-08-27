#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>

struct Device;
struct tg3_sampler;

struct Texture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
  VkSampler sampler;
  uint32_t width, height;
};

Texture loadTexture(const Device &dev, std::filesystem::path path);

Texture createTextureFromPixels(const Device &dev, uint32_t width,
                                uint32_t height, const uint8_t *pixels);

Texture createTextureFromMemory(const Device &dev, const uint8_t *data,
                                size_t size, const tg3_sampler *sampler);

Texture createWhiteTexture(const Device &dev);

VkSampler createSamplerForGltf(VkDevice device, const tg3_sampler *gltfSampler);
