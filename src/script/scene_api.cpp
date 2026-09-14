#include "scene_api.hpp"

#include "src/scene/scene.hpp"

#include <lua.hpp>

static Scene *getScene(lua_State *lua) {
  return static_cast<Scene *>(lua_touserdata(lua, lua_upvalueindex(1)));
}

static uint32_t optId(lua_State *lua, int idx) {
  if (lua_isnoneornil(lua, idx)) {
    return 0;
  }
  return static_cast<uint32_t>(luaL_checkinteger(lua, idx));
}

static GameObject *findOpt(Scene *scene, lua_State *lua, int idx) {
  return scene->find(optId(lua, idx));
}

static GameObject *checkObject(lua_State *lua, Scene *scene, int idx) {
  GameObject *object = scene->find(optId(lua, idx));
  if (object == nullptr) {
    luaL_error(lua, "invalid object id");
  }
  return object;
}

static float checkField(lua_State *lua, int idx, const char *key) {
  lua_getfield(lua, idx, key);
  float value = static_cast<float>(luaL_checknumber(lua, -1));
  lua_pop(lua, 1);
  return value;
}

static void setField(lua_State *lua, const char *key, float value) {
  lua_pushnumber(lua, value);
  lua_setfield(lua, -2, key);
}

static Vec3 checkVec3(lua_State *lua, int idx) {
  luaL_checktype(lua, idx, LUA_TTABLE);
  float x = checkField(lua, idx, "x");
  float y = checkField(lua, idx, "y");
  float z = checkField(lua, idx, "z");
  return {x, y, z};
}

static void pushVec3(lua_State *lua, Vec3 v) {
  lua_newtable(lua);
  setField(lua, "x", v.x);
  setField(lua, "y", v.y);
  setField(lua, "z", v.z);
}

static Quat checkQuat(lua_State *lua, int idx) {
  luaL_checktype(lua, idx, LUA_TTABLE);
  float w = checkField(lua, idx, "w");
  float x = checkField(lua, idx, "x");
  float y = checkField(lua, idx, "y");
  float z = checkField(lua, idx, "z");
  return {w, x, y, z};
}

static void pushQuat(lua_State *lua, Quat q) {
  lua_newtable(lua);
  setField(lua, "w", q.w);
  setField(lua, "x", q.x);
  setField(lua, "y", q.y);
  setField(lua, "z", q.z);
}

static Quat normalizeOrError(lua_State *lua, Quat q) {
  if (q.length() < 1e-6f) {
    luaL_error(lua, "degenerate rotation");
  }
  return q.normalize();
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

static int l_isValid(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, findOpt(scene, lua, 1) != nullptr);
  return 1;
}

static int l_getName(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    lua_pushstring(lua, object->name.c_str());
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_findAll(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_newtable(lua);
  int i = 1;
  for (const GameObject &object : scene->objects()) {
    lua_pushinteger(lua, object.id);
    lua_rawseti(lua, -2, i++);
  }
  return 1;
}

static int l_mainCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *camera = scene->mainCamera()) {
    lua_pushinteger(lua, camera->id);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_setMainCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  if (!object->camera.has_value()) {
    return luaL_error(lua, "object has no camera component");
  }
  scene->setMainCamera(object->id);
  return 0;
}

static int l_getPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushVec3(lua, object->transform.position);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_setPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.position = checkVec3(lua, 2);
  return 0;
}

static int l_getRotation(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushQuat(lua, object->transform.rotation);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_setRotation(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.rotation = normalizeOrError(lua, checkQuat(lua, 2));
  return 0;
}

static int l_setRotationEuler(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  Vec3 euler = checkVec3(lua, 2);
  object->transform.rotation = Quat::fromEuler(euler).normalize();
  return 0;
}

static int l_getScale(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushVec3(lua, object->transform.scale);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

static int l_setScale(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.scale = checkVec3(lua, 2);
  return 0;
}

static int l_translate(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  Vec3 delta = checkVec3(lua, 2);
  object->transform.position = object->transform.position + delta;
  return 0;
}

static int l_rotateAxisAngle(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  Vec3 axis = checkVec3(lua, 2);
  float angle = static_cast<float>(luaL_checknumber(lua, 3));
  if (axis.length() < 1e-6f) {
    return luaL_error(lua, "degenerate rotation axis");
  }
  Quat delta = Quat::fromAxisAngle(axis, angle);
  object->transform.rotation = (delta * object->transform.rotation).normalize();
  return 0;
}

static int l_hasRenderer(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->renderer.has_value());
  return 1;
}

static int l_hasCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->camera.has_value());
  return 1;
}

