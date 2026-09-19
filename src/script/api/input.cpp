#include "input.hpp"

#include "src/script/api/input_state.hpp"

#include <GLFW/glfw3.h>

#include <string_view>

namespace {

const InputState *getInput(lua_State *lua) {
  return static_cast<const InputState *>(
      lua_touserdata(lua, lua_upvalueindex(1)));
}

GLFWwindow *getWindow(lua_State *lua) {
  if (lua_isnil(lua, lua_upvalueindex(2)))
    return nullptr;
  return static_cast<GLFWwindow *>(lua_touserdata(lua, lua_upvalueindex(2)));
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

int cursorModeFromString(lua_State *lua, int idx) {
  size_t len = 0;
  const char *name = luaL_checklstring(lua, idx, &len);
  std::string_view mode(name, len);
  if (mode == "normal")
    return GLFW_CURSOR_NORMAL;
  if (mode == "hidden")
    return GLFW_CURSOR_HIDDEN;
  if (mode == "disabled")
    return GLFW_CURSOR_DISABLED;
  return luaL_error(
      lua, "unknown cursor mode '%s' (expected normal|hidden|disabled)", name);
}

const char *cursorModeToString(int mode) {
  switch (mode) {
  case GLFW_CURSOR_HIDDEN:
    return "hidden";
  case GLFW_CURSOR_DISABLED:
    return "disabled";
  default:
    return "normal";
  }
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

int l_setCursorMode(lua_State *lua) {
  int mode = cursorModeFromString(lua, 1);
  GLFWwindow *window = getWindow(lua);
  if (window != nullptr) {
    glfwSetInputMode(window, GLFW_CURSOR, mode);
    if (mode == GLFW_CURSOR_DISABLED) {
      if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
      glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
  }
  return 0;
}

int l_cursorMode(lua_State *lua) {
  GLFWwindow *window = getWindow(lua);
  if (window == nullptr) {
    lua_pushstring(lua, "normal");
    return 1;
  }
  lua_pushstring(lua,
                 cursorModeToString(glfwGetInputMode(window, GLFW_CURSOR)));
  return 1;
}

} // namespace

void pushInputClosure(lua_State *lua, const InputState *state,
                      GLFWwindow *window, lua_CFunction fn) {
  lua_pushlightuserdata(lua, const_cast<InputState *>(state));
  if (window != nullptr)
    lua_pushlightuserdata(lua, window);
  else
    lua_pushnil(lua);
  lua_pushcclosure(lua, fn, 2);
}

void registerInputApi(lua_State *lua, const InputState *state,
                      GLFWwindow *window) {
  lua_newtable(lua);
  pushInputClosure(lua, state, window, l_isKeyDown);
  lua_setfield(lua, -2, "isKeyDown");
  pushInputClosure(lua, state, window, l_isMouseDown);
  lua_setfield(lua, -2, "isMouseDown");
  pushInputClosure(lua, state, window, l_mousePos);
  lua_setfield(lua, -2, "mousePos");
  pushInputClosure(lua, state, window, l_mouseDelta);
  lua_setfield(lua, -2, "mouseDelta");
  pushInputClosure(lua, state, window, l_scroll);
  lua_setfield(lua, -2, "scroll");
  pushInputClosure(lua, state, window, l_setCursorMode);
  lua_setfield(lua, -2, "setCursorMode");
  pushInputClosure(lua, state, window, l_cursorMode);
  lua_setfield(lua, -2, "cursorMode");
  lua_setglobal(lua, "input");
}
