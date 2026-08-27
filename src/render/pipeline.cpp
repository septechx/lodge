#include "pipeline.hpp"

#include "../consts.hpp"
#include "../utils.hpp"
#include "allocator.hpp"
#include "utils.hpp"
#include "vertex.hpp"

#include <cstring>
#include <print>
#include <vulkan/vulkan_core.h>

struct ShaderModules {
  VkShaderModule vert, frag;
};

static ShaderModules loadShaders(VkDevice device) {
  ShaderModules modules;

  if (auto spir = readFileToString("build/shader.vert.spv"); spir.has_value()) {
    VkShaderModuleCreateInfo vci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spir->size(),
        .pCode = reinterpret_cast<const uint32_t *>(spir->data()),
    };
    CHECK_VK(vkCreateShaderModule(device, &vci, nullptr, &modules.vert),
             "create VS module");
  } else {
    std::println("Failed to read vertex shader");
    exit(1);
  }

  if (auto spir = readFileToString("build/shader.frag.spv"); spir.has_value()) {
    VkShaderModuleCreateInfo fci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = spir->size(),
        .pCode = reinterpret_cast<const uint32_t *>(spir->data()),
    };
    CHECK_VK(vkCreateShaderModule(device, &fci, nullptr, &modules.frag),
             "create FS module");
  } else {
    std::println("Failed to read fragment shader");
    exit(1);
  }

  return modules;
}

AllocatedBuffer createVertexBuffer(VkDevice device, VkPhysicalDevice physical,
                                   const void *data, VkDeviceSize size) {
  AllocatedBuffer buf =
      createBuffer(device, physical, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(device, buf.memory, 0, size, 0, &dst),
           "map vertex memory");
  memcpy(dst, data, static_cast<size_t>(size));
  vkUnmapMemory(device, buf.memory);

  return buf;
}

AllocatedBuffer createIndexBuffer(VkDevice device, VkPhysicalDevice physical,
                                  const void *data, VkDeviceSize size) {
  AllocatedBuffer buf =
      createBuffer(device, physical, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(device, buf.memory, 0, size, 0, &dst),
           "map index memory");
  memcpy(dst, data, static_cast<size_t>(size));
  vkUnmapMemory(device, buf.memory);

  return buf;
}

CameraUniformBuffer createCameraUniformBuffer(VkDevice device,
                                              VkPhysicalDevice physical) {
  AllocatedBuffer buf = createBuffer(device, physical, sizeof(CameraData),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(vkMapMemory(device, buf.memory, 0, sizeof(CameraData), 0, &mapped),
           "map camera uniform");
  return CameraUniformBuffer{buf.buffer, buf.memory,
                             static_cast<CameraData *>(mapped)};
}

DepthBuffer createDepthBuffer(VkDevice device, VkPhysicalDevice physical,
                              VkFormat format, uint32_t width,
                              uint32_t height) {
  AllocatedImage img = createImage(device, physical, width, height, format,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device, &vi, nullptr, &view), "create depth view");

  return DepthBuffer{img.image, img.memory, view, format};
}

SceneDescriptors createSceneDescriptors(VkDevice device,
                                        const std::vector<Texture> &textures,
                                        CameraUniformBuffer *cameras) {
  LDG_ASSERT(!textures.empty());

  VkDescriptorSetLayoutBinding bindings[2] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
  };
  VkDescriptorSetLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = bindings,
  };
  VkDescriptorSetLayout setLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &lci, nullptr, &setLayout),
           "create set layout");

  uint32_t textureCount = static_cast<uint32_t>(textures.size());
  uint32_t setCount = textureCount * MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolSize poolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = setCount},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = setCount},
  };
  VkDescriptorPoolCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = setCount,
      .poolSizeCount = 2,
      .pPoolSizes = poolSizes,
  };
  VkDescriptorPool pool;
  CHECK_VK(vkCreateDescriptorPool(device, &pci, nullptr, &pool),
           "create descriptor pool");

  std::vector<VkDescriptorSetLayout> layouts(setCount, setLayout);
  VkDescriptorSetAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = setCount,
      .pSetLayouts = layouts.data(),
  };
  std::vector<VkDescriptorSet> sets(setCount);
  CHECK_VK(vkAllocateDescriptorSets(device, &ai, sets.data()), "alloc sets");

  for (uint32_t t = 0; t < textureCount; ++t) {
    for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
      uint32_t idx = f * textureCount + t;
      VkDescriptorImageInfo imageInfo = {
          .sampler = textures[t].sampler,
          .imageView = textures[t].view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      };
      VkDescriptorBufferInfo bufferInfo = {
          .buffer = cameras[f].buffer,
          .offset = 0,
          .range = sizeof(CameraData),
      };
      VkWriteDescriptorSet writes[2] = {
          {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
           .dstSet = sets[idx],
           .dstBinding = 0,
           .dstArrayElement = 0,
           .descriptorCount = 1,
           .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
           .pImageInfo = &imageInfo},
          {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
           .dstSet = sets[idx],
           .dstBinding = 1,
           .dstArrayElement = 0,
           .descriptorCount = 1,
           .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
           .pBufferInfo = &bufferInfo},
      };
      vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
    }
  }

  return SceneDescriptors{
      .layout = setLayout,
      .pool = pool,
      .sets = std::move(sets),
      .textureCount = textureCount,
  };
}

GraphicsPipeline createPipeline(VkDevice device, VkFormat colorFormat,
                                VkFormat depthFormat, const VkExtent2D &extent,
                                VkDescriptorSetLayout setLayout) {
  ShaderModules modules = loadShaders(device);

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
  VkVertexInputAttributeDescription attrs[2] = {
      {
          .location = 0,
          .binding = 0,
          .format = VK_FORMAT_R32G32B32_SFLOAT,
          .offset = offsetof(Vertex, position),
      },
      {
          .location = 1,
          .binding = 0,
          .format = VK_FORMAT_R32G32_SFLOAT,
          .offset = offsetof(Vertex, texture),
      }};
  VkPipelineVertexInputStateCreateInfo vertexInput = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding,
      .vertexAttributeDescriptionCount = 2,
      .pVertexAttributeDescriptions = attrs,
  };

  VkPipelineInputAssemblyStateCreateInfo assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

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
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  VkPipelineDepthStencilStateCreateInfo depthStencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .minDepthBounds = 0.0f,
      .maxDepthBounds = 1.0f,
  };

  VkPipelineColorBlendAttachmentState blendAttach = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo colorBlend = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
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
      .size = sizeof(Mat4),
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
           "create graphics pipeline");

  vkDestroyShaderModule(device, modules.frag, nullptr);
  vkDestroyShaderModule(device, modules.vert, nullptr);

  return GraphicsPipeline{pipeline, layout};
}
