#pragma once

#include "src/scene/game_object.hpp"
#include "src/serialize/serialize.hpp"

#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

class Scene : public ser::Serializable, public ser::Deserialazable {
public:
  using ModelPathForHandle =
      std::function<std::optional<std::string>(ModelHandle)>;
  using ModelHandleForPath =
      std::function<std::optional<ModelHandle>(const std::string &)>;

  static constexpr const char *BUILTIN_GIZMO = "builtin:gizmo";

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

  ser::Value serialize() const override;
  void deserialize(ser::Value value) override;

  ser::Value serializeWithModels(const ModelPathForHandle &pathFor) const;
  void deserializeWithModels(const ser::Value &value,
                             const ModelHandleForPath &handleFor);

private:
  std::deque<GameObject> m_objects;
  uint32_t m_nextId = 1;
  uint32_t m_mainCameraId = 0;
};
