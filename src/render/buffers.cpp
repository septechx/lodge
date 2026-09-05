#include "buffers.hpp"

#include "src/consts.hpp"
#include "src/render/allocator.hpp"
#include "src/render/cube.hpp"
#include "src/render/utils.hpp"
#include "src/utils.hpp"

AllocatedBuffer createVertexBuffer(Device device, const void *data,
                                   VkDeviceSize size) {
  AllocatedBuffer buf =
      createBuffer(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(device.device, buf.memory, 0, size, 0, &dst),
           "map vertex memory");
  memcpy(dst, data, static_cast<size_t>(size));
  vkUnmapMemory(device.device, buf.memory);

  return buf;
}

AllocatedBuffer createIndexBuffer(Device device, const void *data,
                                  VkDeviceSize size) {
  AllocatedBuffer buf =
      createBuffer(device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *dst = nullptr;
  CHECK_VK(vkMapMemory(device.device, buf.memory, 0, size, 0, &dst),
           "map index memory");
  memcpy(dst, data, static_cast<size_t>(size));
  vkUnmapMemory(device.device, buf.memory);

  return buf;
}

CameraUniformBuffer createCameraUniformBuffer(Device device) {
  AllocatedBuffer buf = createBuffer(device, sizeof(CameraData),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(
      vkMapMemory(device.device, buf.memory, 0, sizeof(CameraData), 0, &mapped),
      "map camera uniform");
  return CameraUniformBuffer{buf.buffer, buf.memory,
                             static_cast<CameraData *>(mapped)};
}

LightUniformBuffer createLightUniformBuffer(Device device) {
  AllocatedBuffer buf = createBuffer(device, sizeof(LightData),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(
      vkMapMemory(device.device, buf.memory, 0, sizeof(LightData), 0, &mapped),
      "map light uniform");
  return LightUniformBuffer{buf.buffer, buf.memory,
                            static_cast<LightData *>(mapped)};
}

MaterialUniformBuffer createMaterialUniformBuffer(Device device) {
  AllocatedBuffer buf = createBuffer(device, sizeof(MaterialsBlock),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(vkMapMemory(device.device, buf.memory, 0, sizeof(MaterialsBlock), 0,
                       &mapped),
           "map material uniform");
  return MaterialUniformBuffer{buf.buffer, buf.memory,
                               static_cast<MaterialsBlock *>(mapped)};
}

ObjectUniformBuffer createObjectUniformBuffer(Device device) {
  AllocatedBuffer buf = createBuffer(device, sizeof(ObjectsBlock),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(vkMapMemory(device.device, buf.memory, 0, sizeof(ObjectsBlock), 0,
                       &mapped),
           "map object uniform");
  return ObjectUniformBuffer{buf.buffer, buf.memory,
                             static_cast<ObjectsBlock *>(mapped)};
}

VkFormat findDepthFormat(VkPhysicalDevice physical) {
  static const VkFormat candidates[] = {
      VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT,
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D16_UNORM,
  };
  auto it = std::ranges::find_if(candidates, [&](VkFormat candidate) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical, candidate, &props);
    return (props.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
           (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
  });
  if (it == std::ranges::end(candidates)) {
    spdlog::error("no sampled-depth-capable format found");
    exit(1);
  }
  return *it;
}

DepthBuffer createDepthBuffer(Device device, VkFormat format, uint32_t width,
                              uint32_t height, bool sampled) {
  VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (sampled)
    usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  AllocatedImage img = createImage(device, width, height, format, usage);

  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device.device, &vi, nullptr, &view),
           "create depth view");

  return DepthBuffer{img.image, img.memory, view, format};
}

SceneGrab createSceneGrab(Device device, VkFormat format, uint32_t width,
                          uint32_t height) {
  AllocatedImage img = createImage(device, width, height, format,
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                       VK_IMAGE_USAGE_SAMPLED_BIT);

  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device.device, &vi, nullptr, &view),
           "create grab view");

  return SceneGrab{img.image, img.memory, view};
}

SceneGrab createFloatTarget(Device device, VkFormat format, uint32_t width,
                            uint32_t height) {
  AllocatedImage img = createImage(device, width, height, format,
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                       VK_IMAGE_USAGE_SAMPLED_BIT);
  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device.device, &vi, nullptr, &view),
           "create float target view");
  return SceneGrab{img.image, img.memory, view};
}