static int l_hasLight(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->light.has_value());
  return 1;
}

static int l_getCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = findOpt(scene, lua, 1);
  if (object == nullptr || !object->camera.has_value()) {
    lua_pushnil(lua);
    return 1;
  }
  lua_newtable(lua);
  setField(lua, "fovY", object->camera->fovY);
  setField(lua, "nearZ", object->camera->nearZ);
  setField(lua, "farZ", object->camera->farZ);
  return 1;
}

static int l_setCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  if (!object->camera.has_value()) {
    return luaL_error(lua, "object has no camera component");
  }
  luaL_checktype(lua, 2, LUA_TTABLE);
  float fovY = checkField(lua, 2, "fovY");
  float nearZ = checkField(lua, 2, "nearZ");
  float farZ = checkField(lua, 2, "farZ");
  if (fovY <= 0.0f) {
    return luaL_error(lua, "fovY must be positive");
  }
  if (nearZ <= 0.0f) {
    return luaL_error(lua, "nearZ must be positive");
  }
  if (farZ <= nearZ) {
    return luaL_error(lua, "farZ must be greater than nearZ");
  }
  object->camera->fovY = fovY;
  object->camera->nearZ = nearZ;
  object->camera->farZ = farZ;
  return 0;
}

static int l_getLightColor(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = findOpt(scene, lua, 1);
  if (object == nullptr || !object->light.has_value()) {
    lua_pushnil(lua);
    return 1;
  }
  lua_newtable(lua);
  setField(lua, "r", object->light->color.x);
  setField(lua, "g", object->light->color.y);
  setField(lua, "b", object->light->color.z);
  return 1;
}

static int l_setLightColor(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  if (!object->light.has_value()) {
    return luaL_error(lua, "object has no light component");
  }
  luaL_checktype(lua, 2, LUA_TTABLE);
  float r = checkField(lua, 2, "r");
  float g = checkField(lua, 2, "g");
  float b = checkField(lua, 2, "b");
  object->light->color = {r, g, b};
  return 0;
}

struct Binding {
  const char *name;
  lua_CFunction fn;
};

const Binding BINDINGS[] = {
    {"findByName", l_findByName},
    {"isValid", l_isValid},
    {"getName", l_getName},
    {"findAll", l_findAll},
    {"mainCamera", l_mainCamera},
    {"setMainCamera", l_setMainCamera},
    {"getPosition", l_getPosition},
    {"setPosition", l_setPosition},
    {"getRotation", l_getRotation},
    {"setRotation", l_setRotation},
    {"setRotationEuler", l_setRotationEuler},
    {"getScale", l_getScale},
    {"setScale", l_setScale},
    {"translate", l_translate},
    {"rotateAxisAngle", l_rotateAxisAngle},
    {"hasRenderer", l_hasRenderer},
    {"hasCamera", l_hasCamera},
    {"hasLight", l_hasLight},
    {"getCamera", l_getCamera},
    {"setCamera", l_setCamera},
    {"getLightColor", l_getLightColor},
    {"setLightColor", l_setLightColor},
};

void registerSceneApi(lua_State *lua, Scene &scene) {
  lua_newtable(lua);
  for (const Binding &binding : BINDINGS) {
    lua_pushlightuserdata(lua, &scene);
    lua_pushcclosure(lua, binding.fn, 1);
    lua_setfield(lua, -2, binding.name);
  }
  lua_setglobal(lua, "scene");
}
