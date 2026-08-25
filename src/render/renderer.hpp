#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "init.hpp"
#include "pipeline.hpp"
#include "render.hpp"
#include "texture.hpp"

class Renderer {
public:
  Renderer(GLFWwindow &window);
  ~Renderer();
  void drawFrame();
  void onResize(uint32_t width, uint32_t height);

private:
  void recreateSwapchain();
  void destroySwapchainResources();

  GLFWwindow &m_window;
  VkInstance m_instance;
  VkSurfaceKHR m_surface;
  Device m_dev;
  Swapchain m_sc;
  VkFormat m_depthFormat;
  std::vector<DepthBuffer> m_depths;
  bool m_swapchainDirty = false;
  Texture m_tex;
  UniformBuffer m_ubos[MAX_FRAMES_IN_FLIGHT];
  SceneDescriptors m_desc;
  AllocatedBuffer m_vb;
  AllocatedBuffer m_ib;
  GraphicsPipeline m_gp;
  CmdBundle m_cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> m_submitSem;
  VkSemaphore m_acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence m_frameFence[MAX_FRAMES_IN_FLIGHT];
  int m_frame = 0;
};
