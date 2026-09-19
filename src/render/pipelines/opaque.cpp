#include "opaque.hpp"

#include "src/render/pipelines/builder.hpp"
#include "src/render/pipelines/embedded_shaders.hpp"

GraphicsPipeline
createOpaquePipeline(VkDevice device, VkFormat colorFormat,
                     VkFormat depthFormat,
                     std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertWords = LODGE_SHADER_WORDS(k_opaqueVert),
      .fragWords = LODGE_SHADER_WORDS(k_opaqueFrag),
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
      .vertWords = LODGE_SHADER_WORDS(k_opaqueVert),
      .fragWords = LODGE_SHADER_WORDS(k_opaqueGrabFrag),
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
      .vertWords = LODGE_SHADER_WORDS(k_opaqueVert),
      .fragWords = LODGE_SHADER_WORDS(k_opaqueCompFrag),
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
