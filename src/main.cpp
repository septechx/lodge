#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../thirdparty/stb_image.h"

#include <cstring>

#include "consts.hpp"
#include "render/init.hpp"
#include "render/pipeline.hpp"
#include "render/render.hpp"
#include "render/texture.hpp"
#include "render/utils.hpp"
#include "render/vertex.hpp"

#define VERTEX_COUNT 6
const Vertex VERTICES[VERTEX_COUNT] = {
    {-0.5f, -0.5f, 0.0f, 0.0f}, {0.5f, -0.5f, 1.0f, 0.0f},
    {-0.5f, 0.5f, 0.0f, 1.0f},  {-0.5f, 0.5f, 0.0f, 1.0f},
    {0.5f, -0.5f, 1.0f, 0.0f},  {0.5f, 0.5f, 1.0f, 1.0f},
};

#define MAX_FRAMES_IN_FLIGHT 2

int main() {
  int init_result = glfwInit();
  if (init_result == 0) {
    std::println(stderr, "glfw init failed");
    return 1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Hello world", nullptr, nullptr);
  if (!window) {
    std::println(stderr, "glfw window failed");
    return 1;
  }

  VkInstance instance = createInstance();

  VkSurfaceKHR surface;
  CHECK_VK(glfwCreateWindowSurface(instance, window, nullptr, &surface),
           "create surface");

  Device dev = createDevice(instance, surface);

  Swapchain sc = createSwapchain(dev.device, dev.physical, surface, *window);

  DecodedImage img = decodePNG("textures/red_brick_diff_1k.png");

  StagingBuffer staging = createStaging(dev.device, dev.physical, img.texSize);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(dev.device, staging.memory, 0, img.texSize, 0, &dst),
           "map staging");
  memcpy(dst, img.pixels, static_cast<size_t>(img.texSize));
  vkUnmapMemory(dev.device, staging.memory);

  Texture tex = createTexture(dev.device, dev.physical, img.w, img.h);
  uploadPixels(dev.device, dev.queue, dev.queueFamily, tex, staging.buffer);
  vkDestroyBuffer(dev.device, staging.buffer, nullptr);
  vkFreeMemory(dev.device, staging.memory, nullptr);
  stbi_image_free(img.pixels);

  TextureDescriptor desc = createTextureDescriptor(dev.device, tex);

  VertexBuffer vb = createVertexBuffer(dev.device, dev.physical, VERTICES);

  GraphicsPipeline gp =
      createPipeline(dev.device, sc.format, sc.extent, desc.layout);

  CmdBundle cmd[MAX_FRAMES_IN_FLIGHT];
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    cmd[i] = createCmd(dev.device, dev.queueFamily);

  VkSemaphoreCreateInfo sci = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  std::vector<VkSemaphore> submitSem(sc.imageCount);
  for (uint32_t i = 0; i < sc.imageCount; i++)
    CHECK_VK(vkCreateSemaphore(dev.device, &sci, nullptr, &submitSem[i]),
             "create submit sem");

  VkSemaphore acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence frameFence[MAX_FRAMES_IN_FLIGHT];
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CHECK_VK(vkCreateSemaphore(dev.device, &sci, nullptr, &acquireSem[i]),
             "create acquire sem");
    VkFenceCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    CHECK_VK(vkCreateFence(dev.device, &fci, nullptr, &frameFence[i]),
             "create fence");
  }

  int frame = 0;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    CHECK_VK(
        vkWaitForFences(dev.device, 1, &frameFence[frame], VK_TRUE, UINT64_MAX),
        "wait fence");
    CHECK_VK(vkResetFences(dev.device, 1, &frameFence[frame]), "reset fence");

    uint32_t imageIndex = 0;
    VkResult res =
        vkAcquireNextImageKHR(dev.device, sc.swapchain, UINT64_MAX,
                              acquireSem[frame], VK_NULL_HANDLE, &imageIndex);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
      continue;
    }
    CHECK_VK(res, "acquire image");

    recordFrame(cmd[frame].cmd, gp.pipeline, gp.layout,
                static_cast<float>(glfwGetTime()), vb.buffer, desc.set,
                sc.images[imageIndex], sc.views[imageIndex], sc.extent,
                std::ranges::size(VERTICES));

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
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    } else {
      CHECK_VK(res, "present");
    }

    frame = (frame + 1) % MAX_FRAMES_IN_FLIGHT;
  }
}
