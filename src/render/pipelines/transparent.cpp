#include "transparent.hpp"

#include "src/render/pipelines/builder.hpp"
#include "src/render/pipelines/embedded_shaders.hpp"

GraphicsPipeline
createTransparentPipeline(VkDevice device, VkFormat colorFormat,
                          VkFormat depthFormat,
                          std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertWords = LODGE_SHADER_WORDS(k_transparentVert),
      .fragWords = LODGE_SHADER_WORDS(k_transparentFrag),
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