SsrTarget createSsrTarget(Device device, uint32_t width, uint32_t height) {
  VkFormatProperties props;
  vkGetPhysicalDeviceFormatProperties(device.physical, SSR_FORMAT, &props);
  if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) ||
      !(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
    spdlog::error("SSR format cannot be storage+sampled");
    exit(1);
  }
  AllocatedImage img =
      createImage(device, width, height, SSR_FORMAT,
                  VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
  VkImageViewCreateInfo vi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = SSR_FORMAT,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
  };
  VkImageView view;
  CHECK_VK(vkCreateImageView(device.device, &vi, nullptr, &view),
           "create ssr view");
  return SsrTarget{img.image, img.memory, view};
}

SsrUniformBuffer createSsrUniformBuffer(Device device) {
  AllocatedBuffer buf =
      createBuffer(device, sizeof(SsrData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(
      vkMapMemory(device.device, buf.memory, 0, sizeof(SsrData), 0, &mapped),
      "map ssr uniform");
  return SsrUniformBuffer{buf.buffer, buf.memory,
                          static_cast<SsrData *>(mapped)};
}

ProbeUniformBuffer createProbeUniformBuffer(Device device) {
  AllocatedBuffer buf = createBuffer(device, sizeof(ProbeData),
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  void *mapped = nullptr;
  CHECK_VK(
      vkMapMemory(device.device, buf.memory, 0, sizeof(ProbeData), 0, &mapped),
      "map probe uniform");
  return ProbeUniformBuffer{buf.buffer, buf.memory,
                            static_cast<ProbeData *>(mapped)};
}

EnvCube createEnvCube(Device device, VkFormat format) {
  EnvCube env;

  AllocatedImage img = createImage(device, CUBE_SIZE, CUBE_SIZE, format,
                                   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                       VK_IMAGE_USAGE_SAMPLED_BIT |
                                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                   VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, 6);

  env.image = img.image;
  env.memory = img.memory;

  VkImageViewCreateInfo cvi = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = img.image,
      .viewType = VK_IMAGE_VIEW_TYPE_CUBE,
      .format = format,
      .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6},
  };
  CHECK_VK(vkCreateImageView(device.device, &cvi, nullptr, &env.cubeView),
           "create env cube view");

  for (uint32_t i = 0; i < 6; ++i) {
    VkImageViewCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = env.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, i, 1}};
    CHECK_VK(vkCreateImageView(device.device, &vi, nullptr, &env.faceViews[i]),
             "create env face view");
  }

  return env;
}

void destroyEnvCube(VkDevice device, EnvCube &env) {
  for (VkImageView view : env.faceViews) {
    vkDestroyImageView(device, view, nullptr);
  }
  vkDestroyImageView(device, env.cubeView, nullptr);
  vkDestroyImage(device, env.image, nullptr);
  vkFreeMemory(device, env.memory, nullptr);
  env = EnvCube{};
}

