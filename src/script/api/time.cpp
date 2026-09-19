#include "time.hpp"

namespace {

const double *getElapsed(lua_State *lua) {
  return static_cast<const double *>(lua_touserdata(lua, lua_upvalueindex(1)));
}

int l_now(lua_State *lua) {
  lua_pushnumber(lua, *getElapsed(lua));
  return 1;
}

} // namespace

void registerTimeApi(lua_State *lua, const double *elapsed) {
  lua_newtable(lua);
  lua_pushlightuserdata(lua, const_cast<double *>(elapsed));
  lua_pushcclosure(lua, l_now, 1);
  lua_setfield(lua, -2, "now");
  lua_setglobal(lua, "time");
}
