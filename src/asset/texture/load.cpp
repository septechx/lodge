#include "load.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../../render/allocator.hpp"
#include "../../render/init.hpp"
#include "../../render/utils.hpp"
#include "tiny_gltf_v3.h"
#include <spdlog/spdlog.h>

#include <cstring>

static Texture createTexture(VkDevice device, VkPhysicalDevice physical,
                             int width, int height, VkSampler sampler) {
  AllocatedImage img =
      createImage(device, physical, static_cast<uint32_t>(width),
                  static_cast<uint32_t>(height), VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device, &vi, nullptr, &view),
           "create texture view");

  return Texture{
      img.image,
      img.memory,
      view,
      sampler,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
  };
}

static VkSampler createDefaultSampler(VkDevice device) {
  VkSamplerCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };
  VkSampler sampler;
  CHECK_VK(vkCreateSampler(device, &sci, nullptr, &sampler), "create sampler");
  return sampler;
}

VkSampler createSamplerForGltf(VkDevice device,
                               const tg3_sampler *gltfSampler) {
  VkSamplerCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };

  if (gltfSampler != nullptr) {
    if (gltfSampler->mag_filter == TG3_TEXTURE_FILTER_NEAREST)
      sci.magFilter = VK_FILTER_NEAREST;
    else if (gltfSampler->mag_filter == TG3_TEXTURE_FILTER_LINEAR)
      sci.magFilter = VK_FILTER_LINEAR;

    switch (gltfSampler->min_filter) {
    case TG3_TEXTURE_FILTER_NEAREST:
    case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TG3_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
      sci.minFilter = VK_FILTER_NEAREST;
      sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
      break;
    case TG3_TEXTURE_FILTER_LINEAR:
    case TG3_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    case TG3_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
      sci.minFilter = VK_FILTER_LINEAR;
      sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
      break;
    default:
      break;
    }

    auto wrapToVk = [](int32_t wrap) {
      switch (wrap) {
      case TG3_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
      case TG3_TEXTURE_WRAP_MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
      case TG3_TEXTURE_WRAP_REPEAT:
      default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
      }
    };
    sci.addressModeU = wrapToVk(gltfSampler->wrap_s);
    sci.addressModeV = wrapToVk(gltfSampler->wrap_t);
  }

  VkSampler sampler;
  CHECK_VK(vkCreateSampler(device, &sci, nullptr, &sampler),
           "create glTF sampler");
  return sampler;
}

static void uploadPixels(VkDevice device, VkQueue queue, uint32_t queueFamily,
                         Texture tex, VkBuffer staging) {
  VkCommandPoolCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamily,
  };
  VkCommandPool pool;
  CHECK_VK(vkCreateCommandPool(device, &pci, nullptr, &pool), "upload pool");
  VkCommandBufferAllocateInfo cai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VkCommandBuffer cmd;
  CHECK_VK(vkAllocateCommandBuffers(device, &cai, &cmd), "upload cmd");

  VkCommandBufferBeginInfo begin = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  CHECK_VK(vkBeginCommandBuffer(cmd, &begin), "begin upload cmd");

  VkImageMemoryBarrier2 toTransfer = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = tex.image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkDependencyInfo dep0 = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &toTransfer,
  };
  vkCmdPipelineBarrier2(cmd, &dep0);

  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
      .imageOffset = {0, 0, 0},
      .imageExtent = {tex.width, tex.height, 1},
  };
  vkCmdCopyBufferToImage(cmd, staging, tex.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  VkImageMemoryBarrier2 toShaderRead = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
      .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = tex.image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkDependencyInfo dep1 = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &toShaderRead,
  };
  vkCmdPipelineBarrier2(cmd, &dep1);

  CHECK_VK(vkEndCommandBuffer(cmd), "end upload cmd");
  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
  };
  CHECK_VK(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), "submit upload");
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, pool, 1, &cmd);
  vkDestroyCommandPool(device, pool, nullptr);
}

