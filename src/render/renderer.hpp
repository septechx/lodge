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
  SceneGrab m_grabNormal;
  DepthBuffer m_grabDepth;
  SsrTarget m_ssr;
  VkSampler m_grabSampler;
  VkSampler m_depthSampler;
  VkSampler m_ssrSampler;
  std::vector<EnvCube> m_envs;
  std::vector<Vec3> m_probes;
  VkSampler m_envSampler;
  bool m_swapchainDirty = false;
  bool m_sceneInitialized = false;
  CameraUniformBuffer m_cameraUniforms[MAX_FRAMES_IN_FLIGHT];
  LightUniformBuffer m_lights[MAX_FRAMES_IN_FLIGHT];
  MaterialUniformBuffer m_materials[MAX_FRAMES_IN_FLIGHT];
  ObjectUniformBuffer m_objects[MAX_FRAMES_IN_FLIGHT];
  SsrUniformBuffer m_ssrUbos[MAX_FRAMES_IN_FLIGHT];
  std::vector<ProbeUniformBuffer> m_probeBoxes;
  CameraUniformBuffer m_bakeCamera;
  LightUniformBuffer m_bakeLights;
  MaterialUniformBuffer m_bakeMaterials;
  ObjectUniformBuffer m_bakeObjects;
  SplitDescriptors m_sets;
  const AssetStore *m_assets = nullptr;
  GraphicsPipelines m_pipelines;
  ComputePipeline m_ssrPipeline;
  DepthBuffer m_bakeDepth;
  CmdBundle m_bakeCmd;
  VkFence m_bakeFence = VK_NULL_HANDLE;
  bool m_bakePending = false;
  uint32_t m_rebakeNext = 0;
  CmdBundle m_cmd[MAX_FRAMES_IN_FLIGHT];
  std::vector<VkSemaphore> m_submitSem;
  VkSemaphore m_acquireSem[MAX_FRAMES_IN_FLIGHT];
  VkFence m_frameFence[MAX_FRAMES_IN_FLIGHT];
  int m_frame = 0;
};
