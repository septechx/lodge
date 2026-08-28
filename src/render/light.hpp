#pragma once

#include "../math/Vec3.hpp"
#include "init.hpp"
#include "pipeline.hpp"
#include "render_object.hpp"
#include "vertex.hpp"

#include <vector>

struct Light {
  Vec3 pos{4.0f, 4.0f, 4.0f};
  bool showGizmo = false;
  float gizmoSize = 0.2f;
  RenderObject gizmo{};
  bool hasGizmo = false;

  void createGizmo(const Device &dev, size_t textureCount) {
    std::vector<Vertex> verts;
    verts.reserve(24);
    verts.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {1, 0, 0}});
    verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {1, 0, 0}});
    verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {1, 0, 0}});
    verts.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {1, 0, 0}});
    verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {-1, 0, 0}});
    verts.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {-1, 0, 0}});
    verts.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {-1, 0, 0}});
    verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {-1, 0, 0}});
    verts.push_back({{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}, {0, 1, 0}});
    verts.push_back({{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}, {0, 1, 0}});
    verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0, 1, 0}});
    verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0, 1, 0}});
    verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0, -1, 0}});
    verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0, -1, 0}});
    verts.push_back({{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, {0, -1, 0}});
    verts.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {0, -1, 0}});
    verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0, 0, 1}});
    verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0, 0, 1}});
    verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0, 0, 1}});
    verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0, 0, 1}});
    verts.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0, 0, -1}});
    verts.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0, 0, -1}});
    verts.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0, 0, -1}});
    verts.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0, 0, -1}});

    std::vector<uint32_t> indices;
    indices.reserve(36);
    for (uint32_t f = 0; f < 6; ++f) {
      uint32_t b = f * 4;
      indices.push_back(b + 0);
      indices.push_back(b + 1);
      indices.push_back(b + 2);
      indices.push_back(b + 0);
      indices.push_back(b + 2);
      indices.push_back(b + 3);
    }

    AllocatedBuffer vbuf = createVertexBuffer(
        dev.device, dev.physical, verts.data(), verts.size() * sizeof(Vertex));
    AllocatedBuffer ibuf =
        createIndexBuffer(dev.device, dev.physical, indices.data(),
                          indices.size() * sizeof(uint32_t));

    gizmo = RenderObject{
        .worldMat = Mat4::IDENTITY,
        .vbuf = vbuf,
        .ibuf = ibuf,
        .indexCount = static_cast<uint32_t>(indices.size()),
        .indexType = VK_INDEX_TYPE_UINT32,
        .textureIndex = static_cast<uint32_t>(textureCount),
    };
    hasGizmo = true;
  }

  void destroyGizmo(const Device &dev) {
    if (!hasGizmo)
      return;
    VkDevice device = dev.device;
    vkDestroyBuffer(device, gizmo.vbuf.buffer, nullptr);
    vkFreeMemory(device, gizmo.vbuf.memory, nullptr);
    vkDestroyBuffer(device, gizmo.ibuf.buffer, nullptr);
    vkFreeMemory(device, gizmo.ibuf.memory, nullptr);
    hasGizmo = false;
  }
};
