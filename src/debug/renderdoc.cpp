#include "renderdoc.hpp"

#include <dlfcn.h>
#include <renderdoc_app.h>
#include <spdlog/spdlog.h>

namespace renderdoc {

static RENDERDOC_API_1_6_0 *api = nullptr;
static void *handle = nullptr;
static bool triedLoad = false;

bool init() {
  if (api)
    return true;
  if (triedLoad) {
    return api != nullptr;
  }
  triedLoad = true;

  handle = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
  bool ownedHandle = false;
  if (!handle) {
    handle = dlopen("librenderdoc.so", RTLD_NOW);
    ownedHandle = true;
    if (!handle) {
      spdlog::debug("RenderDoc: librenderdoc.so not found ({})", dlerror());
      return false;
    }
  }

  auto getApi =
      reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(handle, "RENDERDOC_GetAPI"));
  if (!getApi) {
    spdlog::debug("RenderDoc: RENDERDOC_GetAPI not found");
    if (ownedHandle) {
      dlclose(handle);
      handle = nullptr;
    } else {
      handle = nullptr;
    }
    return false;
  }

  int ret =
      getApi(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void **>(&api));
  if (ret != 1 || !api) {
    spdlog::warn("RenderDoc: RENDERDOC_GetAPI failed for 1.6.0");
    api = nullptr;
    if (ownedHandle && handle) {
      dlclose(handle);
      handle = nullptr;
    } else if (!ownedHandle) {
      handle = nullptr;
    }
    return false;
  }

  int major = 0, minor = 0, patch = 0;
  if (api->GetAPIVersion)
    api->GetAPIVersion(&major, &minor, &patch);
  spdlog::info("RenderDoc API loaded: {}.{}.{}", major, minor, patch);

  if (api->GetCaptureFilePathTemplate) {
    const char *tmpl = api->GetCaptureFilePathTemplate();
    if (tmpl)
      spdlog::debug("RenderDoc capture template: {}", tmpl);
  }

  return true;
}

void shutdown() { api = nullptr; }

bool isLoaded() {
  if (!api && !triedLoad)
    init();
  return api != nullptr;
}

void startCapture(VkInstance instance) {
  if (!isLoaded())
    return;
  RENDERDOC_DevicePointer dev = nullptr;
  if (instance != VK_NULL_HANDLE)
    dev = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance);
  spdlog::info("RenderDoc: StartFrameCapture (bake)");
  api->StartFrameCapture(dev, nullptr);
}

uint32_t endCapture(VkInstance instance) {
  if (!isLoaded())
    return 0;
  RENDERDOC_DevicePointer dev = nullptr;
  if (instance != VK_NULL_HANDLE)
    dev = RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance);
  uint32_t res = api->EndFrameCapture(dev, nullptr);
  spdlog::info("RenderDoc: EndFrameCapture -> {}", res);
  return res;
}

void setCaptureTitle(const char *title) {
  if (!isLoaded() || !api->SetCaptureTitle)
    return;
  api->SetCaptureTitle(title);
}

void setCaptureFilePathTemplate(const char *path) {
  if (!isLoaded() || !api->SetCaptureFilePathTemplate)
    return;
  api->SetCaptureFilePathTemplate(path);
}

ScopedCapture::ScopedCapture(VkInstance inst) : instance(inst) {
  if (isLoaded()) {
    startCapture(instance);
    active = true;
  }
}

ScopedCapture::~ScopedCapture() {
  if (active)
    endCapture(instance);
}

} // namespace renderdoc
