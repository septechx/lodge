#include "opaque.hpp"

#include "src/render/pipelines/builder.hpp"

GraphicsPipeline
createOpaquePipeline(VkDevice device, VkFormat colorFormat,
                     VkFormat depthFormat,
                     std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/opaque.vert.spv",
      .fragPath = "build/opaque.frag.spv",
      .cull = Cull::None,
      .blendEnable = false,
      .depthTest = true,
      .depthWrite = true,
      .colorAttachmentCount = 1,
      .colorFormats = {colorFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::StaticMesh,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}

GraphicsPipeline
createOpaqueGrabPipeline(VkDevice device, VkFormat colorFormat,
                         VkFormat normalFormat, VkFormat depthFormat,
                         std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/opaque.vert.spv",
      .fragPath = "build/opaque_grab.frag.spv",
      .cull = Cull::None,
      .blendEnable = false,
      .depthTest = true,
      .depthWrite = true,
      .colorAttachmentCount = 2,
      .colorFormats = {colorFormat, normalFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::StaticMesh,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}

GraphicsPipeline
createOpaqueCompPipeline(VkDevice device, VkFormat colorFormat,
                         VkFormat depthFormat,
                         std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/opaque.vert.spv",
      .fragPath = "build/opaque_comp.frag.spv",
      .cull = Cull::None,
      .blendEnable = false,
      .depthTest = true,
      .depthWrite = true,
      .colorAttachmentCount = 1,
      .colorFormats = {colorFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::StaticMesh,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}
