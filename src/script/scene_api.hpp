#pragma once

#include "src/scene/scene.hpp"

#include <lua.hpp>

void registerSceneApi(lua_State *lua, Scene &scene);
void pushGameObject(lua_State *lua, Scene *scene, uint32_t id);
