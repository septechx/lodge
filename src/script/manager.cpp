#include "manager.hpp"

#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <string>

void ScriptManager::throwError(const std::filesystem::path &path) {
  std::string error = lua_tostring(m_lua, -1);
  lua_pop(m_lua, 1);
  spdlog::error("Lua error in {}: {}", path.string(), error);
}

ScriptManager::ScriptManager() {
  m_lua = luaL_newstate();
  luaL_openlibs(m_lua);
}

ScriptManager::~ScriptManager() {
  for (Script &script : m_scripts) {
    if (script.updateRef != LUA_NOREF) {
      luaL_unref(m_lua, LUA_REGISTRYINDEX, script.updateRef);
    }
  }

  lua_close(m_lua);
}

void ScriptManager::loadScript(std::filesystem::path path) {
  if (luaL_dofile(m_lua, path.c_str()) != LUA_OK) {
    throwError(path);
  }

  lua_getglobal(m_lua, "Update");

  if (!lua_isfunction(m_lua, -1)) {
    lua_pop(m_lua, 1);
    spdlog::error("Script {} does not define update()");
  }

  int updateRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);

  m_scripts.push_back({path, updateRef});
}

void ScriptManager::updateScripts(float dt) {
  for (Script &script : m_scripts) {
    if (script.updateRef != LUA_NOREF) {
      lua_rawgeti(m_lua, LUA_REGISTRYINDEX, script.updateRef);
      lua_pushnumber(m_lua, dt);
      if (lua_pcall(m_lua, 1, 0, 0) != LUA_OK) {
        throwError(script.path);
      }
    }
  }
}
