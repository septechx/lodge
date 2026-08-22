#pragma once

#include <vulkan/vulkan.h>

struct CmdBundle {
  VkCommandPool pool;
  VkCommandBuffer cmd;
};

CmdBundle createCmd(VkDevice device, uint32_t queueFamily);

void recordFrame(VkCommandBuffer cmd, VkPipeline pipeline,
                 VkPipelineLayout layout, float time, VkBuffer vertexBuffer,
                 VkDescriptorSet texSet, VkImage image, VkImageView view,
                 const VkExtent2D &extent, uint32_t vertexCount);
