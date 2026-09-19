#include "log.hpp"

#include <spdlog/spdlog.h>

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

void registerLogApi(lua_State *lua) {
  lua_newtable(lua);
  lua_pushcfunction(lua, l_logInfo);
  lua_setfield(lua, -2, "info");
  lua_pushcfunction(lua, l_logWarn);
  lua_setfield(lua, -2, "warn");
  lua_pushcfunction(lua, l_logError);
  lua_setfield(lua, -2, "error");
  lua_setglobal(lua, "log");
}
