#pragma once

#include "src/render/buffers.hpp"
#include "src/render/pipelines/pipeline.hpp"
#include "src/render/render.hpp"
#include "src/render/render_object.hpp"

#include <vulkan/vulkan.h>

#include <span>

void recordBakeFace(VkCommandBuffer cmd, GraphicsPipelines pipelines,
                    std::span<const RenderObject> objects,
                    const SplitDescriptors &descriptors, EnvCube env,
                    uint32_t face, VkImage faceDepthImage,
                    VkImageView faceDepthView);

void bakeEnvironment(Device device, GraphicsPipelines pipelines,
                     std::span<const RenderObject> objects,
                     const SplitDescriptors &descriptors,
                     std::span<const EnvCube> envs,
                     std::span<const Vec3> probes,
                     CameraUniformBuffer &bakeCamera, DepthBuffer &bakeDepth,
                     CmdBundle &bakeCmd, VkFence bakeFence);

void bakeOneFace(Device device, GraphicsPipelines pipelines,
                 std::span<const RenderObject> objects,
                 const SplitDescriptors &descriptors, EnvCube env, Vec3 probe,
                 uint32_t face, CameraUniformBuffer &bakeCamera,
                 DepthBuffer &bakeDepth, CmdBundle &bakeCmd, VkFence bakeFence);

bool tryBakeOneFaceAsync(
    Device device, GraphicsPipelines pipelines,
    std::span<const RenderObject> objects, const SplitDescriptors &descriptors,
    EnvCube env, Vec3 probe, uint32_t face, CameraUniformBuffer &bakeCamera,
    LightUniformBuffer &bakeLights, MaterialUniformBuffer &bakeMaterials,
    ObjectUniformBuffer &bakeObjects, const LightsBlock *freshLights,
    const MaterialsBlock *freshMaterials, const ObjectsBlock *freshObjects,
    DepthBuffer &bakeDepth, CmdBundle &bakeCmd, VkFence bakeFence,
    bool &pending);
