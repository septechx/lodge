#pragma once

#include "src/script/api/input_state.hpp"

#include <lua.hpp>

struct GLFWwindow;

void registerInputApi(lua_State *lua, const InputState *state,
                      GLFWwindow *window = nullptr);
