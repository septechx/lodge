#include "scene.hpp"

#include "src/scene/game_object.hpp"
#include "src/serialize/serialize.hpp"

#include <ranges>
#include <utility>

GameObject &Scene::create(std::string name) {
  m_objects.push_back({.id = m_nextId++, .name = std::move(name)});
  return m_objects.back();
}

GameObject *Scene::find(uint32_t id) {
  for (GameObject &object : m_objects) {
    if (object.id == id)
      return &object;
  }
  return nullptr;
}

const GameObject *Scene::find(uint32_t id) const {
  for (const GameObject &object : m_objects) {
    if (object.id == id)
      return &object;
  }
  return nullptr;
}

GameObject *Scene::findByName(std::string_view name) {
  for (GameObject &object : m_objects) {
    if (object.name == name)
      return &object;
  }
  return nullptr;
}

const GameObject *Scene::findByName(std::string_view name) const {
  for (const GameObject &object : m_objects) {
    if (object.name == name)
      return &object;
  }
  return nullptr;
}

GameObject *Scene::mainCamera() {
  GameObject *object = find(m_mainCameraId);
  if (object != nullptr && object->camera.has_value())
    return object;
  return nullptr;
}

const GameObject *Scene::mainCamera() const {
  const GameObject *object = find(m_mainCameraId);
  if (object != nullptr && object->camera.has_value())
    return object;
  return nullptr;
}

ser::Value Scene::serialize() const {
  auto objects =
      m_objects |
      std::views::transform([](const GameObject &object) -> ser::Value {
        return ser::Value::Map{
            {"name", object.name},
            {"transform",
             ser::Value::Map{
                 {"position", ser::Value::Array{object.transform.position.x,
                                                object.transform.position.y,
                                                object.transform.position.z}},
                 {"rotation", ser::Value::Array{object.transform.rotation.w,
                                                object.transform.rotation.x,
                                                object.transform.rotation.y,
                                                object.transform.rotation.z}},
                 {"scale", ser::Value::Array{object.transform.scale.x,
                                             object.transform.scale.y,
                                             object.transform.scale.z}},
             }},
            {"renderer",
             object.renderer.has_value()
                 ? ser::Value{ser::Value::Map{
                       {"model", static_cast<ser::Value::Int>(
                                     object.renderer->model.index)}}}
                 : ser::Value{}},
            {"camera",
             object.camera.has_value()
                 ? ser::Value{ser::Value::Map{{"fovY", object.camera->fovY},
                                              {"nearZ", object.camera->nearZ},
                                              {"farZ", object.camera->farZ}}}
                 : ser::Value{}},
            {"light",
             object.light.has_value()
                 ? ser::Value{ser::Value::Map{
                       {"color", ser::Value::Array{object.light->color.x,
                                                   object.light->color.y,
                                                   object.light->color.z}}}}
                 : ser::Value{}},
        };
      });
  return ser::Value::Map{
      {"objects", ser::Value::Array{objects.begin(), objects.end()}},
  };
}

void Scene::deserialize(ser::Value value) {
  const auto &objects = value.asMap().at("objects").asArray();
  m_objects.clear();
  m_nextId = 1;
  m_mainCameraId = 0;
  for (size_t i = 0; i < objects.size(); ++i) {
    const auto &object = objects[i].asMap();
    GameObject &objectOut = create(object.at("name").asString());

    const auto &transform = object.at("transform").asMap();
    const auto &position = transform.at("position").asArray();
    objectOut.transform.position = {position[0].asReal(), position[1].asReal(),
                                    position[2].asReal()};
    const auto &rotation = transform.at("rotation").asArray();
    objectOut.transform.rotation = {rotation[0].asReal(), rotation[1].asReal(),
                                    rotation[2].asReal(), rotation[3].asReal()};
    const auto &scale = transform.at("scale").asArray();
    objectOut.transform.scale = {scale[0].asReal(), scale[1].asReal(),
                                 scale[2].asReal()};

    const auto &renderer = object.at("renderer");
    if (!renderer.isNull()) {
      const auto &rendererParams = renderer.asMap();
      objectOut.renderer = ModelRenderer{ModelHandle{
          static_cast<uint32_t>(rendererParams.at("model").asInt())}};
    } else {
      objectOut.renderer.reset();
    }

    const auto &camera = object.at("camera");
    if (!camera.isNull()) {
      const auto &cameraParams = camera.asMap();
      CameraParams params;
      params.fovY = cameraParams.at("fovY").asReal();
      params.nearZ = cameraParams.at("nearZ").asReal();
      params.farZ = cameraParams.at("farZ").asReal();
      objectOut.camera = params;
    } else {
      objectOut.camera.reset();
    }

    const auto &light = object.at("light");
    if (!light.isNull()) {
      const auto &lightParams = light.asMap();
      const auto &color = lightParams.at("color").asArray();
      LightParams params;
      params.color = {color[0].asReal(), color[1].asReal(), color[2].asReal()};
      objectOut.light = params;
    } else {
      objectOut.light.reset();
    }
  }
}
