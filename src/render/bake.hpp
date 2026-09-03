#pragma once

#include "src/render/buffers.hpp"
#include "src/render/pipelines/pipeline.hpp"
#include "src/render/render_object.hpp"

#include <vulkan/vulkan.h>

#include <span>

void recordBakeFace(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                    std::span<const RenderObject> objects,
                    const SceneDescriptors &descriptors, EnvCube env,
                    uint32_t face, VkImage faceDepthImage,
                    VkImageView faceDepthView);

void bakeEnvironment(Device device, GraphicsPipelines pipelines,
                     std::span<const RenderObject> objects,
                     const SceneDescriptors &descriptors,
                     std::span<const EnvCube> envs,
                     std::span<const Vec3> probes,
                     CameraUniformBuffer *cameras);
