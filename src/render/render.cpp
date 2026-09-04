#include "render.hpp"

#include "src/render/pipelines/pipeline.hpp"
#include "src/render/utils.hpp"

#include <imgui_impl_vulkan.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>

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

static Vec3 objectCenter(const RenderObject &object) {
  return {
      object.worldMat(0, 3),
      object.worldMat(1, 3),
      object.worldMat(2, 3),
  };
}

static uint32_t nearestProbe(Vec3 pos, std::span<const Vec3> probes,
                             uint32_t envCount) {
  if (probes.empty() || envCount == 0) {
    return 0;
  }
  uint32_t best = 0;
  float bestDist2 = -1.0f;
  uint32_t count = std::min<uint32_t>(probes.size(), envCount);
  for (uint32_t i = 0; i < count; ++i) {
    Vec3 d = pos - probes[i];
    float dist2 = d.length();
    if (bestDist2 < 0.0f || dist2 < bestDist2) {
      bestDist2 = dist2;
      best = i;
    }
  }
  return best;
}

void recordFrame(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                 std::span<const RenderObject> objects,
                 const SceneDescriptors &descriptors, uint32_t frameIndex,
                 std::span<const Vec3> probes, VkImage image, VkImageView view,
                 VkImage depthImage, VkImageView depthView, SceneGrab grab,
                 const VkExtent2D &extent, ImDrawData *drawData) {

  VkCommandBufferBeginInfo begin = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  CHECK_VK(vkBeginCommandBuffer(cmd, &begin), "begin cmd buffer");

  VkImageMemoryBarrier2 toGrab[2] = {
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
       .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
       .srcAccessMask = 0,
       .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
       .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
       .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .image = grab.image,
       .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}},
      {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
       .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
       .srcAccessMask = 0,
       .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
       .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
       .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
       .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
       .image = depthImage,
       .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1}}};
  VkDependencyInfo depGrab = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                              .imageMemoryBarrierCount = 2,
                              .pImageMemoryBarriers = toGrab};
  vkCmdPipelineBarrier2(cmd, &depGrab);

  VkRenderingAttachmentInfo grabColor = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = grab.view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {.color = {{0.19f, 0.19f, 0.19f, 1.0f}}}};
  VkRenderingAttachmentInfo grabDepthAtt = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = depthView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}}};
  VkRenderingInfo grabRendering = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                                   .renderArea = {{0, 0}, extent},
                                   .layerCount = 1,
                                   .colorAttachmentCount = 1,
                                   .pColorAttachments = &grabColor,
                                   .pDepthAttachment = &grabDepthAtt};
  vkCmdBeginRendering(cmd, &grabRendering);

  VkViewport viewport = {
      0.0f,
      0.0f,
      static_cast<float>(extent.width),
      static_cast<float>(extent.height),
      0.0f,
      1.0f,
  };
  VkRect2D scissor = {{0, 0}, extent};

  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.sky.pipeline);

  // Proc sky doesn't care about material, pick last material
  VkDescriptorSet skySet =
      descriptors.get(frameIndex, descriptors.textureCount - 1);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.sky.layout, 0, 1, &skySet, 0, nullptr);

  // No vertex buffer for sky, just get the rasterizer to do something
  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.opaque.pipeline);

  for (size_t i = 0; i < objects.size(); ++i) {
    const RenderObject &object = objects[i];
    if (object.material.kind != MaterialKind::Opaque) {
      continue;
    }

    uint32_t texIdx = object.material.texture.index;
    if (texIdx >= descriptors.textureCount)
      texIdx = 0;
    VkDescriptorSet set = descriptors.get(frameIndex, texIdx);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelines.opaque.layout, 0, 1, &set, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &object.vbuf.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, object.ibuf.buffer, 0, object.indexType);

    PushConstants pc{.model = object.worldMat,
                     .materialIdx = static_cast<uint32_t>(i)};
    vkCmdPushConstants(cmd, pipelines.opaque.layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(PushConstants), &pc);

    vkCmdDrawIndexed(cmd, object.indexCount, 1, 0, 0, 0);
  }

  vkCmdEndRendering(cmd);

  VkImageMemoryBarrier2 grabToRead = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = grab.image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};
  VkDependencyInfo depGrabRead = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                  .imageMemoryBarrierCount = 1,
                                  .pImageMemoryBarriers = &grabToRead};
  vkCmdPipelineBarrier2(cmd, &depGrabRead);

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
      .clearValue = {.color = {{0.19f, 0.19f, 0.19f, 1.0f}}},
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

  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  // START =  TODO: Re-use grab
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.sky.pipeline);

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelines.sky.layout, 0, 1, &skySet, 0, nullptr);

  // No vertex buffer for sky, just get the rasterizer to do something
  vkCmdDraw(cmd, 3, 1, 0, 0);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.opaque.pipeline);

  for (size_t i = 0; i < objects.size(); ++i) {
    const RenderObject &object = objects[i];
    if (object.material.kind != MaterialKind::Opaque) {
      continue;
    }

    uint32_t texIdx = object.material.texture.index;
    if (texIdx >= descriptors.textureCount)
      texIdx = 0;
    VkDescriptorSet set = descriptors.get(frameIndex, texIdx);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelines.opaque.layout, 0, 1, &set, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &object.vbuf.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, object.ibuf.buffer, 0, object.indexType);

    PushConstants pc{.model = object.worldMat,
                     .materialIdx = static_cast<uint32_t>(i)};
    vkCmdPushConstants(cmd, pipelines.opaque.layout, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(PushConstants), &pc);

    vkCmdDrawIndexed(cmd, object.indexCount, 1, 0, 0, 0);
  }
  // END

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.transparent.pipeline);

  for (size_t i = 0; i < objects.size(); ++i) {
    const RenderObject &object = objects[i];
    if (object.material.kind != MaterialKind::Transparent) {
      continue;
    }

    uint32_t texIdx = object.material.texture.index;
    if (texIdx >= descriptors.textureCount)
      texIdx = 0;
    uint32_t envIdx =
        nearestProbe(objectCenter(object), probes, descriptors.envCount);
    VkDescriptorSet set = descriptors.get(frameIndex, texIdx, envIdx);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelines.transparent.layout, 0, 1, &set, 0,
                            nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &object.vbuf.buffer, &offset);
    vkCmdBindIndexBuffer(cmd, object.ibuf.buffer, 0, object.indexType);

    PushConstants pc{.model = object.worldMat,
                     .materialIdx = static_cast<uint32_t>(i)};
    vkCmdPushConstants(cmd, pipelines.transparent.layout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants),
                       &pc);

    vkCmdDrawIndexed(cmd, object.indexCount, 1, 0, 0, 0);
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
