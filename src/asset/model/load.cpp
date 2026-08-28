#include "load.hpp"

#include "../../math/Quat.hpp"
#include "../../math/Vec2.hpp"
#include "../../render/allocator.hpp"
#include "../../render/pipeline.hpp"
#include "../../render/vertex.hpp"
#include "../../utils.hpp"
#include "tiny_gltf_v3.h"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

static std::string_view severity(tg3_severity severity) {
  switch (severity) {
  case TG3_SEVERITY_INFO:
    return "INFO";
  case TG3_SEVERITY_WARNING:
    return "WARNING";
  case TG3_SEVERITY_ERROR:
    return "ERROR";
  }
  std::unreachable();
}

static Mat4 mat4FromNode(const tg3_node &node) {
  if (node.has_matrix) {
    Mat4 r;
    for (int i = 0; i < 16; ++i)
      r[i] = static_cast<float>(node.matrix[i]);
    return r;
  }
  Mat4 t = Mat4::translate({
      static_cast<float>(node.translation[0]),
      static_cast<float>(node.translation[1]),
      static_cast<float>(node.translation[2]),
  });
  Mat4 r = Mat4::fromQuat({
      static_cast<float>(node.rotation[3]),
      static_cast<float>(node.rotation[2]),
      static_cast<float>(node.rotation[1]),
      static_cast<float>(node.rotation[0]),
  });
  Mat4 s = Mat4::scale({
      static_cast<float>(node.scale[0]),
      static_cast<float>(node.scale[1]),
      static_cast<float>(node.scale[2]),
  });
  return t * r * s;
}

static size_t sizeComp(int32_t ct) {
  switch (ct) {
  case TG3_COMPONENT_TYPE_BYTE:
    return 1;
  case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
    return 1;
  case TG3_COMPONENT_TYPE_SHORT:
    return 2;
  case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
    return 2;
  case TG3_COMPONENT_TYPE_INT:
    return 4;
  case TG3_COMPONENT_TYPE_UNSIGNED_INT:
    return 4;
  case TG3_COMPONENT_TYPE_FLOAT:
    return 4;
  default:
    return 0;
  }
}

static size_t numComp(int32_t type) {
  switch (type) {
  case TG3_TYPE_SCALAR:
    return 1;
  case TG3_TYPE_VEC2:
    return 2;
  case TG3_TYPE_VEC3:
    return 3;
  case TG3_TYPE_VEC4:
    return 4;
  case TG3_TYPE_MAT4:
    return 16;
  default:
    return 0;
  }
}

static uint8_t *accessorBytes(const tinygltf3::Model &model, int32_t accIndex,
                              size_t *outBytes) {
  LDG_ASSERT(accIndex >= 0 &&
             static_cast<uint32_t>(accIndex) < model->accessors_count);
  const tg3_accessor *acc = &model->accessors[accIndex];
  LDG_ASSERT(acc->buffer_view >= 0 && static_cast<uint32_t>(acc->buffer_view) <
                                          model->buffer_views_count);
  const tg3_buffer_view *bv = &model->buffer_views[acc->buffer_view];
  LDG_ASSERT(bv->buffer >= 0 &&
             static_cast<uint32_t>(bv->buffer) < model->buffers_count);
  const tg3_buffer *buf = &model->buffers[bv->buffer];
  size_t cs = sizeComp(acc->component_type);
  size_t nc = numComp(acc->type);
  LDG_ASSERT(cs && nc);
  size_t total = static_cast<size_t>(acc->count) * nc * cs;
  LDG_ASSERT(bv->byte_offset + acc->byte_offset + total <=
             bv->byte_offset + bv->byte_length);
  LDG_ASSERT(bv->byte_offset + bv->byte_length <= buf->data.count);
  uint8_t *base = const_cast<uint8_t *>(buf->data.data + bv->byte_offset +
                                        acc->byte_offset);
  if (outBytes)
    *outBytes = total;
  return base;
}

static int32_t findAttribute(const tg3_primitive *prim, std::string_view name) {
  size_t nameLen = name.size();
  for (uint32_t ai = 0; ai < prim->attributes_count; ++ai) {
    const tg3_str_int_pair *kv = &prim->attributes[ai];
    if (kv->key.len == nameLen &&
        memcmp(kv->key.data, name.data(), nameLen) == 0) {
      return kv->value;
    }
  }
  return -1;
}

