#pragma once

#include <vulkan/vulkan.h>

#define LDG_RENDERDOC_CAP(path, title, instance)                               \
  do {                                                                         \
    renderdoc::isLoaded();                                                     \
    renderdoc::setCaptureFilePathTemplate(path);                               \
    renderdoc::setCaptureTitle(title);                                         \
    renderdoc::startCapture(instance);                                         \
  } while (0)

#define LDG_RENDERDOC_ENDCAP(instance) renderdoc::endCapture(instance)

namespace renderdoc {

bool init();
void shutdown();
bool isLoaded();

void startCapture(VkInstance instance = VK_NULL_HANDLE);
uint32_t endCapture(VkInstance instance = VK_NULL_HANDLE);

struct ScopedCapture {
  VkInstance instance;
  bool active = false;
  explicit ScopedCapture(VkInstance inst = VK_NULL_HANDLE);
  ~ScopedCapture();
  ScopedCapture(const ScopedCapture &) = delete;
  ScopedCapture &operator=(const ScopedCapture &) = delete;
};

void setCaptureTitle(const char *title);

void setCaptureFilePathTemplate(const char *path);

} // namespace renderdoc
