#pragma once

#include <vulkan/vulkan.h>

#include <print>

#define CHECK_VK(call, what)                                                   \
  do {                                                                         \
    VkResult __r = (call);                                                     \
    if (__r != VK_SUCCESS) {                                                   \
      std::println(stderr, "VK error {} at {} ({})", static_cast<int>(__r),    \
                   what, #call);                                               \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

uint32_t findMemoryType(VkPhysicalDevice physical, uint32_t typeBits,
                        VkMemoryPropertyFlags props);