static Vec2 readTexcoord(const tinygltf3::Model &model, int32_t accIndex,
                         uint32_t vertexIdx) {
  if (accIndex < 0)
    return {0.0f, 0.0f};
  const tg3_accessor *acc = &model->accessors[accIndex];
  const tg3_buffer_view *bv = &model->buffer_views[acc->buffer_view];
  const tg3_buffer *buf = &model->buffers[bv->buffer];
  int32_t stride = tg3_accessor_byte_stride(acc, bv);
  if (stride <= 0)
    stride = static_cast<int32_t>(sizeComp(acc->component_type) *
                                  numComp(acc->type));
  const uint8_t *base = buf->data.data + bv->byte_offset + acc->byte_offset;
  const uint8_t *elem =
      base + static_cast<size_t>(vertexIdx) * static_cast<size_t>(stride);

  if (acc->component_type == TG3_COMPONENT_TYPE_FLOAT &&
      acc->type == TG3_TYPE_VEC2) {
    const float *f = reinterpret_cast<const float *>(elem);
    return {f[0], f[1]};
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE &&
      acc->type == TG3_TYPE_VEC2) {
    const uint8_t *u = elem;
    if (acc->normalized) {
      return {u[0] / 255.0f, u[1] / 255.0f};
    } else {
      return {static_cast<float>(u[0]), static_cast<float>(u[1])};
    }
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT &&
      acc->type == TG3_TYPE_VEC2) {
    const uint16_t *u = reinterpret_cast<const uint16_t *>(elem);
    if (acc->normalized) {
      return {u[0] / 65535.0f, u[1] / 65535.0f};
    } else {
      return {static_cast<float>(u[0]), static_cast<float>(u[1])};
    }
  }
  return {0.0f, 0.0f};
}

static Vec3 readPosition(const tinygltf3::Model &model, int32_t accIndex,
                         uint32_t vertexIdx) {
  const tg3_accessor *acc = &model->accessors[accIndex];
  const tg3_buffer_view *bv = &model->buffer_views[acc->buffer_view];
  const tg3_buffer *buf = &model->buffers[bv->buffer];
  int32_t stride = tg3_accessor_byte_stride(acc, bv);
  if (stride <= 0)
    stride = static_cast<int32_t>(sizeComp(acc->component_type) *
                                  numComp(acc->type));
  const uint8_t *base = buf->data.data + bv->byte_offset + acc->byte_offset;
  const uint8_t *elem =
      base + static_cast<size_t>(vertexIdx) * static_cast<size_t>(stride);
  const float *f = reinterpret_cast<const float *>(elem);
  return {f[0], f[1], f[2]};
}

static Vec3 readNormal(const tinygltf3::Model &model, int32_t accIndex,
                       uint32_t vertexIdx) {
  if (accIndex < 0)
    return {0.0f, 0.0f, 1.0f};
  const tg3_accessor *acc = &model->accessors[accIndex];
  const tg3_buffer_view *bv = &model->buffer_views[acc->buffer_view];
  const tg3_buffer *buf = &model->buffers[bv->buffer];
  int32_t stride = tg3_accessor_byte_stride(acc, bv);
  if (stride <= 0)
    stride = static_cast<int32_t>(sizeComp(acc->component_type) *
                                  numComp(acc->type));
  const uint8_t *base = buf->data.data + bv->byte_offset + acc->byte_offset;
  const uint8_t *elem =
      base + static_cast<size_t>(vertexIdx) * static_cast<size_t>(stride);

  if (acc->component_type == TG3_COMPONENT_TYPE_FLOAT &&
      acc->type == TG3_TYPE_VEC3) {
    const float *f = reinterpret_cast<const float *>(elem);
    return {f[0], f[1], f[2]};
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_BYTE &&
      acc->type == TG3_TYPE_VEC3) {
    const int8_t *b = reinterpret_cast<const int8_t *>(elem);
    if (acc->normalized) {
      return {std::max(b[0] / 127.0f, -1.0f), std::max(b[1] / 127.0f, -1.0f),
              std::max(b[2] / 127.0f, -1.0f)};
    } else {
      return {static_cast<float>(b[0]), static_cast<float>(b[1]),
              static_cast<float>(b[2])};
    }
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_BYTE &&
      acc->type == TG3_TYPE_VEC3) {
    const uint8_t *u = elem;
    if (acc->normalized) {
      return {u[0] / 255.0f, u[1] / 255.0f, u[2] / 255.0f};
    } else {
      return {static_cast<float>(u[0]), static_cast<float>(u[1]),
              static_cast<float>(u[2])};
    }
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_SHORT &&
      acc->type == TG3_TYPE_VEC3) {
    const int16_t *s = reinterpret_cast<const int16_t *>(elem);
    if (acc->normalized) {
      return {std::max(s[0] / 32767.0f, -1.0f),
              std::max(s[1] / 32767.0f, -1.0f),
              std::max(s[2] / 32767.0f, -1.0f)};
    } else {
      return {static_cast<float>(s[0]), static_cast<float>(s[1]),
              static_cast<float>(s[2])};
    }
  }
  if (acc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT &&
      acc->type == TG3_TYPE_VEC3) {
    const uint16_t *u = reinterpret_cast<const uint16_t *>(elem);
    if (acc->normalized) {
      return {u[0] / 65535.0f, u[1] / 65535.0f, u[2] / 65535.0f};
    } else {
      return {static_cast<float>(u[0]), static_cast<float>(u[1]),
              static_cast<float>(u[2])};
    }
  }
  return {0.0f, 0.0f, 1.0f};
}