SplitDescriptors createSplitDescriptors(
    VkDevice device, const std::vector<Texture> &textures,
    CameraUniformBuffer *cameras, LightUniformBuffer *lights,
    MaterialUniformBuffer *materials, ObjectUniformBuffer *objects,
    SsrUniformBuffer *ssrUbos, std::span<const ProbeUniformBuffer> probes,
    const CameraUniformBuffer &bakeCamera, const LightUniformBuffer &bakeLights,
    const MaterialUniformBuffer &bakeMaterials,
    const ObjectUniformBuffer &bakeObjects, VkSampler sceneSampler,
    VkImageView sceneView, VkSampler envSampler,
    std::span<const VkImageView> envViews, VkSampler grabSampler,
    VkImageView normalView, VkSampler depthSampler, VkImageView depthView,
    VkSampler ssrSampler, VkImageView ssrView) {
  LDG_ASSERT(!textures.empty());
  LDG_ASSERT(!envViews.empty());
  LDG_ASSERT(probes.size() == envViews.size());

  // Set 0 Frame: camera b0, light b1, ssrParams b2, objects b3
  VkDescriptorSetLayoutBinding frameBindings[4] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 2,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 3,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
  };
  VkDescriptorSetLayoutCreateInfo frameLci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 4,
      .pBindings = frameBindings,
  };
  VkDescriptorSetLayout frameLayout;
  CHECK_VK(
      vkCreateDescriptorSetLayout(device, &frameLci, nullptr, &frameLayout),
      "create frame set layout");

  // Set 1 Material: texture b0, materials b1
  VkDescriptorSetLayoutBinding materialBindings[2] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
  };
  VkDescriptorSetLayoutCreateInfo materialLci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = materialBindings,
  };
  VkDescriptorSetLayout materialLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &materialLci, nullptr,
                                       &materialLayout),
           "create material set layout");

  // Set 2 Env: cube b0, probe b1
  VkDescriptorSetLayoutBinding envBindings[2] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
  };
  VkDescriptorSetLayoutCreateInfo envLci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = envBindings,
  };
  VkDescriptorSetLayout envLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &envLci, nullptr, &envLayout),
           "create env set layout");

  // Set 3 Pass: scene b0, normal b1, depth b2, ssr sampled b3, ssr storage b4
  VkDescriptorSetLayoutBinding passBindings[5] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 2,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 3,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 4,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
  };
  VkDescriptorSetLayoutCreateInfo passLci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 5,
      .pBindings = passBindings,
  };
  VkDescriptorSetLayout passLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &passLci, nullptr, &passLayout),
           "create pass set layout");

  uint32_t textureCount = static_cast<uint32_t>(textures.size());
  uint32_t envCount = static_cast<uint32_t>(envViews.size());

  uint32_t frameSetCount = MAX_FRAMES_IN_FLIGHT + 1;
  VkDescriptorPoolSize framePoolSizes[1] = {
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = frameSetCount * 4},
  };
  VkDescriptorPoolCreateInfo framePci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = frameSetCount,
      .poolSizeCount = 1,
      .pPoolSizes = framePoolSizes,
  };
  VkDescriptorPool framePool;
  CHECK_VK(vkCreateDescriptorPool(device, &framePci, nullptr, &framePool),
           "create frame pool");

  std::vector<VkDescriptorSetLayout> frameLayouts(frameSetCount, frameLayout);
  VkDescriptorSetAllocateInfo frameAi = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = framePool,
      .descriptorSetCount = frameSetCount,
      .pSetLayouts = frameLayouts.data(),
  };
  std::vector<VkDescriptorSet> frameAll(frameSetCount);
  CHECK_VK(vkAllocateDescriptorSets(device, &frameAi, frameAll.data()),
           "alloc frame sets");
  std::vector<VkDescriptorSet> frameSets(
      frameAll.begin(), frameAll.begin() + MAX_FRAMES_IN_FLIGHT);
  std::vector<VkDescriptorSet> frameBake(
      frameAll.begin() + MAX_FRAMES_IN_FLIGHT, frameAll.end());

  uint32_t materialSetCount =
      textureCount * MAX_FRAMES_IN_FLIGHT + textureCount;
  VkDescriptorPoolSize materialPoolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = materialSetCount},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = materialSetCount},
  };
  VkDescriptorPoolCreateInfo materialPci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = materialSetCount,
      .poolSizeCount = 2,
      .pPoolSizes = materialPoolSizes,
  };
  VkDescriptorPool materialPool;
  CHECK_VK(vkCreateDescriptorPool(device, &materialPci, nullptr, &materialPool),
           "create material pool");

  std::vector<VkDescriptorSetLayout> materialLayouts(materialSetCount,
                                                     materialLayout);
  VkDescriptorSetAllocateInfo materialAi = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = materialPool,
      .descriptorSetCount = materialSetCount,
      .pSetLayouts = materialLayouts.data(),
  };
  std::vector<VkDescriptorSet> materialAll(materialSetCount);
  CHECK_VK(vkAllocateDescriptorSets(device, &materialAi, materialAll.data()),
           "alloc material sets");
  std::vector<VkDescriptorSet> materialSets(
      materialAll.begin(),
      materialAll.begin() + textureCount * MAX_FRAMES_IN_FLIGHT);
  std::vector<VkDescriptorSet> materialBake(
      materialAll.begin() + textureCount * MAX_FRAMES_IN_FLIGHT,
      materialAll.end());

  VkDescriptorPoolSize envPoolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = envCount},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = envCount},
  };
  VkDescriptorPoolCreateInfo envPci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = envCount,
      .poolSizeCount = 2,
      .pPoolSizes = envPoolSizes,
  };
  VkDescriptorPool envPool;
  CHECK_VK(vkCreateDescriptorPool(device, &envPci, nullptr, &envPool),
           "create env pool");

  std::vector<VkDescriptorSetLayout> envLayouts(envCount, envLayout);
  VkDescriptorSetAllocateInfo envAi = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = envPool,
      .descriptorSetCount = envCount,
      .pSetLayouts = envLayouts.data(),
  };
  std::vector<VkDescriptorSet> envSets(envCount);
  CHECK_VK(vkAllocateDescriptorSets(device, &envAi, envSets.data()),
           "alloc env sets");

  // ---- Pass pool: per-frame sets, 4 samplers + 1 storage each ----
  VkDescriptorPoolSize passPoolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = MAX_FRAMES_IN_FLIGHT * 4},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = MAX_FRAMES_IN_FLIGHT},
  };
  VkDescriptorPoolCreateInfo passPci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
      .poolSizeCount = 2,
      .pPoolSizes = passPoolSizes,
  };
  VkDescriptorPool passPool;
  CHECK_VK(vkCreateDescriptorPool(device, &passPci, nullptr, &passPool),
           "create pass pool");

  std::vector<VkDescriptorSetLayout> passLayouts(MAX_FRAMES_IN_FLIGHT,
                                                 passLayout);
  VkDescriptorSetAllocateInfo passAi = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = passPool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = passLayouts.data(),
  };
  std::vector<VkDescriptorSet> passSets(MAX_FRAMES_IN_FLIGHT);
  CHECK_VK(vkAllocateDescriptorSets(device, &passAi, passSets.data()),
           "alloc pass sets");

  auto writeFrame = [&](VkDescriptorSet dst, VkBuffer cameraBuf,
                        VkBuffer lightBuf, VkBuffer ssrBuf,
                        VkBuffer objectsBuf) {
    VkDescriptorBufferInfo cameraInfo = {
        .buffer = cameraBuf,
        .offset = 0,
        .range = sizeof(CameraData),
    };
    VkDescriptorBufferInfo lightInfo = {
        .buffer = lightBuf,
        .offset = 0,
        .range = sizeof(LightData),
    };
    VkDescriptorBufferInfo ssrBufInfo = {
        .buffer = ssrBuf,
        .offset = 0,
        .range = sizeof(SsrData),
    };
    VkDescriptorBufferInfo objectsBufInfo = {
        .buffer = objectsBuf,
        .offset = 0,
        .range = sizeof(ObjectsBlock),
    };
    VkWriteDescriptorSet writes[4] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &cameraInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &lightInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 2,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &ssrBufInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 3,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &objectsBufInfo},
    };
    vkUpdateDescriptorSets(device, 4, writes, 0, nullptr);
  };

  auto writeMaterial = [&](VkDescriptorSet dst, uint32_t texIdx,
                           VkBuffer matBuf) {
    VkDescriptorImageInfo imageInfo = {
        .sampler = textures[texIdx].sampler,
        .imageView = textures[texIdx].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorBufferInfo materialInfo = {
        .buffer = matBuf,
        .offset = 0,
        .range = sizeof(MaterialsBlock),
    };
    VkWriteDescriptorSet writes[2] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &imageInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &materialInfo},
    };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
  };

  auto writeEnv = [&](VkDescriptorSet dst, uint32_t envIdx, VkBuffer probeBuf) {
    VkDescriptorImageInfo envInfo = {
        .sampler = envSampler,
        .imageView = envViews[envIdx],
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorBufferInfo probeBufInfo = {
        .buffer = probeBuf,
        .offset = 0,
        .range = sizeof(ProbeData),
    };
    VkWriteDescriptorSet writes[2] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &envInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &probeBufInfo},
    };
    vkUpdateDescriptorSets(device, 2, writes, 0, nullptr);
  };

  auto writePass = [&](VkDescriptorSet dst) {
    VkDescriptorImageInfo sceneInfo = {
        .sampler = sceneSampler,
        .imageView = sceneView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo normalInfo = {
        .sampler = grabSampler,
        .imageView = normalView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo depthInfo = {
        .sampler = depthSampler,
        .imageView = depthView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo ssrInfo = {
        .sampler = ssrSampler,
        .imageView = ssrView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo ssrStoreInfo = {
        .sampler = VK_NULL_HANDLE,
        .imageView = ssrView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet writes[5] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &sceneInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &normalInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 2,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &depthInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 3,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &ssrInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 4,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo = &ssrStoreInfo},
    };
    vkUpdateDescriptorSets(device, 5, writes, 0, nullptr);
  };

  for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
    writeFrame(frameSets[f], cameras[f].buffer, lights[f].buffer,
               ssrUbos[f].buffer, objects[f].buffer);
  }
  writeFrame(frameBake[0], bakeCamera.buffer, bakeLights.buffer,
             ssrUbos[0].buffer, bakeObjects.buffer);

  for (uint32_t t = 0; t < textureCount; ++t) {
    for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
      writeMaterial(materialSets[f * textureCount + t], t, materials[f].buffer);
    }
    writeMaterial(materialBake[t], t, bakeMaterials.buffer);
  }

  for (uint32_t e = 0; e < envCount; ++e) {
    writeEnv(envSets[e], e, probes[e].buffer);
  }

  for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
    writePass(passSets[f]);
  }

  SplitDescriptors out;
  out.frame = FrameSets{
      .layout = frameLayout,
      .pool = framePool,
      .sets = std::move(frameSets),
      .bakeSets = std::move(frameBake),
  };
  out.material = MaterialSets{
      .layout = materialLayout,
      .pool = materialPool,
      .sets = std::move(materialSets),
      .bakeSets = std::move(materialBake),
      .textureCount = textureCount,
  };
  out.env = EnvSets{
      .layout = envLayout,
      .pool = envPool,
      .sets = std::move(envSets),
  };
  out.pass = PassSets{
      .layout = passLayout,
      .pool = passPool,
      .sets = std::move(passSets),
  };
  out.textureCount = textureCount;
  out.envCount = envCount;
  return out;
}

