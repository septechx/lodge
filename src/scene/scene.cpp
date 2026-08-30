#include "scene.hpp"

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

GameObject *Scene::mainLight() {
  for (GameObject &object : m_objects) {
    if (object.light.has_value())
      return &object;
  }
  return nullptr;
}

const GameObject *Scene::mainLight() const {
  for (const GameObject &object : m_objects) {
    if (object.light.has_value())
      return &object;
  }
  return nullptr;
}
