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
  return serializeWithModels(
      [](ModelHandle) -> std::optional<std::string> { return std::nullopt; });
}

ser::Value Scene::serializeWithModels(const ModelPathForHandle &pathFor) const {
  auto objects =
      m_objects |
      std::views::transform([&](const GameObject &object) -> ser::Value {
        ser::Value rendererValue;
        if (object.renderer.has_value()) {
          std::optional<std::string> path;
          if (pathFor)
            path = pathFor(object.renderer->model);
          if (path.has_value()) {
            rendererValue = ser::Value::Map{{"model", *path}};
          } else {
            rendererValue = ser::Value::Map{
                {"model",
                 static_cast<ser::Value::Int>(object.renderer->model.index)}};
          }
        }
        ser::Value::Array scriptsValue{};
        if (!object.scripts.empty()) {
          scriptsValue.reserve(object.scripts.size());
          for (const ScriptRef &script : object.scripts) {
            scriptsValue.emplace_back(
                ser::Value{ser::Value::String{script.path}});
          }
        }
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
            {"renderer", rendererValue},
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
            {"scripts", object.scripts.empty() ? ser::Value{} : scriptsValue},
        };
      });
  ser::Value mainCameraValue;
  if (m_mainCameraId != 0) {
    if (const GameObject *cam = find(m_mainCameraId))
      mainCameraValue = ser::Value{cam->name};
  }
  return ser::Value::Map{
      {"objects", ser::Value::Array{objects.begin(), objects.end()}},
      {"mainCamera", mainCameraValue},
  };
}

void Scene::deserialize(ser::Value value) {
  deserializeWithModels(value,
                        [](const std::string &) -> std::optional<ModelHandle> {
                          return std::nullopt;
                        });
}

void Scene::deserializeWithModels(const ser::Value &value,
                                  const ModelHandleForPath &handleFor) {
  const auto &root = value.asMap();
  const auto &objects = root.at("objects").asArray();
  m_objects.clear();
  m_nextId = 1;
  m_mainCameraId = 0;
  for (size_t i = 0; i < objects.size(); ++i) {
    const auto &object = objects[i].asMap();
    GameObject &objectOut = create(object.at("name").asString());

    auto transformIt = object.find("transform");
    if (transformIt != object.end() && !transformIt->second.isNull()) {
      const auto &transform = transformIt->second.asMap();
      auto posIt = transform.find("position");
      if (posIt != transform.end() && !posIt->second.isNull()) {
        const auto &position = posIt->second.asArray();
        if (position.size() >= 3)
          objectOut.transform.position = {
              position[0].asReal(), position[1].asReal(), position[2].asReal()};
      }
      auto rotIt = transform.find("rotation");
      if (rotIt != transform.end() && !rotIt->second.isNull()) {
        const auto &rotation = rotIt->second.asArray();
        if (rotation.size() >= 4)
          objectOut.transform.rotation = {
              rotation[0].asReal(), rotation[1].asReal(), rotation[2].asReal(),
              rotation[3].asReal()};
      }
      auto scaleIt = transform.find("scale");
      if (scaleIt != transform.end() && !scaleIt->second.isNull()) {
        const auto &scale = scaleIt->second.asArray();
        if (scale.size() >= 3)
          objectOut.transform.scale = {scale[0].asReal(), scale[1].asReal(),
                                       scale[2].asReal()};
      }
    }

    auto rendererIt = object.find("renderer");
    if (rendererIt != object.end() && !rendererIt->second.isNull()) {
      const auto &rendererParams = rendererIt->second.asMap();
      auto modelIt = rendererParams.find("model");
      if (modelIt != rendererParams.end() && !modelIt->second.isNull()) {
        if (modelIt->second.isString()) {
          std::optional<ModelHandle> resolved;
          if (handleFor)
            resolved = handleFor(modelIt->second.asString());
          if (resolved.has_value())
            objectOut.renderer = ModelRenderer{*resolved};
          else
            objectOut.renderer.reset();
        } else {
          objectOut.renderer = ModelRenderer{
              ModelHandle{static_cast<uint32_t>(modelIt->second.asInt())}};
        }
      } else {
        objectOut.renderer.reset();
      }
    } else {
      objectOut.renderer.reset();
    }

    auto cameraIt = object.find("camera");
    if (cameraIt != object.end() && !cameraIt->second.isNull()) {
      const auto &cameraParams = cameraIt->second.asMap();
      CameraParams params;
      auto fovIt = cameraParams.find("fovY");
      if (fovIt != cameraParams.end() && !fovIt->second.isNull())
        params.fovY = fovIt->second.asReal();
      auto nearIt = cameraParams.find("nearZ");
      if (nearIt != cameraParams.end() && !nearIt->second.isNull())
        params.nearZ = nearIt->second.asReal();
      auto farIt = cameraParams.find("farZ");
      if (farIt != cameraParams.end() && !farIt->second.isNull())
        params.farZ = farIt->second.asReal();
      objectOut.camera = params;
    } else {
      objectOut.camera.reset();
    }

    auto lightIt = object.find("light");
    if (lightIt != object.end() && !lightIt->second.isNull()) {
      const auto &lightParams = lightIt->second.asMap();
      LightParams params;
      auto colorIt = lightParams.find("color");
      if (colorIt != lightParams.end() && !colorIt->second.isNull()) {
        const auto &color = colorIt->second.asArray();
        if (color.size() >= 3)
          params.color = {color[0].asReal(), color[1].asReal(),
                          color[2].asReal()};
      }
      objectOut.light = params;
    } else {
      objectOut.light.reset();
    }

    auto scriptsIt = object.find("scripts");
    if (scriptsIt != object.end() && !scriptsIt->second.isNull()) {
      if (scriptsIt->second.isArray()) {
        const auto &scriptRefs = scriptsIt->second.asArray();
        objectOut.scripts.clear();
        objectOut.scripts.reserve(scriptRefs.size());
        for (const auto &ref : scriptRefs) {
          if (ref.isString())
            objectOut.scripts.push_back(ScriptRef{ref.asString()});
        }
      } else if (scriptsIt->second.isString()) {
        objectOut.scripts = {ScriptRef{scriptsIt->second.asString()}};
      } else {
        objectOut.scripts = {};
      }
    } else {
      objectOut.scripts = {};
    }
  }

  auto mainCameraIt = root.find("mainCamera");
  if (mainCameraIt != root.end() && !mainCameraIt->second.isNull()) {
    if (mainCameraIt->second.isString()) {
      if (GameObject *cam = findByName(mainCameraIt->second.asString()))
        m_mainCameraId = cam->id;
    } else {
      uint32_t id = static_cast<uint32_t>(mainCameraIt->second.asInt());
      if (find(id) != nullptr)
        m_mainCameraId = id;
    }
  }
}
