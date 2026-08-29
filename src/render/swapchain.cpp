#include "swapchain.hpp"

#include "src/render/utils.hpp"

#include <algorithm>
#include <ranges>

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
