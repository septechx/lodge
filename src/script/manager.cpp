#include "manager.hpp"

#include <lua.hpp>
#include <spdlog/spdlog.h>

#include <string>

static Scene *getScene(lua_State *lua) {
  return static_cast<Scene *>(lua_touserdata(lua, lua_upvalueindex(1)));
}

static int l_findByName(lua_State *lua) {
  Scene *scene = getScene(lua);
  const char *name = luaL_checkstring(lua, 1);
  if (GameObject *object = scene->findByName(name)) {
    lua_pushinteger(lua, object->id);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_getPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = scene->find(luaL_checkinteger(lua, 1));
  if (!object) {
    lua_pushnil(lua);
    return 1;
  }
  lua_pushnumber(lua, object->transform.position.x);
  lua_pushnumber(lua, object->transform.position.y);
  lua_pushnumber(lua, object->transform.position.z);
  return 3;
}

static int l_setPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object =
      scene->find(static_cast<uint32_t>(luaL_checkinteger(lua, 1)));
  if (!object) {
    return luaL_error(lua, "invalid object id");
  }
  object->transform.position = {static_cast<float>(luaL_checknumber(lua, 2)),
                                static_cast<float>(luaL_checknumber(lua, 3)),
                                static_cast<float>(luaL_checknumber(lua, 4))};
  return 0;
}

ScriptManager::ScriptManager(Scene &scene) : m_scene(scene) {
  m_lua = luaL_newstate();
  luaL_openlibs(m_lua);

  lua_newtable(m_lua);

  lua_pushlightuserdata(m_lua, &m_scene);
  lua_pushcclosure(m_lua, l_findByName, 1);
  lua_setfield(m_lua, -2, "findByName");

  lua_pushlightuserdata(m_lua, &m_scene);
  lua_pushcclosure(m_lua, l_getPosition, 1);
  lua_setfield(m_lua, -2, "getPosition");

  lua_pushlightuserdata(m_lua, &m_scene);
  lua_pushcclosure(m_lua, l_setPosition, 1);
  lua_setfield(m_lua, -2, "setPosition");

  lua_setglobal(m_lua, "scene");
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

void ScriptManager::throwError(const std::filesystem::path &path) {
  std::string error = lua_tostring(m_lua, -1);
  lua_pop(m_lua, 1);
  spdlog::error("Lua error in {}: {}", path.string(), error);
}
