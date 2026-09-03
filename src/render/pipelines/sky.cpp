#include "sky.hpp"

#include "src/render/buffers.hpp"
#include "src/render/utils.hpp"

GraphicsPipeline createSkyPipeline(VkDevice device, VkFormat colorFormat,
                                   VkFormat depthFormat,
                                   const VkExtent2D &extent,
                                   VkDescriptorSetLayout setLayout) {
  ShaderModules modules =
      loadShaders(device, "build/sky.vert.spv", "build/sky.frag.spv");

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

  VkPipelineVertexInputStateCreateInfo vertexInput = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};

  VkPipelineInputAssemblyStateCreateInfo assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

  VkViewport viewport = {
      0.0f,
      0.0f,
      static_cast<float>(extent.width),
      static_cast<float>(extent.height),
      0.0f,
      1.0f,
  };
  VkRect2D scissor = {{0, 0}, extent};
  VkPipelineViewportStateCreateInfo viewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo raster = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineDepthStencilStateCreateInfo depthStencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
  };

  VkPipelineColorBlendAttachmentState blendAttach = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo colorBlend = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &blendAttach,
  };

  VkPipelineRenderingCreateInfo rendering = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorFormat,
      .depthAttachmentFormat = depthFormat,
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
      .setLayoutCount = 1,
      .pSetLayouts = &setLayout,
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
           "create sky pipeline");

  vkDestroyShaderModule(device, modules.frag, nullptr);
  vkDestroyShaderModule(device, modules.vert, nullptr);

  return GraphicsPipeline{pipeline, layout};
}
