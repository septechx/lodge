#include "transparent.hpp"

#include "src/render/pipelines/builder.hpp"

GraphicsPipeline
createTransparentPipeline(VkDevice device, VkFormat colorFormat,
                          VkFormat depthFormat,
                          std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertPath = "build/transparent.vert.spv",
      .fragPath = "build/transparent.frag.spv",
      .cull = Cull::Back,
      .blendEnable = true,
      .depthTest = true,
      .depthWrite = false,
      .colorAttachmentCount = 1,
      .colorFormats = {colorFormat},
      .depthFormat = depthFormat,
      .vertexInput = VertexInputKind::StaticMesh,
  };
  return createGraphicsPipeline(device, desc, setLayouts);
}
