#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

VkInstance createInstance();

void destroyInstance(VkInstance instance);

struct Device {
  VkPhysicalDevice physical;
  VkDevice device;
  uint32_t queueFamily;
  VkQueue queue;
};

Device createDevice(VkInstance instance, VkSurfaceKHR surface);

struct Swapchain {
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;
  VkExtent2D extent;
  VkFormat format;
  uint32_t imageCount;
  std::vector<VkImage> images;
  std::vector<VkImageView> views;
};

Swapchain createSwapchain(VkDevice device, VkPhysicalDevice physical,
                          VkSurfaceKHR surface, GLFWwindow &window);