static std::vector<Texture> loadGltfTextures(const Device &dev,
                                             const tinygltf3::Model &model) {
  std::vector<Texture> textures;
  textures.reserve(model->textures_count);

  for (uint32_t ti = 0; ti < model->textures_count; ++ti) {
    const tg3_texture *tex = &model->textures[ti];
    const tg3_sampler *sampler = nullptr;
    if (tex->sampler >= 0 &&
        static_cast<uint32_t>(tex->sampler) < model->samplers_count) {
      sampler = &model->samplers[tex->sampler];
    }

    if (tex->source < 0 ||
        static_cast<uint32_t>(tex->source) >= model->images_count) {
      spdlog::warn("texture {} has invalid source {}, using white", ti,
                   tex->source);
      textures.push_back(createWhiteTexture(dev));
      continue;
    }

    const tg3_image *img = &model->images[tex->source];

    if (img->buffer_view >= 0 &&
        static_cast<uint32_t>(img->buffer_view) < model->buffer_views_count) {
      const tg3_buffer_view *bv = &model->buffer_views[img->buffer_view];
      const tg3_buffer *buf = &model->buffers[bv->buffer];
      const uint8_t *data = buf->data.data + bv->byte_offset;
      size_t size = bv->byte_length;

      Texture t = createTextureFromMemory(dev, data, size, sampler);
      spdlog::debug("loaded glTF texture {} from image {} ({} bytes) -> {}x{}",
                    ti, tex->source, size, t.width, t.height);
      textures.push_back(t);
    } else {
      spdlog::warn("texture {} image {} has no buffer view, using white", ti,
                   tex->source);
      textures.push_back(createWhiteTexture(dev));
    }
  }

  return textures;
}

