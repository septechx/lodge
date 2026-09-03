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
    return props.optimalTilingFeatures &
           VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  });
  if (it == std::ranges::end(candidates)) {
    spdlog::error("no supported depth format found");
    exit(1);
  }
  return *it;
}

DepthBuffer createDepthBuffer(Device device, VkFormat format, uint32_t width,
                              uint32_t height) {
  AllocatedImage img = createImage(device, width, height, format,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

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

SceneDescriptors
createSceneDescriptors(VkDevice device, const std::vector<Texture> &textures,
                       CameraUniformBuffer *cameras, LightUniformBuffer *lights,
                       MaterialUniformBuffer *materials, VkSampler sceneSampler,
                       VkImageView sceneView, VkSampler envSampler,
                       std::span<const VkImageView> envViews) {
  LDG_ASSERT(!textures.empty());
  LDG_ASSERT(!envViews.empty());

  VkDescriptorSetLayoutBinding bindings[6] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
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
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}};
  VkDescriptorSetLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 6,
      .pBindings = bindings,
  };
  VkDescriptorSetLayout setLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &lci, nullptr, &setLayout),
           "create set layout");

  uint32_t textureCount = static_cast<uint32_t>(textures.size());
  uint32_t envCount = static_cast<uint32_t>(envViews.size());
  uint32_t setCount = textureCount * envCount * MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolSize poolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = setCount * 3},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = setCount * 3},
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
      for (uint32_t e = 0; e < envCount; ++e) {
        uint32_t idx = (f * envCount + e) * textureCount + t;
        VkDescriptorImageInfo imageInfo = {
            .sampler = textures[t].sampler,
            .imageView = textures[t].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkDescriptorBufferInfo lightInfo = {
            .buffer = lights[f].buffer,
            .offset = 0,
            .range = sizeof(LightData),
        };
        VkDescriptorBufferInfo cameraInfo = {
            .buffer = cameras[f].buffer,
            .offset = 0,
            .range = sizeof(CameraData),
        };
        VkDescriptorBufferInfo materialInfo = {
            .buffer = materials[f].buffer,
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
            .imageView = envViews[e],
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet writes[6] = {
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
             .pBufferInfo = &cameraInfo},
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet = sets[idx],
             .dstBinding = 2,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
             .pBufferInfo = &lightInfo},
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet = sets[idx],
             .dstBinding = 3,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
             .pBufferInfo = &materialInfo},
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet = sets[idx],
             .dstBinding = 4,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             .pImageInfo = &sceneInfo},
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
             .dstSet = sets[idx],
             .dstBinding = 5,
             .dstArrayElement = 0,
             .descriptorCount = 1,
             .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
             .pImageInfo = &envInfo}};
        vkUpdateDescriptorSets(device, 6, writes, 0, nullptr);
      }
    }
  }

  return SceneDescriptors{
      .layout = setLayout,
      .pool = pool,
      .sets = std::move(sets),
      .textureCount = textureCount,
      .envCount = envCount,
  };
}

void updateSceneGrabDescriptors(VkDevice device,
                                const SceneDescriptors &descriptors,
                                VkSampler sceneSampler, VkImageView sceneView) {
  for (VkDescriptorSet set : descriptors.sets) {
    VkDescriptorImageInfo sceneInfo = {
        .sampler = sceneSampler,
        .imageView = sceneView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = 4,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &sceneInfo,
    };
    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
  }
}
