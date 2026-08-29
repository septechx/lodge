#include "renderer.hpp"

#include "src/asset/model/load.hpp"
#include "src/consts.hpp"
#include "src/render/init.hpp"
#include "src/render/pipelines/opaque.hpp"
#include "src/render/pipelines/transparent.hpp"
#include "src/render/render.hpp"
#include "src/render/render_object.hpp"
#include "src/render/utils.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>

#include <cstring>
#include <ranges>
#include <vector>

static VkFormat findDepthFormat(VkPhysicalDevice physical) {
  static const VkFormat candidates[] = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D16_UNORM,
  };
  auto it = std::ranges::find_if(candidates, [&](VkFormat candidate) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical, candidate, &props);
    return props.optimalTilingFeatures &
           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  });
  if (it == std::ranges::end(candidates)) {
    spdlog::error("no supported depth format found");
    exit(1);
  }
  return *it;
}

Renderer::Renderer(GLFWwindow &window) : m_window(window) {
  m_instance = createInstance();

  CHECK_VK(glfwCreateWindowSurface(m_instance, &window, nullptr, &m_surface),
           "create surface");

  m_dev = createDevice(m_instance, m_surface);
  m_sc = createSwapchain(m_dev.device, m_dev.physical, m_surface, window);

  m_depthFormat = findDepthFormat(m_dev.physical);
  m_depths.resize(m_sc.imageCount);
  for (uint32_t i = 0; i < m_sc.imageCount; ++i)
    m_depths[i] = createDepthBuffer(m_dev, m_depthFormat, m_sc.extent.width,
                                    m_sc.extent.height);

  LoadedModel loaded = loadModel(m_dev, "models/Car3.glb");
  m_renderObjects = std::move(loaded.objects);
  m_textures = std::move(loaded.textures);

  uint8_t yellow[4] = {255, 230, 64, 255};
  Texture lightTex = createTextureFromPixels(m_dev, 1, 1, yellow);
  m_textures.push_back(lightTex);
  m_light.createGizmo(m_dev, m_textures.size() - 1);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    m_cameraUniforms[i] = createCameraUniformBuffer(m_dev);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    m_lights[i] = createLightUniformBuffer(m_dev);

  m_desc = createSceneDescriptors(m_dev.device, m_textures, m_cameraUniforms,
                                  m_lights);

  m_pipelines = {
      .opaque = createOpaquePipeline(m_dev.device, m_sc.format, m_depthFormat,
                                     m_sc.extent, m_desc.layout),
      .transparent = createTransparentPipeline(
          m_dev.device, m_sc.format, m_depthFormat, m_sc.extent, m_desc.layout),
  };

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    m_cmd[i] = createCmd(m_dev.device, m_dev.queueFamily);

  VkSemaphoreCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  m_submitSem.resize(m_sc.imageCount);
  for (uint32_t i = 0; i < m_sc.imageCount; ++i)
    CHECK_VK(vkCreateSemaphore(m_dev.device, &sci, nullptr, &m_submitSem[i]),
             "create submit sem");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    CHECK_VK(vkCreateSemaphore(m_dev.device, &sci, nullptr, &m_acquireSem[i]),
             "create acquire sem");
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    CHECK_VK(vkCreateFence(m_dev.device, &fci, nullptr, &m_frameFence[i]),
             "create fence");
  }
}

void Renderer::onResize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0)
    return;
  m_swapchainDirty = true;
}

void Renderer::destroySwapchainResources() {
  const VkDevice device = m_dev.device;

  for (DepthBuffer depth : m_depths) {
    vkDestroyImageView(device, depth.view, nullptr);
    vkDestroyImage(device, depth.image, nullptr);
    vkFreeMemory(device, depth.memory, nullptr);
  }
  m_depths.clear();

  for (VkImageView view : m_sc.views)
    vkDestroyImageView(device, view, nullptr);
  vkDestroySwapchainKHR(device, m_sc.swapchain, nullptr);
}

void Renderer::recreateSwapchain() {
  const VkDevice device = m_dev.device;

  vkDeviceWaitIdle(device);

  destroySwapchainResources();

  m_sc = createSwapchain(device, m_dev.physical, m_surface, m_window);

  m_depths.resize(m_sc.imageCount);
  for (uint32_t i = 0; i < m_sc.imageCount; ++i)
    m_depths[i] = createDepthBuffer(m_dev, m_depthFormat, m_sc.extent.width,
                                    m_sc.extent.height);

  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui_ImplVulkan_SetMinImageCount(m_sc.imageCount);
  }
}

