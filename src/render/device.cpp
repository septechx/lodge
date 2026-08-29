#include "device.hpp"

#include "utils.hpp"

Device createDevice(VkInstance instance, VkSurfaceKHR surface) {
  uint32_t count = 0;
  CHECK_VK(vkEnumeratePhysicalDevices(instance, &count, nullptr), "count GPUs");

  std::vector<VkPhysicalDevice> phys(count);
  CHECK_VK(vkEnumeratePhysicalDevices(instance, &count, phys.data()),
           "list GPUs");

  VkPhysicalDevice chosen = VK_NULL_HANDLE;
  uint32_t chosenFamily = 0;
  for (uint32_t i = 0; i < count && !chosen; ++i) {
    uint32_t famCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys[i], &famCount, nullptr);

    std::vector<VkQueueFamilyProperties> fams(famCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phys[i], &famCount, fams.data());

    for (uint32_t j = 0; j < famCount; ++j) {
      VkBool32 present = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(phys[i], j, surface, &present);
      if ((fams[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
        chosen = phys[j];
        chosenFamily = j;
        break;
      }
    }
  }
  if (!chosen) {
    spdlog::error("no GPU with a graphics+present queue found");
    exit(1);
  }

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(chosen, &props);
  spdlog::debug("GPU: {}", props.deviceName);

  float priority = 1.0f;
  VkDeviceQueueCreateInfo queueInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = chosenFamily,
      .queueCount = 1,
      .pQueuePriorities = &priority,
  };
  const char *deviceExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkPhysicalDeviceVulkan13Features vk13 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
      .synchronization2 = VK_TRUE,
      .dynamicRendering = VK_TRUE,
  };
  VkDeviceCreateInfo dinfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &vk13,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueInfo,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = deviceExts,
  };
  VkDevice device;
  CHECK_VK(vkCreateDevice(chosen, &dinfo, nullptr, &device), "create device");

  VkQueue queue;
  vkGetDeviceQueue(device, chosenFamily, 0, &queue);

  return Device{chosen, device, chosenFamily, queue};
}
