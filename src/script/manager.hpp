#pragma once

#include "src/core/event.hpp"
#include "src/scene/scene.hpp"
#include "src/script/api/input_state.hpp"

#include <lua.hpp>

#include <filesystem>
#include <vector>

struct GLFWwindow;

struct Script {
  std::filesystem::path path;
  int envRef = LUA_NOREF;
  int startRef = LUA_NOREF;
  int updateRef = LUA_NOREF;
  uint32_t ownerId = 0;
};

class ScriptManager {
public:
  ScriptManager(Scene &scene, GLFWwindow *window = nullptr);
  ~ScriptManager();

  ScriptManager(const ScriptManager &) = delete;
  ScriptManager &operator=(const ScriptManager &) = delete;

  void loadScript(std::filesystem::path path);
  void loadObjectScript(uint32_t ownerId, std::filesystem::path path);
  void loadSceneScripts();
  void clear();

  size_t scriptCount() const { return m_scripts.size(); }

  void onEvent(const Event &event);
  void onUpdate(float dt);

  InputState &inputState() { return m_input; }
  const InputState &inputState() const { return m_input; }
  double elapsed() const { return m_elapsed; }

private:
  lua_State *m_lua;
  std::vector<Script> m_scripts;
  Scene &m_scene;
  InputState m_input;
  GLFWwindow *m_window = nullptr;
  double m_elapsed = 0.0;

  void loadScriptWithOwner(std::filesystem::path path, uint32_t ownerId);
  void throwError(const std::filesystem::path &path, uint32_t ownerId = 0);
};
