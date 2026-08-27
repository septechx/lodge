#include "load.hpp"

#include "../math/Quat.hpp"
#include "../utils.hpp"
#include "src/render/allocator.hpp"
#include "src/render/pipeline.hpp"
#include "tiny_gltf_v3.h"

#include <cstdint>
#include <cstdlib>
#include <print>
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
  size_t total = (size_t)acc->count * nc * cs;
  LDG_ASSERT(bv->byte_offset + acc->byte_offset + total <=
             bv->byte_offset + bv->byte_length);
  LDG_ASSERT(bv->byte_offset + bv->byte_length <= buf->data.count);
  uint8_t *base = const_cast<uint8_t *>(buf->data.data + bv->byte_offset +
                                        acc->byte_offset);
  if (outBytes)
    *outBytes = total;
  return base;
}

static std::vector<RenderObject> buildObjects(const Device &dev,
                                              const tinygltf3::Model &model) {
  LDG_ASSERT(model->scenes_count > 0);
  const tg3_scene *scene = &model->scenes[model->default_scene];

  std::vector<RenderObject> objects;

  struct Frame {
    int32_t node;
    Mat4 parent;
  };
  std::vector<Frame> stack;
  for (int32_t i = scene->nodes_count - 1; i >= 0; --i)
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
          std::println(stderr, "skipping non-indexed primitive");
          continue;
        }

        int posIdx = -1;
        for (uint32_t ai = 0; ai < prim->attributes_count; ++ai) {
          const tg3_str_int_pair *kv = &prim->attributes[ai];
          if (kv->key.len == 8 && memcmp(kv->key.data, "POSITION", 8) == 0) {
            posIdx = kv->value;
            break;
          }
        }
        LDG_ASSERT(posIdx >= 0);

        const tg3_accessor *posAcc = &model->accessors[posIdx];
        LDG_ASSERT(posAcc->type == TG3_TYPE_VEC3 &&
                   posAcc->component_type == TG3_COMPONENT_TYPE_FLOAT);
        size_t posBytes;
        uint8_t *posData = accessorBytes(model, posIdx, &posBytes);
        size_t idxBytes;
        uint8_t *idxData = accessorBytes(model, prim->indices, &idxBytes);
        const tg3_accessor *idxAcc = &model->accessors[prim->indices];
        VkIndexType idxType;
        if (idxAcc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_SHORT)
          idxType = VK_INDEX_TYPE_UINT16;
        else if (idxAcc->component_type == TG3_COMPONENT_TYPE_UNSIGNED_INT)
          idxType = VK_INDEX_TYPE_UINT32;
        else {
          std::println(stderr, "unsupported index component {}",
                       idxAcc->component_type);
          exit(1);
        }

        AllocatedBuffer vbuf =
            createVertexBuffer(dev.device, dev.physical, posData, posBytes);
        AllocatedBuffer ibuf =
            createIndexBuffer(dev.device, dev.physical, idxData, idxBytes);

        objects.push_back(RenderObject{
            world, vbuf, ibuf, static_cast<uint32_t>(idxAcc->count), idxType});
      }
    }

    for (int32_t ci = (int32_t)node->children_count - 1; ci >= 0; --ci) {
      stack.push_back({node->children[ci], world});
    }
  }

  return objects;
}

std::vector<RenderObject> loadModel(const Device &dev,
                                    std::filesystem::path path) {
  tinygltf3::ErrorStack errors;
  tinygltf3::Model model;

  tg3_error_code err = tinygltf3::parse_file(model, errors, path.c_str());
  if (err != TG3_OK) {
    std::println(stderr, "parse {} failed: {}", path.string(),
                 static_cast<int>(err));
    for (uint32_t i = 0; i < errors.count(); ++i)
      std::println(stderr, "[{}] {}", severity(errors.entry(i)->severity),
                   errors.entry(i)->message);
    exit(1);
  }
  std::println(stderr, "{}: {} scenes {} nodes {} meshes", path.string(),
               model->scenes_count, model->nodes_count, model->meshes_count);

  std::vector<RenderObject> draws = buildObjects(dev, model);
  std::println(stderr, "built {} objects from glTF", draws.size());
  return draws;
}
