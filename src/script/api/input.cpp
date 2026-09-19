#include "input.hpp"

#include "src/script/api/input_state.hpp"

#include <GLFW/glfw3.h>

namespace {

const InputState *getInput(lua_State *lua) {
  return static_cast<const InputState *>(
      lua_touserdata(lua, lua_upvalueindex(1)));
}

int resolveKey(lua_State *lua, int idx) {
  if (lua_isnumber(lua, idx))
    return static_cast<int>(lua_tointeger(lua, idx));
  size_t len = 0;
  const char *name = luaL_checklstring(lua, idx, &len);
  std::optional<int> key =
      InputState::keyFromString(std::string_view(name, len));
  if (!key.has_value())
    return luaL_error(lua, "unknown key '%s'", name);
  return *key;
}

int resolveButton(lua_State *lua, int idx) {
  if (lua_isnumber(lua, idx)) {
    int button = static_cast<int>(lua_tointeger(lua, idx));
    if (button < 0 || button > GLFW_MOUSE_BUTTON_8)
      return luaL_error(lua, "mouse button out of range (0-7)");
    return button;
  }
  size_t len = 0;
  const char *name = luaL_checklstring(lua, idx, &len);
  std::optional<int> button =
      InputState::buttonFromString(std::string_view(name, len));
  if (!button.has_value())
    return luaL_error(lua, "unknown mouse button '%s'", name);
  return *button;
}

int l_isKeyDown(lua_State *lua) {
  int key = resolveKey(lua, 1);
  lua_pushboolean(lua, getInput(lua)->isKeyDown(key));
  return 1;
}

int l_isMouseDown(lua_State *lua) {
  int button = resolveButton(lua, 1);
  lua_pushboolean(lua, getInput(lua)->isMouseDown(button));
  return 1;
}

int l_mousePos(lua_State *lua) {
  const InputState *input = getInput(lua);
  lua_newtable(lua);
  lua_pushnumber(lua, input->mouseX());
  lua_setfield(lua, -2, "x");
  lua_pushnumber(lua, input->mouseY());
  lua_setfield(lua, -2, "y");
  return 1;
}

int l_mouseDelta(lua_State *lua) {
  const InputState *input = getInput(lua);
  lua_newtable(lua);
  lua_pushnumber(lua, input->deltaX());
  lua_setfield(lua, -2, "x");
  lua_pushnumber(lua, input->deltaY());
  lua_setfield(lua, -2, "y");
  return 1;
}

int l_scroll(lua_State *lua) {
  const InputState *input = getInput(lua);
  lua_newtable(lua);
  lua_pushnumber(lua, input->scrollX());
  lua_setfield(lua, -2, "x");
  lua_pushnumber(lua, input->scrollY());
  lua_setfield(lua, -2, "y");
  return 1;
}

} // namespace

void registerInputApi(lua_State *lua, const InputState *state) {
  lua_newtable(lua);
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  lua_pushcclosure(lua, l_isKeyDown, 1);
  lua_setfield(lua, -2, "isKeyDown");
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  lua_pushcclosure(lua, l_isMouseDown, 1);
  lua_setfield(lua, -2, "isMouseDown");
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  lua_pushcclosure(lua, l_mousePos, 1);
  lua_setfield(lua, -2, "mousePos");
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  lua_pushcclosure(lua, l_mouseDelta, 1);
  lua_setfield(lua, -2, "mouseDelta");
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  lua_pushcclosure(lua, l_scroll, 1);
  lua_setfield(lua, -2, "scroll");
  lua_setglobal(lua, "input");
}
