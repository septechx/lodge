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

SceneDescriptors createSceneDescriptors(
    VkDevice device, const std::vector<Texture> &textures,
    CameraUniformBuffer *cameras, LightUniformBuffer *lights,
    MaterialUniformBuffer *materials, SsrUniformBuffer *ssrUbos,
    std::span<const ProbeUniformBuffer> probes,
    const CameraUniformBuffer &bakeCamera, const LightUniformBuffer &bakeLights,
    const MaterialUniformBuffer &bakeMaterials, VkSampler sceneSampler,
    VkImageView sceneView, VkSampler envSampler,
    std::span<const VkImageView> envViews, VkSampler grabSampler,
    VkImageView normalView, VkSampler depthSampler, VkImageView depthView,
    VkSampler ssrSampler, VkImageView ssrView) {
  LDG_ASSERT(!textures.empty());
  LDG_ASSERT(!envViews.empty());
  LDG_ASSERT(probes.size() == envViews.size());

  VkDescriptorSetLayoutBinding bindings[12] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 2,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 3,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 4,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 5,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 6,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 7,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 8,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 9,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 10,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 11,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}};
  VkDescriptorSetLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 12,
      .pBindings = bindings,
  };
  VkDescriptorSetLayout setLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &lci, nullptr, &setLayout),
           "create set layout");

  uint32_t textureCount = static_cast<uint32_t>(textures.size());
  uint32_t envCount = static_cast<uint32_t>(envViews.size());
  uint32_t setCount = textureCount * envCount * MAX_FRAMES_IN_FLIGHT;
  uint32_t bakeCount = textureCount;
  uint32_t totalSets = setCount + bakeCount;

  VkDescriptorPoolSize poolSizes[3] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = totalSets * 6},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = totalSets * 5},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = totalSets},
  };
  VkDescriptorPoolCreateInfo pci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = totalSets,
      .poolSizeCount = 3,
      .pPoolSizes = poolSizes,
  };
  VkDescriptorPool pool;
  CHECK_VK(vkCreateDescriptorPool(device, &pci, nullptr, &pool),
           "create descriptor pool");

  std::vector<VkDescriptorSetLayout> layouts(totalSets, setLayout);
  VkDescriptorSetAllocateInfo ai = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = totalSets,
      .pSetLayouts = layouts.data(),
  };
  std::vector<VkDescriptorSet> allSets(totalSets);
  CHECK_VK(vkAllocateDescriptorSets(device, &ai, allSets.data()), "alloc sets");
  std::vector<VkDescriptorSet> sets(allSets.begin(),
                                    allSets.begin() + setCount);
  std::vector<VkDescriptorSet> bakeSets(allSets.begin() + setCount,
                                        allSets.end());

  auto writeSet = [&](VkDescriptorSet dst, uint32_t texIdx, VkBuffer cameraBuf,
                      VkBuffer lightBuf, VkBuffer matBuf, VkBuffer ssrBuf,
                      VkBuffer probeBuf, uint32_t envIdx) {
    VkDescriptorImageInfo imageInfo = {
        .sampler = textures[texIdx].sampler,
        .imageView = textures[texIdx].view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorBufferInfo lightInfo = {
        .buffer = lightBuf,
        .offset = 0,
        .range = sizeof(LightData),
    };
    VkDescriptorBufferInfo cameraInfo = {
        .buffer = cameraBuf,
        .offset = 0,
        .range = sizeof(CameraData),
    };
    VkDescriptorBufferInfo materialInfo = {
        .buffer = matBuf,
        .offset = 0,
        .range = sizeof(MaterialsBlock),
    };
    VkDescriptorImageInfo sceneInfo = {
        .sampler = sceneSampler,
        .imageView = sceneView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkDescriptorImageInfo envInfo = {
        .sampler = envSampler,
        .imageView = envViews[envIdx],
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
    VkDescriptorBufferInfo ssrBufInfo = {
        .buffer = ssrBuf,
        .offset = 0,
        .range = sizeof(SsrData),
    };
    VkDescriptorBufferInfo probeBufInfo = {
        .buffer = probeBuf,
        .offset = 0,
        .range = sizeof(ProbeData),
    };
    VkWriteDescriptorSet writes[12] = {
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
         .pBufferInfo = &cameraInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 2,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &lightInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 3,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &materialInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 4,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &sceneInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 5,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &envInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 6,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &normalInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 7,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &depthInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 8,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &ssrInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 9,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo = &ssrStoreInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 10,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &ssrBufInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = dst,
         .dstBinding = 11,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
         .pBufferInfo = &probeBufInfo},
    };
    vkUpdateDescriptorSets(device, 12, writes, 0, nullptr);
  };

  for (uint32_t t = 0; t < textureCount; ++t) {
    for (int f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f) {
      for (uint32_t e = 0; e < envCount; ++e) {
        uint32_t idx = (f * envCount + e) * textureCount + t;
        writeSet(sets[idx], t, cameras[f].buffer, lights[f].buffer,
                 materials[f].buffer, ssrUbos[f].buffer, probes[e].buffer, e);
      }
    }
  }
  for (uint32_t t = 0; t < textureCount; ++t) {
    writeSet(bakeSets[t], t, bakeCamera.buffer, bakeLights.buffer,
             bakeMaterials.buffer, ssrUbos[0].buffer, probes[0].buffer, 0);
  }

  return SceneDescriptors{
      .layout = setLayout,
      .pool = pool,
      .sets = std::move(sets),
      .textureCount = textureCount,
      .envCount = envCount,
      .bakeSets = std::move(bakeSets),
  };
}

void updateSsrResizeDescriptors(VkDevice device,
                                const SceneDescriptors &descriptors,
                                VkSampler grabSampler, VkImageView normalView,
                                VkSampler depthSampler, VkImageView depthView,
                                VkSampler ssrSampler, VkImageView ssrView,
                                VkSampler sceneSampler, VkImageView sceneView) {
  auto updateOne = [&](VkDescriptorSet set) {
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
    VkDescriptorImageInfo sceneInfo = {
        .sampler = sceneSampler,
        .imageView = sceneView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet writes[5] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 4,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &sceneInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 6,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &normalInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 7,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &depthInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 8,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
         .pImageInfo = &ssrInfo},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = set,
         .dstBinding = 9,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo = &ssrStoreInfo},
    };
    vkUpdateDescriptorSets(device, 5, writes, 0, nullptr);
  };
  for (VkDescriptorSet set : descriptors.sets)
    updateOne(set);
  for (VkDescriptorSet set : descriptors.bakeSets)
    updateOne(set);
}
