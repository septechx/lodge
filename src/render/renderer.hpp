#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "init.hpp"
#include "pipeline.hpp"
#include "render.hpp"
#include "texture.hpp"

struct Renderer {
  VkInstance instance;
  VkSurfaceKHR surface;
  Device dev;
  Swapchain sc;
  Texture tex;
  UniformBuffer ubos[MAX_FRAMES_IN_FLIGHT];
  SceneDescriptors desc;
  VertexBuffer vb;
  GraphicsPipeline gp;
  CmdBundle cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> submitSem;
  VkSemaphore acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence frameFence[MAX_FRAMES_IN_FLIGHT];
  int frame = 0;

  Renderer(GLFWwindow &window);
  ~Renderer();
  void drawFrame();
};
