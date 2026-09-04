#pragma once

#include "src/asset/store.hpp"
#include "src/render/buffers.hpp"
#include "src/render/pipelines/pipeline.hpp"
#include "src/render/render.hpp"
#include "src/render/swapchain.hpp"
#include "src/scene/extract.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

class Renderer {
public:
  Renderer(GLFWwindow &window);
  ~Renderer();

  void initScene(const AssetStore &assets, const FrameScene &frame);

  void drawFrame(const FrameScene &frame);
  void onResize(uint32_t width, uint32_t height);

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
  SceneGrab m_grab;
  VkSampler m_grabSampler;
  std::vector<EnvCube> m_envs;
  std::vector<Vec3> m_probes;
  VkSampler m_envSampler;
  bool m_swapchainDirty = false;
  bool m_sceneInitialized = false;
  CameraUniformBuffer m_cameraUniforms[MAX_FRAMES_IN_FLIGHT];
  LightUniformBuffer m_lights[MAX_FRAMES_IN_FLIGHT];
  MaterialUniformBuffer m_materials[MAX_FRAMES_IN_FLIGHT];
  SceneDescriptors m_desc;
  GraphicsPipelines m_pipelines;
  CmdBundle m_cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> m_submitSem;
  VkSemaphore m_acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence m_frameFence[MAX_FRAMES_IN_FLIGHT];
  int m_frame = 0;
};
