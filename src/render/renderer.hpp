#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../asset/texture/load.hpp"
#include "camera.hpp"
#include "init.hpp"
#include "light.hpp"
#include "pipeline.hpp"
#include "render.hpp"

#include <vector>

class Renderer {
public:
  Renderer(GLFWwindow &window);
  ~Renderer();
  void drawFrame();
  void onResize(uint32_t width, uint32_t height);

  Camera &getCamera() { return m_camera; }
  const Camera &getCamera() const { return m_camera; }
  void setCamera(const Camera &cam) { m_camera = cam; }

  Light &getLight() { return m_light; }
  const Light &getLight() const { return m_light; }
  void setLight(const Light &light) { m_light = light; }

  VkInstance getInstance() const { return m_instance; }
  Device &getDevice() { return m_dev; }
  const Device &getDevice() const { return m_dev; }
  Swapchain &getSwapchain() { return m_sc; }
  VkFormat getSwapchainFormat() const { return m_sc.format; }
  VkFormat getDepthFormat() const { return m_depthFormat; }

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
  std::vector<RenderObject> m_renderObjects;
  bool m_swapchainDirty = false;
  std::vector<Texture> m_textures;
  CameraUniformBuffer m_cameraUniforms[MAX_FRAMES_IN_FLIGHT];
  LightUniformBuffer m_lights[MAX_FRAMES_IN_FLIGHT];
  SceneDescriptors m_desc;
  GraphicsPipeline m_gp;
  CmdBundle m_cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> m_submitSem;
  VkSemaphore m_acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence m_frameFence[MAX_FRAMES_IN_FLIGHT];
  int m_frame = 0;
  Camera m_camera;
  Light m_light;
};
