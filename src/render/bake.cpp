#include "bake.hpp"

#include "src/render/buffers.hpp"
#include "src/render/cube.hpp"
#include "src/render/pipelines/pipeline.hpp"
#include "src/render/render.hpp"
#include "src/render/utils.hpp"

void recordBakeFace(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                    std::span<const RenderObject> objects,
                    const SceneDescriptors &descriptors, EnvCube env,
                    uint32_t face, VkImage faceDepthImage,
                    VkImageView faceDepthView) {

  VkCommandBufferBeginInfo begin = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  CHECK_VK(vkBeginCommandBuffer(cmd, &begin), "begin bake cmd");

  VkImageMemoryBarrier2 toAttach[2] = {
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
          .image = env.image,
          .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, face, 1},
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
          .image = faceDepthImage,
          .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
      },
  };
  VkDependencyInfo dep0 = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 2,
      .pImageMemoryBarriers = toAttach,
  };
  vkCmdPipelineBarrier2(cmd, &dep0);

  VkRenderingAttachmentInfo colorAttachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = env.faceViews[face],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue = {.color = {{0.19f, 0.19f, 0.19f, 1.0f}}},
  };
  VkRenderingAttachmentInfo depthAttachment = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = faceDepthView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .clearValue = {.depthStencil = {1.0f, 0}},
  };
  VkRenderingInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea = {{0, 0}, {CUBE_SIZE, CUBE_SIZE}},
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
  };
  vkCmdBeginRendering(cmd, &rendering);

  VkViewport viewport = {0, 0, CUBE_SIZE, CUBE_SIZE, 0, 1};
  VkRect2D scissor = {{0, 0}, {CUBE_SIZE, CUBE_SIZE}};

  vkCmdSetViewport(cmd, 0, 1, &viewport);
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelines.sky.pipeline);

  // Proc sky doesn't care about material, pick last material
  VkDescriptorSet skySet = descriptors.get(0, descriptors.textureCount - 1);
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
    VkDescriptorSet set = descriptors.get(0, texIdx);
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

  VkImageMemoryBarrier2 toRead = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
      .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
      .dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = env.image,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, (uint32_t)face, 1},
  };
  VkDependencyInfo depB = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
      .imageMemoryBarrierCount = 1,
      .pImageMemoryBarriers = &toRead,
  };
  vkCmdPipelineBarrier2(cmd, &depB);
  CHECK_VK(vkEndCommandBuffer(cmd), "end bake cmd");
}

void bakeEnvironment(Device device, GraphicsPipelines pipelines,
                     std::span<const RenderObject> objects,
                     const SceneDescriptors &descriptors, EnvCube env,
                     CameraUniformBuffer *cameras) {
  // TODO: Get the probe from the model
  const Vec3 probe = {0.0f, 1.2f, 2.06f};

  CmdBundle bake = createCmd(device.device, device.queueFamily);
  VkFenceCreateInfo fci = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  VkFence bakeFence;
  CHECK_VK(vkCreateFence(device.device, &fci, nullptr, &bakeFence),
           "bake fence");

  DepthBuffer bakeDepths[6];
  VkFormat depthFormat = findDepthFormat(device.physical);
  for (int f = 0; f < 6; f++)
    bakeDepths[f] =
        createDepthBuffer(device, depthFormat, CUBE_SIZE, CUBE_SIZE);

  Mat4 proj = Mat4::perspective(90.0f, 1.0f, 0.1f, 20.0f, false);
  for (int f = 0; f < 6; f++) {
    Mat4 view =
        Mat4::lookAt(probe, probe + CUBE_FACES[f].fwd, CUBE_FACES[f].up);
    CameraData cameraData = {.viewProj = proj * view, .viewPos = probe};
    memcpy(cameras[0].mapped, &cameraData, sizeof(CameraData));
    recordBakeFace(bake.cmd, pipelines, objects, descriptors, env, f,
                   bakeDepths[f].image, bakeDepths[f].view);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &bake.cmd,
    };
    CHECK_VK(vkQueueSubmit(device.queue, 1, &submit, bakeFence),
             "submit bake face");
    CHECK_VK(vkWaitForFences(device.device, 1, &bakeFence, VK_TRUE, UINT64_MAX),
             "wait bake face");
    CHECK_VK(vkResetFences(device.device, 1, &bakeFence), "reset bake fence");
    CHECK_VK(vkResetCommandBuffer(bake.cmd, 0), "reset bake cmd");
    spdlog::debug("bake: face {} done", CUBE_FACES[f].layer);
  }

  vkDestroyFence(device.device, bakeFence, nullptr);
  vkFreeCommandBuffers(device.device, bake.pool, 1, &bake.cmd);
  vkDestroyCommandPool(device.device, bake.pool, nullptr);
  for (int f = 0; f < 6; f++) {
    vkDestroyImageView(device.device, bakeDepths[f].view, nullptr);
    vkDestroyImage(device.device, bakeDepths[f].image, nullptr);
    vkFreeMemory(device.device, bakeDepths[f].memory, nullptr);
  }
}
