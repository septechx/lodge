#pragma once

#include "src/render/pipelines/pipeline.hpp"
#include "src/render/pipelines/selection.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <span>

enum class VertexInputKind {
  None,
  StaticMesh,
};

struct PipelineDesc {
  std::span<const uint32_t> vertWords{};
  std::span<const uint32_t> fragWords{};
  Cull cull = Cull::Back;
  bool blendEnable = false;
  bool depthTest = true;
  bool depthWrite = true;
  uint32_t colorAttachmentCount = 1;
  std::array<VkFormat, 2> colorFormats = {};
  VkFormat depthFormat = VK_FORMAT_UNDEFINED;
  VertexInputKind vertexInput = VertexInputKind::StaticMesh;
};

inline uint32_t vertexAttributeCount(VertexInputKind kind) {
  return kind == VertexInputKind::StaticMesh ? 3 : 0;
}

inline bool needsVertexInput(VertexInputKind kind) {
  return kind != VertexInputKind::None;
}

inline VkCullModeFlags toVkCull(Cull c) {
  return c == Cull::None ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
}

inline VkPipelineColorBlendAttachmentState
blendAttachmentFor(bool blendEnable) {
  VkPipelineColorBlendAttachmentState state{
      .blendEnable = blendEnable ? VK_TRUE : VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  return state;
}

inline VkPipelineDepthStencilStateCreateInfo depthStencilFor(bool depthTest,
                                                             bool depthWrite) {
  VkPipelineDepthStencilStateCreateInfo state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = depthTest ? VK_TRUE : VK_FALSE,
      .depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f,
  };
  return state;
}

GraphicsPipeline
createGraphicsPipeline(VkDevice device, const PipelineDesc &desc,
                       std::span<const VkDescriptorSetLayout> setLayouts);
