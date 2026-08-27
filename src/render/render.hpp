#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "imgui.h"
#include "pipeline.hpp"
#include "render_object.hpp"

struct CmdBundle {
  VkCommandPool pool;
  VkCommandBuffer cmd;
};

CmdBundle createCmd(VkDevice device, uint32_t queueFamily);

void recordFrame(VkCommandBuffer cmd, VkPipeline pipeline,
                 VkPipelineLayout layout,
                 const std::vector<RenderObject> &objects,
                 const SceneDescriptors &descriptors, uint32_t frameIndex,
                 VkImage image, VkImageView view, VkImage depthImage,
                 VkImageView depthView, const VkExtent2D &extent,
                 ImDrawData *drawData = nullptr);
