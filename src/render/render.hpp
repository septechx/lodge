#pragma once

#include "imgui.h"
#include <vulkan/vulkan.h>

struct CmdBundle {
  VkCommandPool pool;
  VkCommandBuffer cmd;
};

CmdBundle createCmd(VkDevice device, uint32_t queueFamily);

void recordFrame(VkCommandBuffer cmd, VkPipeline pipeline,
                 VkPipelineLayout layout, VkBuffer vertexBuffer,
                 VkBuffer indexBuffer, uint32_t indexCount,
                 VkDescriptorSet texSet, VkImage image, VkImageView view,
                 VkImage depthImage, VkImageView depthView,
                 const VkExtent2D &extent, ImDrawData *drawData = nullptr);
