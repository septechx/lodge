#pragma once

#include <filesystem>
#include <lua.hpp>

#include <vector>

struct Script {
  std::filesystem::path path;
  int updateRef = LUA_NOREF;
};

class ScriptManager {
public:
  ScriptManager();
  ~ScriptManager();

  void loadScript(std::filesystem::path path);
  void updateScripts(float dt);

private:
  lua_State *m_lua;
  std::vector<Script> m_scripts;

  void throwError(const std::filesystem::path &path);
};
