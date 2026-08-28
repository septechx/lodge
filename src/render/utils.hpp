#pragma once

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#define CHECK_VK(call, what)                                                   \
  do {                                                                         \
    VkResult __r = (call);                                                     \
    if (__r != VK_SUCCESS) {                                                   \
      spdlog::error("VK error {} at {} ({})", static_cast<int>(__r), what,     \
                    #call);                                                    \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
