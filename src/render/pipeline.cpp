#include <print>

#include "../utils.hpp"
#include "pipeline.hpp"
#include "src/consts.hpp"
#include "utils.hpp"
#include "vertex.hpp"

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

UniformBuffer createUniformBuffer(VkDevice device, VkPhysicalDevice physical) {
  VkBufferCreateInfo bci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizeof(UBO),
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  VkBuffer buffer;
  CHECK_VK(vkCreateBuffer(device, &bci, nullptr, &buffer), "create ubo");

  VkMemoryRequirements req;
  vkGetBufferMemoryRequirements(device, buffer, &req);
  uint32_t type = findMemoryType(physical, req.memoryTypeBits,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VkMemoryAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = req.size,
      .memoryTypeIndex = type,
  };
  VkDeviceMemory memory;
  CHECK_VK(vkAllocateMemory(device, &ai, nullptr, &memory), "alloc ubo");
  CHECK_VK(vkBindBufferMemory(device, buffer, memory, 0), "bind ubo");

  void *mapped = nullptr;
  CHECK_VK(vkMapMemory(device, memory, 0, sizeof(UBO), 0, &mapped), "map ubo");
  return UniformBuffer{buffer, memory, static_cast<UBO *>(mapped)};
}

SceneDescriptors createSceneDescriptors(VkDevice device, Texture tex,
                                        UniformBuffer *ubos) {

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

  VkDescriptorPoolSize poolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = MAX_FRAMES_IN_FLIGHT},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = MAX_FRAMES_IN_FLIGHT},
  };
  VkDescriptorPoolCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 2,
      .pPoolSizes = poolSizes,
  };
  VkDescriptorPool pool;
  CHECK_VK(vkCreateDescriptorPool(device, &pci, nullptr, &pool),
           "create descriptor pool");

  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT] = {setLayout, setLayout};
  VkDescriptorSetAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts,
  };
  VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
  CHECK_VK(vkAllocateDescriptorSets(device, &ai, sets), "alloc sets");

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorImageInfo imageInfo = {
        .sampler = tex.sampler,
        .imageView = tex.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = ubos[i].buffer,
        .offset = 0,
        .range = sizeof(UBO),
    };
    VkWriteDescriptorSet writes[2] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = sets[i],
         .dstBinding = 0,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &imageInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = sets[i],
         .dstBinding = 1,
         .dstArrayElement = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &bufferInfo},
    };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
  }

  return SceneDescriptors{setLayout, pool, {sets[0], sets[1]}};
}

GraphicsPipeline createPipeline(VkDevice device, VkFormat format,
                                const VkExtent2D &extent,
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
      {.location = 0,
       .binding = 0,
       .format = VK_FORMAT_R32G32B32_SFLOAT,
       .offset = offsetof(Vertex, x)},
      {.location = 1,
       .binding = 0,
       .format = VK_FORMAT_R32G32_SFLOAT,
       .offset = offsetof(Vertex, u)},
  };
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
      .cullMode = VK_CULL_MODE_NONE,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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
      .pColorAttachmentFormats = &format,
  };

  VkDynamicState dyn[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dyn,
  };

  VkPipelineLayoutCreateInfo layoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = &setLayout,
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
      .pColorBlendState = &colorBlend,
      .pDynamicState = &dynamicState,
      .layout = layout,
  };
  VkPipeline pipeline;
  CHECK_VK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr,
                                     &pipeline),
           "create graphics pipeline");

  return GraphicsPipeline{pipeline, layout};
}
