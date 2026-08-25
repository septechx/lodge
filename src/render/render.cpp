#include "render.hpp"

#include "../math/Mat4.hpp"
#include "imgui_impl_vulkan.h"
#include "utils.hpp"
#include <vulkan/vulkan_core.h>

CmdBundle createCmd(VkDevice device, uint32_t queueFamily) {
  VkCommandPoolCreateInfo cpi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamily,
  };
  VkCommandPool pool;
  CHECK_VK(vkCreateCommandPool(device, &cpi, nullptr, &pool),
           "create command pool");

  VkCommandBufferAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  VkCommandBuffer cmd;
  CHECK_VK(vkAllocateCommandBuffers(device, &ai, &cmd), "alloc command buffer");

  return CmdBundle{pool, cmd};
}

static constexpr Mat4 objectModelMatrix(RenderObject object) {
  return Mat4::translate(object.pos) * Mat4::rotEuler(object.euler) *
         Mat4::scale({object.scale, object.scale, object.scale});
}

void recordFrame(VkCommandBuffer cmd, VkPipeline pipeline,
                 VkPipelineLayout layout, std::vector<RenderObject> objects,
                 VkBuffer vertexBuffer, VkBuffer indexBuffer,
                 uint32_t indexCount, VkDescriptorSet texSet, VkImage image,
                 VkImageView view, VkImage depthImage, VkImageView depthView,
                 const VkExtent2D &extent, ImDrawData *drawData) {

  VkCommandBufferBeginInfo begin = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  CHECK_VK(vkBeginCommandBuffer(cmd, &begin), "begin cmd buffer");

  VkImageMemoryBarrier2 toRenderable[2] = {
      {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
          .srcAccessMask = 0,
          .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = image,
          .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
      },
      {
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
          .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
          .srcAccessMask = 0,
          .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
          .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = depthImage,
          .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
      }};
  VkDependencyInfo dep0 = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers = toRenderable,
  };
  vkCmdPipelineBarrier2(cmd, &dep0);

  VkRenderingAttachmentInfo colorAttachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
  };
  VkRenderingAttachmentInfo depthAttachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depthView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}},
  };
  VkRenderingInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{0, 0}, extent},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(cmd, &rendering);

  VkViewport viewport = {
      0.0f,
      0.0f,
      static_cast<float>(extent.width),
      static_cast<float>(extent.height),
      0.0f,
      1.0f,
  };
  vkCmdSetViewport(cmd, 0, 1, &viewport);
  VkRect2D scissor = {{0, 0}, extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                          &texSet, 0, nullptr);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
  vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

  for (RenderObject object : objects) {
    Mat4 modelMatrix = objectModelMatrix(object);
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4),
                       &modelMatrix);
    vkCmdDrawIndexed(cmd, indexCount, 1, 0, 0, 0);
  }

  vkCmdEndRendering(cmd);

  if (drawData != nullptr && drawData->TotalVtxCount > 0) {
    VkRenderingAttachmentInfo colorAttachmentImGui = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo renderingImGui = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentImGui,
    };
    vkCmdBeginRendering(cmd, &renderingImGui);
    ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
    vkCmdEndRendering(cmd);
  }

  VkImageMemoryBarrier2 toPresent = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
      .dstAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkDependencyInfo dep1 = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &toPresent,
  };
  vkCmdPipelineBarrier2(cmd, &dep1);

  CHECK_VK(vkEndCommandBuffer(cmd), "end cmd buffer");
}
