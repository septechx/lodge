#include "store.hpp"

#include "src/render/buffers.hpp"
#include "src/render/utils.hpp"
#include "src/render/vertex.hpp"

AssetStore::AssetStore(const Device &dev) : m_dev(dev) { createBuiltins(); }

void AssetStore::createBuiltins() {
  uint8_t white[4] = {255, 255, 255, 255};
  uint8_t yellow[4] = {255, 230, 64, 255};
  m_textures.push_back(createTextureFromPixels(m_dev, 1, 1, white));
  m_textures.push_back(createTextureFromPixels(m_dev, 1, 1, yellow));

  std::vector<Vertex> verts;
  verts.reserve(24);
  verts.push_back({{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {1, 0, 0}});
  verts.push_back({{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {1, 0, 0}});
  verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {1, 0, 0}});
  verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {1, 0, 0}});
  verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {-1, 0, 0}});
  verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {-1, 0, 0}});
  verts.push_back({{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {-1, 0, 0}});
  verts.push_back({{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {-1, 0, 0}});
  verts.push_back({{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f}, {0, 1, 0}});
  verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0, 1, 0}});
  verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0, 1, 0}});
  verts.push_back({{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f}, {0, 1, 0}});
  verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0, -1, 0}});
  verts.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, {0, -1, 0}});
  verts.push_back({{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}, {0, -1, 0}});
  verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0, -1, 0}});
  verts.push_back({{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}, {0, 0, 1}});
  verts.push_back({{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}, {0, 0, 1}});
  verts.push_back({{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}, {0, 0, 1}});
  verts.push_back({{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}, {0, 0, 1}});
  verts.push_back({{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0, 0, -1}});
  verts.push_back({{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}, {0, 0, -1}});
  verts.push_back({{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}, {0, 0, -1}});
  verts.push_back({{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0, 0, -1}});

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

  MeshHandle cube = createMesh(verts.data(), verts.size() * sizeof(Vertex),
                               indices.data(), indices.size() * sizeof(uint32_t),
                               static_cast<uint32_t>(indices.size()),
                               VK_INDEX_TYPE_UINT32);

  m_models.push_back(Model{
      .parts = {ModelPart{
          .mesh = cube,
          .material = Material{.texture = yellowTexture()},
          .local = Mat4::IDENTITY,
      }},
  });
}

MeshHandle AssetStore::createMesh(const void *vertices, size_t vertexBytes,
                                  const void *indices, size_t indexBytes,
                                  uint32_t indexCount, VkIndexType indexType) {
  m_meshes.push_back(GpuMesh{
      .vbuf = createVertexBuffer(m_dev, vertices, vertexBytes),
      .ibuf = createIndexBuffer(m_dev, indices, indexBytes),
      .indexCount = indexCount,
      .indexType = indexType,
  });
  return MeshHandle{static_cast<uint32_t>(m_meshes.size() - 1)};
}

TextureHandle AssetStore::addTexture(Texture texture) {
  m_textures.push_back(texture);
  return TextureHandle{static_cast<uint32_t>(m_textures.size() - 1)};
}

ModelHandle AssetStore::addModel(Model model) {
  m_models.push_back(std::move(model));
  return ModelHandle{static_cast<uint32_t>(m_models.size() - 1)};
}

const GpuMesh &AssetStore::mesh(MeshHandle handle) const {
  return m_meshes[handle.index];
}

const Model &AssetStore::model(ModelHandle handle) const {
  return m_models[handle.index];
}

AssetStore::~AssetStore() {
  const VkDevice device = m_dev.device;
  CHECK_VK(vkDeviceWaitIdle(device), "asset store idle");

  for (const GpuMesh &mesh : m_meshes) {
    vkDestroyBuffer(device, mesh.vbuf.buffer, nullptr);
    vkFreeMemory(device, mesh.vbuf.memory, nullptr);
    vkDestroyBuffer(device, mesh.ibuf.buffer, nullptr);
    vkFreeMemory(device, mesh.ibuf.memory, nullptr);
  }

  for (const Texture &tex : m_textures) {
    vkDestroySampler(device, tex.sampler, nullptr);
    vkDestroyImageView(device, tex.view, nullptr);
    vkDestroyImage(device, tex.image, nullptr);
    vkFreeMemory(device, tex.memory, nullptr);
  }
}
