#include "init.hpp"

#include "../consts.hpp"
#include "utils.hpp"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ranges>

const char *VK_VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugMessenger;
PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugMessenger;
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
              VkDebugUtilsMessageTypeFlagsEXT type,
              const VkDebugUtilsMessengerCallbackDataEXT *data, void *user) {
  (void)type;
  (void)user;
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    spdlog::error("  [VK ERROR] {}", data->pMessage);
  else
    spdlog::debug("  [VK]       {}", data->pMessage);
  return VK_FALSE;
}

bool instanceHasLayer(const char *name) {
  uint32_t count = 0;
  CHECK_VK(vkEnumerateInstanceLayerProperties(&count, nullptr), "count layers");

  std::vector<VkLayerProperties> layers(count);
  CHECK_VK(vkEnumerateInstanceLayerProperties(&count, layers.data()),
           "list layers");

  return std::ranges::any_of(layers, [name](const auto &layer) {
    return strcmp(layer.layerName, name) == 0;
  });
}

VkInstance createInstance() {
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "Lodge",
      .applicationVersion =
          VK_MAKE_VERSION(LODGE_MAJOR, LODGE_MINOR, LODGE_PATCH),
      .pEngineName = "Lodge",
      .engineVersion = VK_MAKE_VERSION(LODGE_MAJOR, LODGE_MINOR, LODGE_PATCH),
      .apiVersion = VK_API_VERSION_1_3,
  };

  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  if (!glfwExts) {
    spdlog::error("GLFW found no Vulkan surface extensions");
    exit(1);
  }

  std::vector<const char *> exts(glfwExts, glfwExts + glfwExtCount);
  exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  const char *layers[1] = {VK_VALIDATION_LAYER};
  uint32_t layerCount = 0;
  if (instanceHasLayer(VK_VALIDATION_LAYER)) {
    layerCount = 1;
    spdlog::debug("validation layer: ON");
  } else {
    spdlog::warn("{} not installed; running without validation",
                 VK_VALIDATION_LAYER);
  }

  VkInstanceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = layerCount,
      .ppEnabledLayerNames = layers,
      .enabledExtensionCount = glfwExtCount + 1,
      .ppEnabledExtensionNames = exts.data(),
  };
  VkInstance instance;
  CHECK_VK(vkCreateInstance(&info, nullptr, &instance), "create instance");

  pfnCreateDebugMessenger =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");
  pfnDestroyDebugMessenger =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (pfnCreateDebugMessenger && layerCount) {
    VkDebugUtilsMessengerCreateInfoEXT dci = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };
    CHECK_VK(pfnCreateDebugMessenger(instance, &dci, nullptr, &debugMessenger),
             "create debug messenger");
  }

  return instance;
}

void destroyInstance(VkInstance instance) {
  if (pfnDestroyDebugMessenger && debugMessenger != VK_NULL_HANDLE)
    pfnDestroyDebugMessenger(instance, debugMessenger, nullptr);
  vkDestroyInstance(instance, nullptr);
}

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

Swapchain createSwapchain(VkDevice device, VkPhysicalDevice physical,
                          VkSurfaceKHR surface, GLFWwindow &window) {
  uint32_t fmtCount = 0;
  CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &fmtCount,
                                                nullptr),
           "count surface formats");

  std::vector<VkSurfaceFormatKHR> fmts(fmtCount);
  CHECK_VK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &fmtCount,
                                                fmts.data()),
           "list surface formats");

  auto formatIt = std::ranges::find_if(fmts, [](const auto &format) {
    return format.format == VK_FORMAT_B8G8R8A8_UNORM;
  });
  VkSurfaceFormatKHR format = formatIt != fmts.end() ? *formatIt : fmts[0];

  uint32_t pmCount = 0;
  CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface,
                                                     &pmCount, nullptr),
           "count present modes");

  std::vector<VkPresentModeKHR> modes(pmCount);
  CHECK_VK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface,
                                                     &pmCount, modes.data()),
           "list present modes");

  VkPresentModeKHR presentMode =
      std::ranges::any_of(
          modes,
          [](const auto &mode) { return mode == VK_PRESENT_MODE_MAILBOX_KHR; })
          ? VK_PRESENT_MODE_MAILBOX_KHR
          : VK_PRESENT_MODE_FIFO_KHR;

  VkSurfaceCapabilitiesKHR caps;
  CHECK_VK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps),
           "query surface caps");

  VkExtent2D extent;
  if (caps.currentExtent.width != UINT32_MAX) {
    extent = caps.currentExtent;
  } else {
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(&window, &w, &h);
    extent.width = static_cast<uint32_t>(w);
    extent.height = static_cast<uint32_t>(h);
  }

  uint32_t minCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && minCount > caps.maxImageCount)
    minCount = caps.maxImageCount;

  VkSwapchainCreateInfoKHR sc = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = minCount,
      .imageFormat = format.format,
      .imageColorSpace = format.colorSpace,
      .imageExtent = extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentMode,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };
  VkSwapchainKHR swapchain;
  CHECK_VK(vkCreateSwapchainKHR(device, &sc, nullptr, &swapchain),
           "create swapchain");

  uint32_t imageCount = 0;
  CHECK_VK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr),
           "count swapchain images");

  std::vector<VkImage> images(imageCount);
  CHECK_VK(
      vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images.data()),
      "get swapchain images");

  std::vector<VkImageView> views(images.size());
  for (auto [i, image] : std::views::enumerate(images)) {
    VkImageViewCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format.format,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
    };

    CHECK_VK(vkCreateImageView(device, &vi, nullptr, &views[i]),
             "create image view");
  }

  return Swapchain{surface,    swapchain, extent, format.format,
                   imageCount, images,    views};
}