static std::vector<RenderObject>
buildObjects(const Device &dev, const tinygltf3::Model &model,
             const std::vector<Texture> &textures, uint32_t fallbackTex) {
  int32_t sceneIdx = model->default_scene;
  if (sceneIdx < 0 && model->scenes_count > 0)
    sceneIdx = 0;
  LDG_ASSERT(sceneIdx >= 0 &&
             static_cast<uint32_t>(sceneIdx) < model->scenes_count);
  const tg3_scene *scene = &model->scenes[sceneIdx];

  std::vector<RenderObject> objects;

  struct Frame {
    int32_t node;
    Mat4 parent;
  };
  std::vector<Frame> stack;
  for (int32_t i = static_cast<int32_t>(scene->nodes_count) - 1; i >= 0; --i)
    stack.push_back({scene->nodes[i], Mat4::IDENTITY});
  while (!stack.empty()) {
    Frame frame = stack.back();
    stack.pop_back();

    const tg3_node *node = &model->nodes[frame.node];
    Mat4 local = mat4FromNode(*node);
    Mat4 world = frame.parent * local;

    if (node->mesh >= 0) {
      LDG_ASSERT(static_cast<uint32_t>(node->mesh) < model->meshes_count);
      const tg3_mesh *mesh = &model->meshes[node->mesh];
      for (uint32_t pi = 0; pi < mesh->primitives_count; ++pi) {
        const tg3_primitive *prim = &mesh->primitives[pi];
        if (prim->indices < 0) {
          spdlog::warn("skipping non-indexed primitive");
          continue;
        }

        int posIdx = findAttribute(prim, "POSITION");
        int uvIdx = findAttribute(prim, "TEXCOORD_0");
        int normalIdx = findAttribute(prim, "NORMAL");
        LDG_ASSERT(posIdx >= 0);
        const tg3_accessor *posAcc = &model->accessors[posIdx];
        LDG_ASSERT(posAcc->type == TG3_TYPE_VEC3 &&
                   posAcc->component_type == TG3_COMPONENT_TYPE_FLOAT);
        uint32_t vertexCount = static_cast<uint32_t>(posAcc->count);

        std::vector<Vertex> vertices;
        vertices.reserve(vertexCount);
        for (uint32_t vi = 0; vi < vertexCount; ++vi) {
          Vec3 pos = readPosition(model, posIdx, vi);
          Vec2 uv = {0.0f, 0.0f};
          if (uvIdx >= 0) {
            uv = readTexcoord(model, uvIdx, vi);
          }
          Vec3 normal = readNormal(model, normalIdx, vi);
          vertices.push_back(Vertex{pos, uv, normal});
        }

        size_t idxBytes;
        uint8_t *idxData = accessorBytes(model, prim->indices, &idxBytes);
        const tg3_accessor *idxAcc = &model->accessors[prim->indices];
        VkIndexType idxType;
        if (idxAcc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
          idxType = VK_INDEX_TYPE_UINT16;
        else if (idxAcc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT)
          idxType = VK_INDEX_TYPE_UINT32;
        else {
          spdlog::error("unsupported index component {}",
                        idxAcc->component_type);
          exit(1);
        }

        AllocatedBuffer vbuf =
            createVertexBuffer(dev.device, dev.physical, vertices.data(),
                               vertices.size() * sizeof(Vertex));
        AllocatedBuffer ibuf =
            createIndexBuffer(dev.device, dev.physical, idxData, idxBytes);

        uint32_t texIndex = fallbackTex;
        float baseColorFactor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        bool doubleSided = false;
        if (prim->material >= 0 &&
            static_cast<uint32_t>(prim->material) < model->materials_count) {
          const tg3_material *mat = &model->materials[prim->material];
          doubleSided = mat->double_sided != 0;
          for (int c = 0; c < 4; ++c)
            baseColorFactor[c] = static_cast<float>(
                mat->pbr_metallic_roughness.base_color_factor[c]);
          int texIdx = mat->pbr_metallic_roughness.base_color_texture.index;
          if (texIdx >= 0 && static_cast<uint32_t>(texIdx) < textures.size()) {
            texIndex = static_cast<uint32_t>(texIdx);
          } else if (texIdx >= 0) {
            spdlog::warn("material {} baseColorTexture {} out of range, using "
                         "fallback {} ",
                         prim->material, texIdx, fallbackTex);
          }
        } else if (prim->material >= 0) {
          spdlog::warn("primitive material {} out of range", prim->material);
        }

        RenderObject obj{
            .worldMat = world,
            .vbuf = vbuf,
            .ibuf = ibuf,
            .indexCount = static_cast<uint32_t>(idxAcc->count),
            .indexType = idxType,
            .textureIndex = texIndex,
            .baseColorFactor = {baseColorFactor[0], baseColorFactor[1],
                                baseColorFactor[2], baseColorFactor[3]},
            .doubleSided = doubleSided,
        };
        objects.push_back(obj);
      }
    }

    for (int32_t ci = static_cast<int32_t>(node->children_count) - 1; ci >= 0;
         --ci) {
      stack.push_back({node->children[ci], world});
    }
  }

  return objects;
}

LoadedModel loadModel(const Device &dev, std::filesystem::path path) {
  tinygltf3::ErrorStack errors;
  tinygltf3::Model model;

  tg3_error_code err = tinygltf3::parse_file(model, errors, path.c_str());
  if (err != TG3_OK) {
    spdlog::error("parse {} failed: {}", path.string(), static_cast<int>(err));
    for (uint32_t i = 0; i < errors.count(); ++i)
      spdlog::error("[{}] {}", severity(errors.entry(i)->severity),
                    errors.entry(i)->message);
    exit(1);
  }
  spdlog::debug(
      "{}: {} scenes {} nodes {} meshes {} textures {} images {} materials",
      path.string(), model->scenes_count, model->nodes_count,
      model->meshes_count, model->textures_count, model->images_count,
      model->materials_count);

  std::vector<Texture> textures = loadGltfTextures(dev, model);

  uint32_t fallbackTex = 0;
  if (textures.empty()) {
    spdlog::warn("no glTF textures found, creating white fallback");
    Texture white = createWhiteTexture(dev);
    textures.push_back(white);
    fallbackTex = 0;
  } else {
    bool needFallback = false;
    for (uint32_t mi = 0; mi < model->meshes_count; ++mi) {
      const tg3_mesh *mesh = &model->meshes[mi];
      for (uint32_t pi = 0; pi < mesh->primitives_count; ++pi) {
        const tg3_primitive *prim = &mesh->primitives[pi];
        if (prim->material < 0) {
          needFallback = true;
        } else if (static_cast<uint32_t>(prim->material) <
                   model->materials_count) {
          const tg3_material *mat = &model->materials[prim->material];
          if (mat->pbr_metallic_roughness.base_color_texture.index < 0)
            needFallback = true;
        }
      }
    }
    if (needFallback) {
      Texture white = createWhiteTexture(dev);
      fallbackTex = static_cast<uint32_t>(textures.size());
      textures.push_back(white);
      spdlog::warn("added white fallback texture at index {}", fallbackTex);
    } else {
      fallbackTex = 0;
    }
  }

  std::vector<RenderObject> objects =
      buildObjects(dev, model, textures, fallbackTex);
  spdlog::debug("built {} objects with {} textures from glTF", objects.size(),
                textures.size());

  return {std::move(objects), std::move(textures)};
}
