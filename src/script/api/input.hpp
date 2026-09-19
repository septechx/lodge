#pragma once

#include "src/script/api/input_state.hpp"

#include <lua.hpp>

void registerInputApi(lua_State *lua, const InputState *state);
