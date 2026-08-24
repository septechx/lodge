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

private:
  VkInstance m_instance;
  VkSurfaceKHR m_surface;
  Device m_dev;
  Swapchain m_sc;
  Texture m_tex;
  UniformBuffer m_ubos[MAX_FRAMES_IN_FLIGHT];
  SceneDescriptors m_desc;
  VertexBuffer m_vb;
  GraphicsPipeline m_gp;
  CmdBundle m_cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> m_submitSem;
  VkSemaphore m_acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence m_frameFence[MAX_FRAMES_IN_FLIGHT];
  int m_frame = 0;
};
