#pragma once

#include "src/scene/scene.hpp"

#include <lua.hpp>

#include <filesystem>
#include <vector>

struct Script {
  std::filesystem::path path;
  int envRef = LUA_NOREF;
  int startRef = LUA_NOREF;
  int updateRef = LUA_NOREF;
};

class ScriptManager {
public:
  ScriptManager(Scene &scene);
  ~ScriptManager();

  void loadScript(std::filesystem::path path);
  void updateScripts(float dt);

private:
  lua_State *m_lua;
  std::vector<Script> m_scripts;
  Scene &m_scene;

  void throwError(const std::filesystem::path &path);
};
