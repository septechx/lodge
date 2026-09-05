#include "renderer.hpp"

#include "src/asset/store.hpp"
#include "src/consts.hpp"
#include "src/render/bake.hpp"
#include "src/render/cube.hpp"
#include "src/render/init.hpp"
#include "src/render/pipelines/opaque.hpp"
#include "src/render/pipelines/sky.hpp"
#include "src/render/pipelines/ssr.hpp"
#include "src/render/pipelines/transparent.hpp"
#include "src/render/render.hpp"
#include "src/render/utils.hpp"
#include "src/utils.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <vector>

static Mat4 cubeInvForFallback(const Mat4 &worldMat) {
  Vec3 col0{worldMat(0, 0), worldMat(1, 0), worldMat(2, 0)};
  Vec3 col1{worldMat(0, 1), worldMat(1, 1), worldMat(2, 1)};
  Vec3 col2{worldMat(0, 2), worldMat(1, 2), worldMat(2, 2)};
  constexpr float kEps = 1e-8f;
  if (col0.length() < kEps || col1.length() < kEps || col2.length() < kEps) {
    spdlog::warn("degenerate world matrix (zero scale), using identity "
                 "cubeInv for fallback reflection");
    return Mat4::IDENTITY;
  }
  return worldMat.inverse();
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

  m_grab = createSceneGrab(m_dev, m_sc.format, m_sc.extent.width,
                           m_sc.extent.height);
  m_grabNormal = createFloatTarget(m_dev, GRAB_NORMAL_FORMAT, m_sc.extent.width,
                                   m_sc.extent.height);
  m_grabDepth = createDepthBuffer(m_dev, m_depthFormat, m_sc.extent.width,
                                  m_sc.extent.height, true);
  m_ssr = createSsrTarget(m_dev, m_sc.extent.width, m_sc.extent.height);
  VkSamplerCreateInfo gsci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  CHECK_VK(vkCreateSampler(m_dev.device, &gsci, nullptr, &m_grabSampler),
           "create grab sampler");

  VkSamplerCreateInfo ssci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  CHECK_VK(vkCreateSampler(m_dev.device, &ssci, nullptr, &m_ssrSampler),
           "create ssr sampler");

  VkSamplerCreateInfo dsci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_NEAREST,
      .minFilter = VK_FILTER_NEAREST,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  CHECK_VK(vkCreateSampler(m_dev.device, &dsci, nullptr, &m_depthSampler),
           "create depth sampler");

  VkSamplerCreateInfo esci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
  };
  CHECK_VK(vkCreateSampler(m_dev.device, &esci, nullptr, &m_envSampler),
           "create env sampler");

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
             "create frame fence");
  }
}