Texture loadTexture(const Device &dev, std::filesystem::path path) {
  int w, h;
  unsigned char *pixels = stbi_load(path.c_str(), &w, &h, nullptr, 4);
  if (!pixels) {
    spdlog::error("stb_image failed to load {}: {}", path.string(),
                  stbi_failure_reason());
    exit(1);
  }
  spdlog::debug("texture: {} ({}x{} RGBA)", path.string(), w, h);
  VkDeviceSize texSize = static_cast<VkDeviceSize>(w * h * 4);

  AllocatedBuffer staging = createBuffer(
      dev.device, dev.physical, texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(dev.device, staging.memory, 0, texSize, 0, &dst),
           "map staging");
  memcpy(dst, pixels, static_cast<size_t>(texSize));
  vkUnmapMemory(dev.device, staging.memory);

  VkSampler sampler = createDefaultSampler(dev.device);
  Texture tex = createTexture(dev.device, dev.physical, w, h, sampler);
  uploadPixels(dev.device, dev.queue, dev.queueFamily, tex, staging.buffer);

  vkDestroyBuffer(dev.device, staging.buffer, nullptr);
  vkFreeMemory(dev.device, staging.memory, nullptr);
  stbi_image_free(pixels);
  return tex;
}

Texture createTextureFromPixels(const Device &dev, uint32_t width,
                                uint32_t height, const uint8_t *pixels) {
  VkDeviceSize texSize = static_cast<VkDeviceSize>(width * height * 4);
  AllocatedBuffer staging = createBuffer(
      dev.device, dev.physical, texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(dev.device, staging.memory, 0, texSize, 0, &dst),
           "map staging pixels");
  memcpy(dst, pixels, static_cast<size_t>(texSize));
  vkUnmapMemory(dev.device, staging.memory);

  VkSampler sampler = createDefaultSampler(dev.device);
  Texture tex = createTexture(dev.device, dev.physical, static_cast<int>(width),
                              static_cast<int>(height), sampler);
  uploadPixels(dev.device, dev.queue, dev.queueFamily, tex, staging.buffer);

  vkDestroyBuffer(dev.device, staging.buffer, nullptr);
  vkFreeMemory(dev.device, staging.memory, nullptr);
  return tex;
}

Texture createTextureFromMemory(const Device &dev, const uint8_t *data,
                                size_t size, const tg3_sampler *sampler) {
  int w, h;
  unsigned char *pixels =
      stbi_load_from_memory(data, static_cast<int>(size), &w, &h, nullptr, 4);
  if (!pixels) {
    spdlog::error("stb_image failed from memory: {}", stbi_failure_reason());
    exit(1);
  }
  spdlog::debug("texture from memory ({}x{} RGBA, {} bytes)", w, h, size);
  VkDeviceSize texSize = static_cast<VkDeviceSize>(w * h * 4);

  AllocatedBuffer staging = createBuffer(
      dev.device, dev.physical, texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(dev.device, staging.memory, 0, texSize, 0, &dst),
           "map staging memory tex");
  memcpy(dst, pixels, static_cast<size_t>(texSize));
  vkUnmapMemory(dev.device, staging.memory);

  VkSampler vkSampler = createSamplerForGltf(dev.device, sampler);
  Texture tex = createTexture(dev.device, dev.physical, w, h, vkSampler);
  uploadPixels(dev.device, dev.queue, dev.queueFamily, tex, staging.buffer);

  vkDestroyBuffer(dev.device, staging.buffer, nullptr);
  vkFreeMemory(dev.device, staging.memory, nullptr);
  stbi_image_free(pixels);
  return tex;
}

Texture createWhiteTexture(const Device &dev) {
  uint8_t white[4] = {255, 255, 255, 255};
  VkDeviceSize texSize = 4;
  AllocatedBuffer staging = createBuffer(
      dev.device, dev.physical, texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(dev.device, staging.memory, 0, texSize, 0, &dst),
           "map white staging");
  memcpy(dst, white, 4);
  vkUnmapMemory(dev.device, staging.memory);

  VkSampler sampler = createDefaultSampler(dev.device);
  Texture tex = createTexture(dev.device, dev.physical, 1, 1, sampler);
  uploadPixels(dev.device, dev.queue, dev.queueFamily, tex, staging.buffer);

  vkDestroyBuffer(dev.device, staging.buffer, nullptr);
  vkFreeMemory(dev.device, staging.memory, nullptr);
  return tex;
}
