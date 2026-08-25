#pragma once

#include "imgui.h"
#include "render_object.hpp"
#include <vulkan/vulkan.h>

#include <vector>

struct CmdBundle {
  VkCommandPool pool;
  VkCommandBuffer cmd;
};

CmdBundle createCmd(VkDevice device, uint32_t queueFamily);

void recordFrame(VkCommandBuffer cmd, VkPipeline pipeline,
                 VkPipelineLayout layout, std::vector<RenderObject> objects,
                 VkBuffer vertexBuffer, VkBuffer indexBuffer,
                 uint32_t indexCount, VkDescriptorSet texSet, VkImage image,
                 VkImageView view, VkImage depthImage, VkImageView depthView,
                 const VkExtent2D &extent, ImDrawData *drawData = nullptr);