void Renderer::initScene(const AssetStore &assets, const FrameScene &frame) {
  LDG_ASSERT(!m_sceneInitialized);

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    m_cameraUniforms[i] = createCameraUniformBuffer(m_dev);
    m_lights[i] = createLightUniformBuffer(m_dev);
    m_materials[i] = createMaterialUniformBuffer(m_dev);
    m_ssrUbos[i] = createSsrUniformBuffer(m_dev);
  }
  m_bakeCamera = createCameraUniformBuffer(m_dev);
  m_bakeLights = createLightUniformBuffer(m_dev);
  m_bakeMaterials = createMaterialUniformBuffer(m_dev);

  m_probes.clear();
  for (const RenderObject &object : frame.objects) {
    if (object.material.kind != MaterialKind::Transparent) {
      continue;
    }
    if (m_probes.size() >= MAX_ENVS) {
      break;
    }
    m_probes.push_back({
        object.worldMat(0, 3),
        object.worldMat(1, 3),
        object.worldMat(2, 3),
    });
  }
  if (m_probes.empty()) {
    m_probes.push_back({0.0f, 1.2f, 3.2f});
  }

  m_envs.clear();
  m_envs.reserve(m_probes.size());
  for (size_t i = 0; i < m_probes.size(); ++i) {
    m_envs.push_back(createEnvCube(m_dev, m_sc.format));
  }
  std::vector<VkImageView> envViews;
  envViews.reserve(m_envs.size());
  for (EnvCube &env : m_envs) {
    envViews.push_back(env.cubeView);
  }

  m_probeBoxes.clear();
  m_probeBoxes.reserve(m_probes.size());
  uint32_t boxCount = static_cast<uint32_t>(
      std::min<size_t>(frame.objects.size(), MAX_PROBE_BOXES));
  for (size_t i = 0; i < m_probes.size(); ++i) {
    m_probeBoxes.push_back(createProbeUniformBuffer(m_dev));
    m_probeBoxes.back().mapped->probe = {m_probes[i].x, m_probes[i].y,
                                         m_probes[i].z, 0.0f};
    m_probeBoxes.back().mapped->count = boxCount;
  }

  m_desc = createSceneDescriptors(
      m_dev.device, assets.textures(), m_cameraUniforms, m_lights, m_materials,
      m_ssrUbos, m_probeBoxes, m_bakeCamera, m_bakeLights, m_bakeMaterials,
      m_grabSampler, m_grab.view, m_envSampler, envViews, m_grabSampler,
      m_grabNormal.view, m_depthSampler, m_grabDepth.view, m_ssrSampler,
      m_ssr.view);

  if (!frame.lights.empty()) {
    LightData ld{.lightPos = frame.lights.front().pos,
                 .lightColor = frame.lights.front().color};
    for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
      memcpy(m_lights[f].mapped, &ld, sizeof(LightData));
  } else {
    LightData ld{.lightPos = {4.0f, 4.0f, 4.0f},
                 .lightColor = {1.0f, 1.0f, 1.0f}};
    for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
      memcpy(m_lights[f].mapped, &ld, sizeof(LightData));
  }
  auto writeMaterials = [&](int f) {
    for (size_t i = 0; i < frame.objects.size(); ++i) {
      const Material &mat = frame.objects[i].material;
      MaterialData &dst = m_materials[f].mapped->data[i];
      if (mat.kind == MaterialKind::Transparent) {
        dst.thickness = 0.09f;
        dst.ior = 1.52f;
      } else {
        dst.thickness = 0.0f;
        dst.ior = 1.0f;
      }
      dst.baseColor = mat.baseColorFactor;
      dst.metallic = mat.metallicFactor;
      dst.roughness = mat.roughnessFactor;
      dst.cubeInv = cubeInvForFallback(frame.objects[i].worldMat);
    }
  };
  for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
    writeMaterials(f);
  memcpy(m_bakeLights.mapped, m_lights[0].mapped, sizeof(LightData));
  memcpy(m_bakeMaterials.mapped, m_materials[0].mapped, sizeof(MaterialsBlock));

  m_pipelines = {
      .opaque = createOpaquePipeline(m_dev.device, m_sc.format, m_depthFormat,
                                     m_sc.extent, m_desc.layout),
      .opaqueGrab = createOpaqueGrabPipeline(m_dev.device, m_sc.format,
                                             GRAB_NORMAL_FORMAT, m_depthFormat,
                                             m_sc.extent, m_desc.layout),
      .opaqueComp = createOpaqueCompPipeline(
          m_dev.device, m_sc.format, m_depthFormat, m_sc.extent, m_desc.layout),
      .transparent = createTransparentPipeline(
          m_dev.device, m_sc.format, m_depthFormat, m_sc.extent, m_desc.layout),
      .sky = createSkyPipeline(m_dev.device, m_sc.format, m_depthFormat,
                               m_sc.extent, m_desc.layout),
      .skyGrab =
          createSkyGrabPipeline(m_dev.device, m_sc.format, GRAB_NORMAL_FORMAT,
                                m_depthFormat, m_sc.extent, m_desc.layout),
  };
  m_ssrPipeline = createSsrPipeline(m_dev.device, m_desc.layout);

  m_bakeDepth =
      createDepthBuffer(m_dev, m_depthFormat, CUBE_SIZE, CUBE_SIZE, false);
  m_bakeCmd = createCmd(m_dev.device, m_dev.queueFamily);
  VkFenceCreateInfo bfci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  CHECK_VK(vkCreateFence(m_dev.device, &bfci, nullptr, &m_bakeFence),
           "create bake fence");

  bakeEnvironment(m_dev, m_pipelines, frame.objects, m_desc, m_envs, m_probes,
                  m_bakeCamera, m_bakeDepth, m_bakeCmd, m_bakeFence);

  m_sceneInitialized = true;
}

void Renderer::onResize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0)
    return;
  m_swapchainDirty = true;
}

