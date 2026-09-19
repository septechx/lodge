#include "sky.hpp"

#include "src/render/pipelines/builder.hpp"
#include "src/render/pipelines/embedded_shaders.hpp"

GraphicsPipeline
createSkyPipeline(VkDevice device, VkFormat colorFormat, VkFormat depthFormat,
                  std::span<const VkDescriptorSetLayout> setLayouts) {
  PipelineDesc desc{
      .vertWords = LODGE_SHADER_WORDS(k_skyVert),
      .fragWords = LODGE_SHADER_WORDS(k_skyFrag),
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
      .vertWords = LODGE_SHADER_WORDS(k_skyVert),
      .fragWords = LODGE_SHADER_WORDS(k_skyFrag),
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