void updatePassResizeDescriptors(VkDevice device, SplitDescriptors &descriptors,
                                 VkSampler grabSampler, VkImageView normalView,
                                 VkSampler depthSampler, VkImageView depthView,
                                 VkSampler ssrSampler, VkImageView ssrView,
                                 VkSampler sceneSampler,
                                 VkImageView sceneView) {
  for (VkDescriptorSet set : descriptors.pass.sets) {
    VkDescriptorImageInfo sceneInfo = {
        .sampler = sceneSampler,
        .imageView = sceneView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo normalInfo = {
        .sampler = grabSampler,
        .imageView = normalView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo depthInfo = {
        .sampler = depthSampler,
        .imageView = depthView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo ssrInfo = {
        .sampler = ssrSampler,
        .imageView = ssrView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo ssrStoreInfo = {
        .sampler = VK_NULL_HANDLE,
        .imageView = ssrView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };
    VkWriteDescriptorSet writes[5] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &sceneInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &normalInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 2,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &depthInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 3,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &ssrInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 4,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo = &ssrStoreInfo},
    };
    vkUpdateDescriptorSets(device, 5, writes, 0, nullptr);
  }
}

void destroySplitDescriptors(VkDevice device, SplitDescriptors &descriptors) {
  vkDestroyDescriptorPool(device, descriptors.frame.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptors.frame.layout, nullptr);
  vkDestroyDescriptorPool(device, descriptors.material.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptors.material.layout, nullptr);
  vkDestroyDescriptorPool(device, descriptors.env.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptors.env.layout, nullptr);
  vkDestroyDescriptorPool(device, descriptors.pass.pool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptors.pass.layout, nullptr);
  descriptors = SplitDescriptors{};
}