void Renderer::destroySwapchainResources() {
  const VkDevice device = m_dev.device;

  vkDestroyImageView(device, m_grab.view, nullptr);
  vkDestroyImage(device, m_grab.image, nullptr);
  vkFreeMemory(device, m_grab.memory, nullptr);

  vkDestroyImageView(device, m_grabNormal.view, nullptr);
  vkDestroyImage(device, m_grabNormal.image, nullptr);
  vkFreeMemory(device, m_grabNormal.memory, nullptr);

  vkDestroyImageView(device, m_grabDepth.view, nullptr);
  vkDestroyImage(device, m_grabDepth.image, nullptr);
  vkFreeMemory(device, m_grabDepth.memory, nullptr);

  vkDestroyImageView(device, m_ssr.view, nullptr);
  vkDestroyImage(device, m_ssr.image, nullptr);
  vkFreeMemory(device, m_ssr.memory, nullptr);

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
                                    m_sc.extent.height, false);

  m_grab = createSceneGrab(m_dev, m_sc.format, m_sc.extent.width,
                           m_sc.extent.height);
  m_grabNormal = createFloatTarget(m_dev, GRAB_NORMAL_FORMAT, m_sc.extent.width,
                                   m_sc.extent.height);
  m_grabDepth = createDepthBuffer(m_dev, m_depthFormat, m_sc.extent.width,
                                  m_sc.extent.height, true);
  m_ssr = createSsrTarget(m_dev, m_sc.extent.width, m_sc.extent.height);
  updateSsrResizeDescriptors(
      m_dev.device, m_desc, m_grabSampler, m_grabNormal.view, m_depthSampler,
      m_grabDepth.view, m_ssrSampler, m_ssr.view, m_grabSampler, m_grab.view);

  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui_ImplVulkan_SetMinImageCount(m_sc.imageCount);
  }
}

