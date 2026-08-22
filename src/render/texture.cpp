#include "../../thirdparty/stb_image.h"
#include <vulkan/vulkan_core.h>

#include <print>

#include "texture.hpp"
#include "utils.hpp"

DecodedImage decodePNG(std::filesystem::path path) {
  DecodedImage img = {0};

  img.pixels = stbi_load(path.c_str(), &img.w, &img.h, nullptr, 4);
  if (!img.pixels) {
    std::println(stderr, "stb_image failed to load {}: {}", path.string(),
                 stbi_failure_reason());
    exit(1);
  }

  std::println(stderr, "texture: {} ({}x{} RGBA)", path.string(), img.w, img.h);

  img.texSize = static_cast<VkDeviceSize>(img.w * img.h * 4);

  return img;
}

StagingBuffer createStaging(VkDevice device, VkPhysicalDevice physical,
                            VkDeviceSize size) {
  VkBufferCreateInfo bci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  VkBuffer buffer;
  CHECK_VK(vkCreateBuffer(device, &bci, nullptr, &buffer), "create staging");

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
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory), "alloc staging");

  CHECK_VK(vkBindBufferMemory(device, buffer, memory, 0), "bind staging");

  return StagingBuffer{buffer, memory};
}

Texture createTexture(VkDevice device, VkPhysicalDevice physical, int width,
                      int height) {
  VkImageCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .extent = {(uint32_t)width, (uint32_t)height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VkImage image;
  CHECK_VK(vkCreateImage(device, &ici, nullptr, &image),
           "create texture image");

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(device, image, &req);
  uint32_t type = findMemoryType(physical, req.memoryTypeBits,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VkMemoryAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = type,
  };
  VkDeviceMemory memory;
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory),
           "alloc texture mem");
  CHECK_VK(vkBindImageMemory(device, image, memory, 0), "bind texture mem");

  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device, &vi, nullptr, &view),
           "create texture view");

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

  return Texture{
      image,
      memory,
      view,
      sampler,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
  };
}

void uploadPixels(VkDevice device, VkQueue queue, uint32_t queueFamily,
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

TextureDescriptor createTextureDescriptor(VkDevice device, Texture tex) {
  VkDescriptorSetLayoutBinding binding = {
      .binding = 0,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 1,
      .pBindings = &binding,
  };
  VkDescriptorSetLayout setLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &lci, nullptr, &setLayout),
           "create set layout");

  VkDescriptorPoolSize poolSize = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = 1,
  };
  VkDescriptorPoolCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = 1,
      .poolSizeCount = 1,
      .pPoolSizes = &poolSize,
  };
  VkDescriptorPool pool;
  CHECK_VK(vkCreateDescriptorPool(device, &pci, nullptr, &pool),
           "create descriptor pool");

  VkDescriptorSetAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &setLayout,
  };
  VkDescriptorSet set;
  CHECK_VK(vkAllocateDescriptorSets(device, &ai, &set), "alloc set");

  VkDescriptorImageInfo imageInfo = {
      .sampler = tex.sampler,
      .imageView = tex.view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkWriteDescriptorSet write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set,
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &imageInfo,
  };
  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

  return TextureDescriptor{setLayout, pool, set};
}
