#include "allocator.hpp"

#include "utils.hpp"
#include <spdlog/spdlog.h>

static uint32_t findMemoryType(VkPhysicalDevice physical, uint32_t typeBits,
                               VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp;
  vkGetPhysicalDeviceMemoryProperties(physical, &mp);
  for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
    if ((typeBits & (1u << i)) &&
        (mp.memoryTypes[i].propertyFlags & props) == props)
      return i;
  spdlog::error("no suitable memory type");
  exit(1);
}

AllocatedBuffer createBuffer(VkDevice device, VkPhysicalDevice physical,
                             VkDeviceSize size, VkBufferUsageFlags usage,
                             VkMemoryPropertyFlags props) {
  VkBufferCreateInfo bci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  VkBuffer buffer;
  CHECK_VK(vkCreateBuffer(device, &bci, nullptr, &buffer), "create buffer");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, buffer, &req);
  uint32_t type = findMemoryType(physical, req.memoryTypeBits, props);
  VkMemoryAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = type,
  };
  VkDeviceMemory memory;
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory), "alloc memory");
  CHECK_VK(vkBindBufferMemory(device, buffer, memory, 0), "bind memory");

  return AllocatedBuffer{buffer, memory};
}

AllocatedImage createImage(VkDevice device, VkPhysicalDevice physical,
                           uint32_t width, uint32_t height, VkFormat format,
                           VkImageUsageFlags usage) {
  VkImageCreateInfo ici = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {width, height, 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VkImage image;
  CHECK_VK(vkCreateImage(device, &ici, nullptr, &image), "create image");

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
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory), "alloc image mem");
  CHECK_VK(vkBindImageMemory(device, image, memory, 0), "bind image mem");

  return AllocatedImage{image, memory};
}
