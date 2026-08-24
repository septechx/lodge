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
  m_instance = createInstance();

  CHECK_VK(glfwCreateWindowSurface(m_instance, &window, nullptr, &m_surface),
           "create surface");

  m_dev = createDevice(m_instance, m_surface);
  m_sc = createSwapchain(m_dev.device, m_dev.physical, m_surface, window);

  m_tex = loadTexture(m_dev, "textures/red_brick_diff_1k.png");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    m_ubos[i] = createUniformBuffer(m_dev.device, m_dev.physical);

  m_desc = createSceneDescriptors(m_dev.device, m_tex, m_ubos);

  m_vb = createVertexBuffer(m_dev.device, m_dev.physical, VERTICES,
                            sizeof(VERTICES));

  m_gp = createPipeline(m_dev.device, m_sc.format, m_sc.extent, m_desc.layout);

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

void Renderer::drawFrame() {
  const VkDevice device = m_dev.device;

  CHECK_VK(
      vkWaitForFences(device, 1, &m_frameFence[m_frame], VK_TRUE, UINT64_MAX),
      "wait fence");
  CHECK_VK(vkResetFences(device, 1, &m_frameFence[m_frame]), "reset fence");

  uint32_t imageIndex = 0;
  VkResult res =
      vkAcquireNextImageKHR(device, m_sc.swapchain, UINT64_MAX,
                            m_acquireSem[m_frame], VK_NULL_HANDLE, &imageIndex);
  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    return;
  }
  CHECK_VK(res, "acquire image");

  float aspect = static_cast<float>(m_sc.extent.width) /
                 static_cast<float>(m_sc.extent.height);
  UBO uboData = {computeViewProj(aspect)};
  memcpy(m_ubos[m_frame].mapped, &uboData, sizeof(UBO));

  recordFrame(m_cmd[m_frame].cmd, m_gp.pipeline, m_gp.layout, m_vb.buffer,
              m_desc.sets[m_frame], m_sc.images[imageIndex],
              m_sc.views[imageIndex], m_sc.extent, std::ranges::size(VERTICES));

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
  if (res != VK_ERROR_OUT_OF_DATE_KHR && res != VK_SUBOPTIMAL_KHR) {
    CHECK_VK(res, "present");
  }

  m_frame = (m_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

Renderer::~Renderer() {
  const VkDevice device = m_dev.device;
  CHECK_VK(vkDeviceWaitIdle(device), "device idle");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyCommandPool(device, m_cmd[i].pool, nullptr);
    vkDestroySemaphore(device, m_acquireSem[i], nullptr);
    vkDestroyFence(device, m_frameFence[i], nullptr);
  }
  for (VkSemaphore sem : m_submitSem)
    vkDestroySemaphore(device, sem, nullptr);

  vkDestroyDescriptorPool(device, m_desc.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, m_desc.layout, nullptr);

  vkDestroyPipeline(device, m_gp.pipeline, nullptr);
  vkDestroyPipelineLayout(device, m_gp.layout, nullptr);

  vkDestroyBuffer(device, m_vb.buffer, nullptr);
  vkFreeMemory(device, m_vb.memory, nullptr);

  vkDestroySampler(device, m_tex.sampler, nullptr);
  vkDestroyImageView(device, m_tex.view, nullptr);
  vkDestroyImage(device, m_tex.image, nullptr);
  vkFreeMemory(device, m_tex.memory, nullptr);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkUnmapMemory(device, m_ubos[i].memory);
    vkDestroyBuffer(device, m_ubos[i].buffer, nullptr);
    vkFreeMemory(device, m_ubos[i].memory, nullptr);
  }

  for (VkImageView view : m_sc.views)
    vkDestroyImageView(device, view, nullptr);
  vkDestroySwapchainKHR(device, m_sc.swapchain, nullptr);
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

  vkDestroyDevice(device, nullptr);
  destroyInstance(m_instance);
}