void Renderer::drawFrame(const FrameScene &frame) {
  LDG_ASSERT(m_sceneInitialized);

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

  FrameCamera camera = frame.camera.value_or(FrameCamera{});
  Vec3 forward = camera.rotation.rotate(Vec3{0.0f, 0.0f, -1.0f});
  Vec3 up = camera.rotation.rotate(Vec3{0.0f, 1.0f, 0.0f});
  Mat4 view = Mat4::lookAt(camera.position, camera.position + forward, up);
  Mat4 proj = Mat4::perspective(camera.fovY, aspect, camera.nearZ, camera.farZ);
  Mat4 viewProj = proj * view;
  CameraData cameraData{.viewProj = viewProj, .viewPos = camera.position};
  memcpy(m_cameraUniforms[m_frame].mapped, &cameraData, sizeof(CameraData));

  SsrData ssrData{
      .viewProj = viewProj,
      .invViewProj = viewProj.inverse(),
      .view = view,
      .camPos = {camera.position.x, camera.position.y, camera.position.z, 0.0f},
      .march = {SSR_MAXDIST, SSR_THICKNESS, (float)SSR_STEPS,
                (float)SSR_REFINE},
  };
  memcpy(m_ssrUbos[m_frame].mapped, &ssrData, sizeof(SsrData));

  if (!frame.lights.empty()) {
    FrameLight light = frame.lights.front();
    LightData lightData{.lightPos = light.pos, .lightColor = light.color};
    memcpy(m_lights[m_frame].mapped, &lightData, sizeof(LightData));
  }

  // TODO: Dedup materials
  LDG_ASSERT(frame.objects.size() <= MAX_MATERIALS);
  for (size_t i = 0; i < frame.objects.size(); ++i) {
    const Material &mat = frame.objects[i].material;
    MaterialData &dst = m_materials[m_frame].mapped->data[i];
    if (mat.kind == MaterialKind::Transparent) {
      dst.thickness = 0.09f;
      dst.ior = 1.52f;
    } else {
      dst.thickness = 0.0f;
      dst.ior = 1.0f;
    }
    dst.baseColor = mat.baseColorFactor;
    dst.metallic = mat.metallicFactor;
    dst.roughness = mat.roughnessFactor;
    dst.cubeInv = cubeInvForFallback(frame.objects[i].worldMat);
  }

  uint32_t boxCount = static_cast<uint32_t>(
      std::min<size_t>(frame.objects.size(), MAX_PROBE_BOXES));
  for (auto &probeBox : m_probeBoxes)
    probeBox.mapped->count = boxCount;

  ImDrawData *drawData = nullptr;
  if (ImGui::GetCurrentContext() != nullptr) {
    drawData = ImGui::GetDrawData();
    if (drawData != nullptr &&
        (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)) {
      drawData = nullptr;
    }
  }

  recordFrame(m_cmd[m_frame].cmd, m_pipelines, m_ssrPipeline, frame.objects,
              m_desc, static_cast<uint32_t>(m_frame), m_probes,
              m_sc.images[imageIndex], m_sc.views[imageIndex],
              m_depths[imageIndex].image, m_depths[imageIndex].view, m_grab,
              m_grabNormal, m_grabDepth.image, m_grabDepth.view, m_ssr,
              m_sc.extent, drawData);

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

  if (!m_envs.empty() && !m_probes.empty()) {
    uint32_t facesPerEnv = 6;
    uint32_t totalFaces = facesPerEnv * (uint32_t)m_envs.size();
    uint32_t slot = m_rebakeNext % totalFaces;
    uint32_t envIdx = slot / facesPerEnv;
    uint32_t face = slot % facesPerEnv;
    if (envIdx < m_envs.size() && envIdx < m_probes.size()) {
      const LightData *freshLight =
          frame.lights.empty() ? nullptr : m_lights[m_frame].mapped;
      bool submitted = tryBakeOneFaceAsync(
          m_dev, m_pipelines, frame.objects, m_desc, m_envs[envIdx],
          m_probes[envIdx], face, m_bakeCamera, m_bakeLights, m_bakeMaterials,
          freshLight, m_materials[m_frame].mapped, m_bakeDepth, m_bakeCmd,
          m_bakeFence, m_bakePending);
      if (submitted) {
        m_rebakeNext++;
      }
    }
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

  vkDestroySampler(device, m_grabSampler, nullptr);
  vkDestroySampler(device, m_depthSampler, nullptr);
  vkDestroySampler(device, m_ssrSampler, nullptr);
  vkDestroySampler(device, m_envSampler, nullptr);

  for (EnvCube &env : m_envs) {
    destroyEnvCube(device, env);
  }
  m_envs.clear();

  if (m_sceneInitialized) {
    vkDestroyDescriptorPool(device, m_desc.pool, nullptr);
    vkDestroyDescriptorSetLayout(device, m_desc.layout, nullptr);

    vkDestroyPipeline(device, m_pipelines.opaque.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.opaque.layout, nullptr);
    vkDestroyPipeline(device, m_pipelines.opaqueGrab.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.opaqueGrab.layout, nullptr);
    vkDestroyPipeline(device, m_pipelines.opaqueComp.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.opaqueComp.layout, nullptr);
    vkDestroyPipeline(device, m_pipelines.transparent.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.transparent.layout, nullptr);
    vkDestroyPipeline(device, m_pipelines.sky.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.sky.layout, nullptr);
    vkDestroyPipeline(device, m_pipelines.skyGrab.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pipelines.skyGrab.layout, nullptr);
    vkDestroyPipeline(device, m_ssrPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, m_ssrPipeline.layout, nullptr);

    m_bakePending = false;
    vkDestroyFence(device, m_bakeFence, nullptr);
    vkFreeCommandBuffers(device, m_bakeCmd.pool, 1, &m_bakeCmd.cmd);
    vkDestroyCommandPool(device, m_bakeCmd.pool, nullptr);
    vkDestroyImageView(device, m_bakeDepth.view, nullptr);
    vkDestroyImage(device, m_bakeDepth.image, nullptr);
    vkFreeMemory(device, m_bakeDepth.memory, nullptr);

    vkUnmapMemory(device, m_bakeCamera.memory);
    vkDestroyBuffer(device, m_bakeCamera.buffer, nullptr);
    vkFreeMemory(device, m_bakeCamera.memory, nullptr);

    vkUnmapMemory(device, m_bakeLights.memory);
    vkDestroyBuffer(device, m_bakeLights.buffer, nullptr);
    vkFreeMemory(device, m_bakeLights.memory, nullptr);

    vkUnmapMemory(device, m_bakeMaterials.memory);
    vkDestroyBuffer(device, m_bakeMaterials.buffer, nullptr);
    vkFreeMemory(device, m_bakeMaterials.memory, nullptr);

    for (auto &probe : m_probeBoxes) {
      vkUnmapMemory(device, probe.memory);
      vkDestroyBuffer(device, probe.buffer, nullptr);
      vkFreeMemory(device, probe.memory, nullptr);
    }
    m_probeBoxes.clear();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vkUnmapMemory(device, m_cameraUniforms[i].memory);
      vkDestroyBuffer(device, m_cameraUniforms[i].buffer, nullptr);
      vkFreeMemory(device, m_cameraUniforms[i].memory, nullptr);

      vkUnmapMemory(device, m_lights[i].memory);
      vkDestroyBuffer(device, m_lights[i].buffer, nullptr);
      vkFreeMemory(device, m_lights[i].memory, nullptr);

      vkUnmapMemory(device, m_materials[i].memory);
      vkDestroyBuffer(device, m_materials[i].buffer, nullptr);
      vkFreeMemory(device, m_materials[i].memory, nullptr);

      vkUnmapMemory(device, m_ssrUbos[i].memory);
      vkDestroyBuffer(device, m_ssrUbos[i].buffer, nullptr);
      vkFreeMemory(device, m_ssrUbos[i].memory, nullptr);
    }
  }

  destroySwapchainResources();
  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

  vkDestroyDevice(device, nullptr);
  destroyInstance(m_instance);
}
