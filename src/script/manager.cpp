#include "manager.hpp"

#include "src/script/api/input.hpp"
#include "src/script/api/log.hpp"
#include "src/script/api/scene.hpp"
#include "src/script/api/time.hpp"

#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <string>

namespace {

void pushSandboxEnv(lua_State *lua) {
  lua_newtable(lua);
  lua_newtable(lua);
  lua_pushvalue(lua, LUA_GLOBALSINDEX);
  lua_setfield(lua, -2, "__index");
  lua_setmetatable(lua, -2);
}

int refEnvField(lua_State *lua, int envRef, const char *name) {
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

} // namespace

ScriptManager::ScriptManager(Scene &scene) : m_scene(scene) {
  m_lua = luaL_newstate();
  luaL_openlibs(m_lua);

  registerSceneApi(m_lua, m_scene);
  registerInputApi(m_lua, &m_input);
  registerTimeApi(m_lua, &m_elapsed);
  registerLogApi(m_lua);
}

ScriptManager::~ScriptManager() {
  clear();

  lua_close(m_lua);
}

void ScriptManager::loadScript(std::filesystem::path path) {
  loadScriptWithOwner(std::move(path), 0);
}

void ScriptManager::loadObjectScript(uint32_t ownerId,
                                     std::filesystem::path path) {
  loadScriptWithOwner(std::move(path), ownerId);
}

void ScriptManager::loadSceneScripts() {
  clear();
  for (GameObject &object : m_scene.objects()) {
    for (const ScriptRef &ref : object.scripts) {
      loadScriptWithOwner(ref.path, object.id);
    }
  }
}

void ScriptManager::clear() {
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
  m_scripts.clear();
}

void ScriptManager::loadScriptWithOwner(std::filesystem::path path,
                                        uint32_t ownerId) {
  if (ownerId != 0 && m_scene.find(ownerId) == nullptr) {
    spdlog::error("Cannot attach script {}: object id {} not found",
                  path.string(), ownerId);
    return;
  }

  if (luaL_loadfile(m_lua, path.c_str()) != LUA_OK) {
    throwError(path, ownerId);
    return;
  }

  pushSandboxEnv(m_lua);
  lua_pushvalue(m_lua, -1);
  int envRef = luaL_ref(m_lua, LUA_REGISTRYINDEX);
  lua_setfenv(m_lua, -2);

  lua_rawgeti(m_lua, LUA_REGISTRYINDEX, envRef);
  if (ownerId != 0) {
    pushGameObject(m_lua, &m_scene, ownerId);
  } else {
    lua_pushnil(m_lua);
  }
  lua_setfield(m_lua, -2, "self");
  if (ownerId != 0) {
    pushGameObject(m_lua, &m_scene, ownerId);
  } else {
    lua_pushnil(m_lua);
  }
  lua_setfield(m_lua, -2, "gameObject");
  lua_pop(m_lua, 1);

  if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
    luaL_unref(m_lua, LUA_REGISTRYINDEX, envRef);
    throwError(path, ownerId);
    return;
  }

  int startRef = refEnvField(m_lua, envRef, "Start");
  int updateRef = refEnvField(m_lua, envRef, "Update");
  m_scripts.push_back({path, envRef, startRef, updateRef, ownerId});

  if (startRef != LUA_NOREF) {
    lua_rawgeti(m_lua, LUA_REGISTRYINDEX, startRef);
    if (lua_pcall(m_lua, 0, 0, 0) != LUA_OK) {
      throwError(path, ownerId);
    }
  }
}

void ScriptManager::onUpdate(float dt) {
  m_elapsed += dt;
  for (Script &script : m_scripts) {
    if (script.updateRef == LUA_NOREF) {
      continue;
    }
    if (script.ownerId != 0 && m_scene.find(script.ownerId) == nullptr) {
      continue;
    }
    lua_rawgeti(m_lua, LUA_REGISTRYINDEX, script.updateRef);
    lua_pushnumber(m_lua, dt);
    if (lua_pcall(m_lua, 1, 0, 0) != LUA_OK) {
      throwError(script.path, script.ownerId);
    }
  }
  m_input.endFrame();
}

void ScriptManager::onEvent(const Event &event) { m_input.onEvent(event); }

void ScriptManager::throwError(const std::filesystem::path &path,
                               uint32_t ownerId) {
  std::string error = lua_tostring(m_lua, -1);
  lua_pop(m_lua, 1);
  if (ownerId != 0) {
    if (const GameObject *object = m_scene.find(ownerId)) {
      spdlog::error("Lua error in {} ({}): {}", path.string(), object->name,
                    error);
      return;
    }
    spdlog::error("Lua error in {} (id {}): {}", path.string(), ownerId, error);
    return;
  }
  spdlog::error("Lua error in {}: {}", path.string(), error);
}
