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

void recordFrame(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                 std::span<const RenderObject> objects,
                 const SceneDescriptors &descriptors, uint32_t frameIndex,
                 std::span<const Vec3> probes, VkImage image, VkImageView view,
                 VkImage depthImage, VkImageView depthView, SceneGrab grab,
                 VkImage grabDepth, VkImageView grabDepthView,
                 const VkExtent2D &extent, ImDrawData *drawData = nullptr);
