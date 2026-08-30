#pragma once

#include "src/scene/game_object.hpp"

#include <deque>
#include <string_view>

class Scene {
public:
  GameObject &create(std::string name);

  GameObject *find(uint32_t id);
  const GameObject *find(uint32_t id) const;
  GameObject *findByName(std::string_view name);
  const GameObject *findByName(std::string_view name) const;

  const std::deque<GameObject> &objects() const { return m_objects; }
  std::deque<GameObject> &objects() { return m_objects; }

  void setMainCamera(uint32_t id) { m_mainCameraId = id; }

  GameObject *mainCamera();
  const GameObject *mainCamera() const;

  GameObject *mainLight();
  const GameObject *mainLight() const;

private:
  std::deque<GameObject> m_objects;
  uint32_t m_nextId = 1;
  uint32_t m_mainCameraId = 0;
};
