#include "builder.hpp"

#include "src/render/buffers.hpp"
#include "src/render/utils.hpp"
#include "src/render/vertex.hpp"

#include <cstddef>

GraphicsPipeline
createGraphicsPipeline(VkDevice device, const PipelineDesc &desc,
                       std::span<const VkDescriptorSetLayout> setLayouts) {
  ShaderModules modules =
      loadShadersFromWords(device, desc.vertWords, desc.fragWords);

  VkPipelineShaderStageCreateInfo stages[2] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_VERTEX_BIT,
       .module = modules.vert,
       .pName = "main"},
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .module = modules.frag,
       .pName = "main"},
  };

  VkVertexInputBindingDescription binding = {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  VkVertexInputAttributeDescription attrs[3] = {
      {.location = 0,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, position)},
      {.location = 1,
       .binding = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(Vertex, texture)},
      {.location = 2,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, normal)}};
  VkPipelineVertexInputStateCreateInfo vertexInput = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };
  if (needsVertexInput(desc.vertexInput)) {
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount =
        vertexAttributeCount(desc.vertexInput);
    vertexInput.pVertexAttributeDescriptions = attrs;
  }

  VkPipelineInputAssemblyStateCreateInfo assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo raster = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = toVkCull(desc.cull),
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineDepthStencilStateCreateInfo depthStencil =
      depthStencilFor(desc.depthTest, desc.depthWrite);

  VkPipelineColorBlendAttachmentState blendAttach[2];
  for (uint32_t i = 0; i < desc.colorAttachmentCount && i < 2; ++i) {
    blendAttach[i] = blendAttachmentFor(desc.blendEnable);
  }
  VkPipelineColorBlendStateCreateInfo colorBlend = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = desc.colorAttachmentCount,
      .pAttachments = blendAttach,
  };

  VkPipelineRenderingCreateInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = desc.colorAttachmentCount,
      .pColorAttachmentFormats = desc.colorFormats.data(),
      .depthAttachmentFormat = desc.depthFormat,
  };

  VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dyn,
  };

  VkPushConstantRange pushRange = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = sizeof(PushConstants),
  };
  VkPipelineLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
      .pSetLayouts = setLayouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushRange,
  };
  VkPipelineLayout layout;
  CHECK_VK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout),
           "create pipeline layout");

  VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &rendering,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &vertexInput,
      .pInputAssemblyState = &assembly,
      .pViewportState = &viewportState,
      .pRasterizationState = &raster,
      .pMultisampleState = &multisample,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlend,
      .pDynamicState = &dynamicState,
      .layout = layout,
  };
  VkPipeline pipeline;
  CHECK_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr,
                                     &pipeline),
           "create graphics pipeline");

  vkDestroyShaderModule(device, modules.frag, nullptr);
  vkDestroyShaderModule(device, modules.vert, nullptr);

  return GraphicsPipeline{pipeline, layout};
}
