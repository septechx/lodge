#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <ranges>

#include "../consts.hpp"
#include "../math/Mat4.hpp"
#include "render.hpp"
#include "renderer.hpp"
#include "utils.hpp"
#include "vertex.hpp"

#define VERTEX_COUNT 6
static const Vertex VERTICES[VERTEX_COUNT] = {
    {-1.0f, 0.0f, -1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, -1.0f, 1.0f, 0.0f},
    {-1.0f, 0.0f, 1.0f, 0.0f, 1.0f},  {-1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
    {1.0f, 0.0f, -1.0f, 1.0f, 0.0f},  {1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
};

static Mat4 computeViewProj(float aspect) {
  Vec3 eye = {2, 1, 0};
  Mat4 view = Mat4::lookAt(eye, Vec3{0, 0, 0}, Vec3::UP);
  Mat4 proj = Mat4::perspective(FOV_Y_DEGREES, aspect, 0.1f, 20.0f);
  return proj * view;
}

Renderer::Renderer(GLFWwindow &window) {
  instance = createInstance();

  CHECK_VK(glfwCreateWindowSurface(instance, &window, nullptr, &surface),
           "create surface");

  dev = createDevice(instance, surface);
  sc = createSwapchain(dev.device, dev.physical, surface, window);

  tex = loadTexture(dev, "textures/red_brick_diff_1k.png");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    ubos[i] = createUniformBuffer(dev.device, dev.physical);

  desc = createSceneDescriptors(dev.device, tex, ubos);

  vb = createVertexBuffer(dev.device, dev.physical, VERTICES, sizeof(VERTICES));

  gp = createPipeline(dev.device, sc.format, sc.extent, desc.layout);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    cmd[i] = createCmd(dev.device, dev.queueFamily);

  VkSemaphoreCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  submitSem.resize(sc.imageCount);
  for (uint32_t i = 0; i < sc.imageCount; ++i)
    CHECK_VK(vkCreateSemaphore(dev.device, &sci, nullptr, &submitSem[i]),
             "create submit sem");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    CHECK_VK(vkCreateSemaphore(dev.device, &sci, nullptr, &acquireSem[i]),
             "create acquire sem");
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    CHECK_VK(vkCreateFence(dev.device, &fci, nullptr, &frameFence[i]),
             "create fence");
  }
}

void Renderer::drawFrame() {
  const VkDevice device = dev.device;

  CHECK_VK(vkWaitForFences(device, 1, &frameFence[frame], VK_TRUE, UINT64_MAX),
           "wait fence");
  CHECK_VK(vkResetFences(device, 1, &frameFence[frame]), "reset fence");

  uint32_t imageIndex = 0;
  VkResult res =
      vkAcquireNextImageKHR(device, sc.swapchain, UINT64_MAX, acquireSem[frame],
                            VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }
  CHECK_VK(res, "acquire image");

  float aspect = static_cast<float>(sc.extent.width) /
                 static_cast<float>(sc.extent.height);
  UBO uboData = {computeViewProj(aspect)};
  memcpy(ubos[frame].mapped, &uboData, sizeof(UBO));

  recordFrame(cmd[frame].cmd, gp.pipeline, gp.layout, vb.buffer,
              desc.sets[frame], sc.images[imageIndex], sc.views[imageIndex],
              sc.extent, std::ranges::size(VERTICES));

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &acquireSem[frame],
      .pWaitDstStageMask = &waitStage,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd[frame].cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &submitSem[imageIndex],
  };
  CHECK_VK(vkQueueSubmit(dev.queue, 1, &submit, frameFence[frame]),
           "queue submit");

  VkPresentInfoKHR present = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &submitSem[imageIndex],
      .swapchainCount = 1,
      .pSwapchains = &sc.swapchain,
      .pImageIndices = &imageIndex,
  };
  res = vkQueuePresentKHR(dev.queue, &present);
  if (res != VK_ERROR_OUT_OF_DATE_KHR && res != VK_SUBOPTIMAL_KHR) {
    CHECK_VK(res, "present");
  }

  frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

Renderer::~Renderer() {
  const VkDevice device = dev.device;
  CHECK_VK(vkDeviceWaitIdle(device), "device idle");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyCommandPool(device, cmd[i].pool, nullptr);
    vkDestroySemaphore(device, acquireSem[i], nullptr);
    vkDestroyFence(device, frameFence[i], nullptr);
  }
  for (VkSemaphore sem : submitSem)
    vkDestroySemaphore(device, sem, nullptr);

  vkDestroyDescriptorPool(device, desc.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, desc.layout, nullptr);

  vkDestroyPipeline(device, gp.pipeline, nullptr);
  vkDestroyPipelineLayout(device, gp.layout, nullptr);

  vkDestroyBuffer(device, vb.buffer, nullptr);
  vkFreeMemory(device, vb.memory, nullptr);

  vkDestroySampler(device, tex.sampler, nullptr);
  vkDestroyImageView(device, tex.view, nullptr);
  vkDestroyImage(device, tex.image, nullptr);
  vkFreeMemory(device, tex.memory, nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkUnmapMemory(device, ubos[i].memory);
    vkDestroyBuffer(device, ubos[i].buffer, nullptr);
    vkFreeMemory(device, ubos[i].memory, nullptr);
  }

  for (VkImageView view : sc.views)
    vkDestroyImageView(device, view, nullptr);
  vkDestroySwapchainKHR(device, sc.swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);

  vkDestroyDevice(device, nullptr);
  destroyInstance(instance);
}
