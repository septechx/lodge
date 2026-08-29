#pragma once

#include <vulkan/vulkan.h>

struct Device {
  VkPhysicalDevice physical;
  VkDevice device;
  uint32_t queueFamily;
  VkQueue queue;
};

Device createDevice(VkInstance instance, VkSurfaceKHR surface);
