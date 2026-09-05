#include "sky.hpp"

#include "src/render/pipelines/builder.hpp"

GraphicsPipeline
createSkyPipeline(VkDevice device, VkFormat colorFormat, VkFormat depthFormat,
                  std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/sky.vert.spv",
      .fragPath = "build/sky.frag.spv",
      .cull = Cull::None,
      .blendEnable = false,
      .depthTest = false,
      .depthWrite = false,
      .colorAttachmentCount = 1,
      .colorFormats = {colorFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::None,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}

GraphicsPipeline
createSkyGrabPipeline(VkDevice device, VkFormat colorFormat,
                      VkFormat normalFormat, VkFormat depthFormat,
                      std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/sky.vert.spv",
      .fragPath = "build/sky.frag.spv",
      .cull = Cull::None,
      .blendEnable = false,
      .depthTest = false,
      .depthWrite = false,
      .colorAttachmentCount = 2,
      .colorFormats = {colorFormat, normalFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::None,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}
