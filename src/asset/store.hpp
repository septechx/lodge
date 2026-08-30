#pragma once

#include "src/asset/handles.hpp"
#include "src/asset/texture/load.hpp"
#include "src/math/Mat4.hpp"
#include "src/render/allocator.hpp"
#include "src/render/device.hpp"
#include "src/scene/material.hpp"

#include <vector>

struct GpuMesh {
  AllocatedBuffer vbuf;
  AllocatedBuffer ibuf;
  uint32_t indexCount;
  VkIndexType indexType;
};

struct ModelPart {
  MeshHandle mesh;
  Material material;
  Mat4 local;
};

struct Model {
  std::vector<ModelPart> parts;
};

class AssetStore {
public:
  explicit AssetStore(const Device &dev);
  ~AssetStore();

  AssetStore(const AssetStore &) = delete;
  AssetStore &operator=(const AssetStore &) = delete;

  MeshHandle createMesh(const void *vertices, size_t vertexBytes,
                        const void *indices, size_t indexBytes,
                        uint32_t indexCount, VkIndexType indexType);
  TextureHandle addTexture(Texture texture);
  ModelHandle addModel(Model model);

  TextureHandle whiteTexture() const { return TextureHandle{0}; }
  TextureHandle yellowTexture() const { return TextureHandle{1}; }
  MeshHandle unitCube() const { return MeshHandle{0}; }
  ModelHandle gizmoModel() const { return ModelHandle{0}; }

  const Device &device() const { return m_dev; }

  const GpuMesh &mesh(MeshHandle handle) const;
  const Model &model(ModelHandle handle) const;
  const std::vector<Texture> &textures() const { return m_textures; }

private:
  void createBuiltins();

  const Device &m_dev;
  std::vector<GpuMesh> m_meshes;
  std::vector<Texture> m_textures;
  std::vector<Model> m_models;
};
