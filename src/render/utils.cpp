#include "utils.hpp"

uint32_t findMemoryType(VkPhysicalDevice physical, uint32_t typeBits,
                        VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp;
  vkGetPhysicalDeviceMemoryProperties(physical, &mp);
  for (uint32_t i = 0; i < mp.memoryTypeCount; ++i)
    if ((typeBits & (1u << i)) &&
        (mp.memoryTypes[i].propertyFlags & props) == props)
      return i;
  std::println(stderr, "no suitable memory type");
  exit(1);
}
