#include "buffers.hpp"

#include "src/consts.hpp"
#include "src/render/utils.hpp"
#include "src/utils.hpp"
#include <vulkan/vulkan_core.h>

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

SceneDescriptors
createSceneDescriptors(VkDevice device, const std::vector<Texture> &textures,
                       CameraUniformBuffer *cameras, LightUniformBuffer *lights,
                       MaterialUniformBuffer *materials, VkSampler sceneSampler,
                       VkImageView sceneView) {
  LDG_ASSERT(!textures.empty());

  VkDescriptorSetLayoutBinding bindings[5] = {
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
       .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}};
  VkDescriptorSetLayoutCreateInfo lci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 5,
      .pBindings = bindings,
  };
  VkDescriptorSetLayout setLayout;
  CHECK_VK(vkCreateDescriptorSetLayout(device, &lci, nullptr, &setLayout),
           "create set layout");

  uint32_t textureCount = static_cast<uint32_t>(textures.size());
  uint32_t setCount = textureCount * MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolSize poolSizes[2] = {
      {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
       .descriptorCount = setCount * 2},
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
      uint32_t idx = f * textureCount + t;
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
      VkWriteDescriptorSet writes[5] = {
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
           .pImageInfo = &sceneInfo}};
      vkUpdateDescriptorSets(device, 5, writes, 0, nullptr);
    }
  }

  return SceneDescriptors{
      .layout = setLayout,
      .pool = pool,
      .sets = std::move(sets),
      .textureCount = textureCount,
  };
}