void Renderer::drawFrame() {
  const VkDevice device = m_dev.device;

  if (m_swapchainDirty) {
    recreateSwapchain();
    m_swapchainDirty = false;
    return;
  }

  if (m_sc.extent.width == 0 || m_sc.extent.height == 0)
    return; // minimized

  CHECK_VK(
      vkWaitForFences(device, 1, &m_frameFence[m_frame], VK_TRUE, UINT64_MAX),
      "wait fence");
  CHECK_VK(vkResetFences(device, 1, &m_frameFence[m_frame]), "reset fence");

  uint32_t imageIndex = 0;
  VkResult res =
      vkAcquireNextImageKHR(device, m_sc.swapchain, UINT64_MAX,
                            m_acquireSem[m_frame], VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  }
  CHECK_VK(res, "acquire image");

  float aspect = static_cast<float>(m_sc.extent.width) /
                 static_cast<float>(m_sc.extent.height);
  CameraData cameraData{m_camera.getViewProj(aspect)};
  memcpy(m_cameraUniforms[m_frame].mapped, &cameraData, sizeof(CameraData));

  LightData lightData{.lightPos = m_light.pos,
                      .lightColor = Vec3{1.0f, 1.0f, 1.0f},
                      .viewPos = m_camera.eye};
  memcpy(m_lights[m_frame].mapped, &lightData, sizeof(LightData));

  ImDrawData *drawData = nullptr;
  if (ImGui::GetCurrentContext() != nullptr) {
    drawData = ImGui::GetDrawData();
    if (drawData != nullptr &&
        (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)) {
      drawData = nullptr;
    }
  }

  std::vector<RenderObject> frameObjects = m_renderObjects;
  if (m_light.showGizmo && m_light.hasGizmo) {
    Mat4 t = Mat4::translate(m_light.pos);
    Mat4 s = Mat4::scale(
        Vec3{m_light.gizmoSize, m_light.gizmoSize, m_light.gizmoSize});
    m_light.gizmo.worldMat = t * s;
    frameObjects.push_back(m_light.gizmo);
  }

  recordFrame(m_cmd[m_frame].cmd, m_pipelines, frameObjects, m_desc,
              static_cast<uint32_t>(m_frame), m_sc.images[imageIndex],
              m_sc.views[imageIndex], m_depths[imageIndex].image,
              m_depths[imageIndex].view, m_sc.extent, drawData);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_acquireSem[m_frame],
      .pWaitDstStageMask = &waitStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &m_cmd[m_frame].cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &m_submitSem[imageIndex],
  };
  CHECK_VK(vkQueueSubmit(m_dev.queue, 1, &submit, m_frameFence[m_frame]),
           "queue submit");

  VkPresentInfoKHR present = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &m_submitSem[imageIndex],
      .swapchainCount = 1,
      .pSwapchains = &m_sc.swapchain,
      .pImageIndices = &imageIndex,
  };
  res = vkQueuePresentKHR(m_dev.queue, &present);
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  } else {
    CHECK_VK(res, "present");
  }

  m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

Renderer::~Renderer() {
  const VkDevice device = m_dev.device;
  CHECK_VK(vkDeviceWaitIdle(device), "device idle");

  m_light.destroyGizmo(m_dev);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyCommandPool(device, m_cmd[i].pool, nullptr);
    vkDestroySemaphore(device, m_acquireSem[i], nullptr);
    vkDestroyFence(device, m_frameFence[i], nullptr);
  }
  for (VkSemaphore sem : m_submitSem)
    vkDestroySemaphore(device, sem, nullptr);

  vkDestroyDescriptorPool(device, m_desc.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, m_desc.layout, nullptr);

  vkDestroyPipeline(device, m_pipelines.opaque.pipeline, nullptr);
  vkDestroyPipelineLayout(device, m_pipelines.opaque.layout, nullptr);
  vkDestroyPipeline(device, m_pipelines.transparent.pipeline, nullptr);
  vkDestroyPipelineLayout(device, m_pipelines.transparent.layout, nullptr);

  for (RenderObject obj : m_renderObjects) {
    vkDestroyBuffer(device, obj.vbuf.buffer, nullptr);
    vkFreeMemory(device, obj.vbuf.memory, nullptr);

    vkDestroyBuffer(device, obj.ibuf.buffer, nullptr);
    vkFreeMemory(device, obj.ibuf.memory, nullptr);
  }

  for (Texture tex : m_textures) {
    vkDestroySampler(device, tex.sampler, nullptr);
    vkDestroyImageView(device, tex.view, nullptr);
    vkDestroyImage(device, tex.image, nullptr);
    vkFreeMemory(device, tex.memory, nullptr);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkUnmapMemory(device, m_cameraUniforms[i].memory);
    vkDestroyBuffer(device, m_cameraUniforms[i].buffer, nullptr);
    vkFreeMemory(device, m_cameraUniforms[i].memory, nullptr);
  }

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkUnmapMemory(device, m_lights[i].memory);
    vkDestroyBuffer(device, m_lights[i].buffer, nullptr);
    vkFreeMemory(device, m_lights[i].memory, nullptr);
  }

  destroySwapchainResources();
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

  vkDestroyDevice(device, nullptr);
  destroyInstance(m_instance);
}
