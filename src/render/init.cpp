#include "init.hpp"

#include "src/consts.hpp"
#include "src/render/utils.hpp"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

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
    spdlog::error("[VK ERROR] {}", data->pMessage);
  else
    spdlog::debug("[VK] {}", data->pMessage);
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
