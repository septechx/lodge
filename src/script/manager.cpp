#include "manager.hpp"

#include "src/script/scene_api.hpp"

#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <string>

static int l_logInfo(lua_State *lua) {
  spdlog::info("{}", luaL_checkstring(lua, 1));
  return 0;
}

static int l_logWarn(lua_State *lua) {
  spdlog::warn("{}", luaL_checkstring(lua, 1));
  return 0;
}

int l_logError(lua_State *lua) {
  spdlog::error("{}", luaL_checkstring(lua, 1));
  return 0;
}

static void registerLogApi(lua_State *lua) {
  lua_newtable(lua);
  lua_pushcfunction(lua, l_logInfo);
  lua_setfield(lua, -2, "info");
  lua_pushcfunction(lua, l_logWarn);
  lua_setfield(lua, -2, "warn");
  lua_pushcfunction(lua, l_logError);
  lua_setfield(lua, -2, "error");
  lua_setglobal(lua, "log");
}

static void pushSandboxEnv(lua_State *lua) {
  lua_newtable(lua);
  lua_newtable(lua);
  lua_pushvalue(lua, LUA_GLOBALSINDEX);
  lua_setfield(lua, -2, "__index");
  lua_setmetatable(lua, -2);
}

static int refEnvField(lua_State *lua, int envRef, const char *name) {
  lua_rawgeti(lua, LUA_REGISTRYINDEX, envRef);
  lua_getfield(lua, -1, name);
  int ref = LUA_NOREF;
  if (lua_isfunction(lua, -1)) {
    ref = luaL_ref(lua, LUA_REGISTRYINDEX);
  } else {
    lua_pop(lua, 1);
  }
  lua_pop(lua, 1);
  return ref;
}

ScriptManager::ScriptManager(Scene &scene) : m_scene(scene) {
  m_lua = luaL_newstate();
  luaL_openlibs(m_lua);

  registerSceneApi(m_lua, m_scene);
  registerLogApi(m_lua);
}

ScriptManager::~ScriptManager() {
  for (Script &script : m_scripts) {
    if (script.startRef != LUA_NOREF) {
      luaL_unref(m_lua, LUA_REGISTRYINDEX, script.startRef);
    }
    if (script.updateRef != LUA_NOREF) {
      luaL_unref(m_lua, LUA_REGISTRYINDEX, script.updateRef);
    }
    if (script.envRef != LUA_NOREF) {
      luaL_unref(m_lua, LUA_REGISTRYINDEX, script.envRef);
    }
  }

  lua_close(m_lua);
}

void ScriptManager::loadScript(std::filesystem::path path) {
  if (luaL_loadfile(m_lua, path.c_str()) != LUA_OK) {
    throwError(path);
    return;
  }

  pushSandboxEnv(m_lua);
  lua_pushvalue(m_lua, -1);
  int envRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);
  lua_setfenv(m_lua, -2);

  if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
    luaL_unref(m_lua, LUA_REGISTRYINDEX, envRef);
    throwError(path);
    return;
  }

  int startRef = refEnvField(m_lua, envRef, "Start");
  int updateRef = refEnvField(m_lua, envRef, "Update");
  m_scripts.push_back({path, envRef, startRef, updateRef});

  if (startRef != LUA_NOREF) {
    lua_rawgeti(m_lua, LUA_REGISTRYINDEX, startRef);
    if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
      throwError(path);
    }
  }
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

void ScriptManager::throwError(const std::filesystem::path &path) {
  std::string error = lua_tostring(m_lua, -1);
  lua_pop(m_lua, 1);
  spdlog::error("Lua error in {}: {}", path.string(), error);
}
