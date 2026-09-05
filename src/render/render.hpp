#pragma once

#include "src/render/buffers.hpp"
#include "src/render/pipelines/pipeline.hpp"
#include "src/render/render_object.hpp"

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <span>

struct CmdBundle {
  VkCommandPool pool;
  VkCommandBuffer cmd;
};

CmdBundle createCmd(VkDevice device, uint32_t queueFamily);

struct FrameContext {
  const SplitDescriptors *descriptors;
  uint32_t frameIndex;
  std::span<const Vec3> probes;
  VkImage image;
  VkImageView view;
  VkImage depthImage;
  VkImageView depthView;
  SceneGrab grab;
  SceneGrab grabNormal;
  VkImage grabDepthImage;
  VkImageView grabDepthView;
  SsrTarget ssrTarget;
  VkExtent2D extent;
};

void recordFrame(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                 ComputePipeline ssr, std::span<const RenderObject> objects,
                 const FrameContext &ctx, ImDrawData *drawData = nullptr);
