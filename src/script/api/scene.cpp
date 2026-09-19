#include "scene.hpp"

#include "src/scene/scene.hpp"

#include <lua.hpp>

#include <cstring>
#include <string>

constexpr const char *GAMEOBJECT_MT = "Lodge.GameObject";

struct ObjectHandle {
  Scene *scene = nullptr;
  uint32_t id = 0;
};

void pushGameObject(lua_State *lua, Scene *scene, uint32_t id) {
  ObjectHandle *handle =
      static_cast<ObjectHandle *>(lua_newuserdata(lua, sizeof(ObjectHandle)));
  handle->scene = scene;
  handle->id = id;
  luaL_getmetatable(lua, GAMEOBJECT_MT);
  lua_setmetatable(lua, -2);
}

namespace {

Scene *getScene(lua_State *lua) {
  return static_cast<Scene *>(lua_touserdata(lua, lua_upvalueindex(1)));
}

bool isGameObject(lua_State *lua, int idx) {
  if (lua_type(lua, idx) != LUA_TUSERDATA) {
    return false;
  }
  int absIdx = idx;
  if (absIdx < 0) {
    absIdx = lua_gettop(lua) + absIdx + 1;
  }
  if (lua_getmetatable(lua, absIdx) == 0) {
    return false;
  }
  lua_getfield(lua, LUA_REGISTRYINDEX, GAMEOBJECT_MT);
  bool eq = lua_rawequal(lua, -1, -2) != 0;
  lua_pop(lua, 2);
  return eq;
}

ObjectHandle *checkHandle(lua_State *lua, int idx) {
  return static_cast<ObjectHandle *>(luaL_checkudata(lua, idx, GAMEOBJECT_MT));
}

uint32_t handleId(lua_State *lua, int idx) {
  return static_cast<ObjectHandle *>(lua_touserdata(lua, idx))->id;
}

uint32_t optId(lua_State *lua, int idx) {
  if (lua_isnoneornil(lua, idx)) {
    return 0;
  }
  if (isGameObject(lua, idx)) {
    return handleId(lua, idx);
  }
  return static_cast<uint32_t>(luaL_checkinteger(lua, idx));
}

GameObject *findOpt(Scene *scene, lua_State *lua, int idx) {
  return scene->find(optId(lua, idx));
}

GameObject *checkObject(lua_State *lua, Scene *scene, int idx) {
  GameObject *object = scene->find(optId(lua, idx));
  if (object == nullptr) {
    luaL_error(lua, "invalid object id");
  }
  return object;
}

GameObject *findHandleOpt(ObjectHandle *handle) {
  if (handle == nullptr || handle->scene == nullptr) {
    return nullptr;
  }
  return handle->scene->find(handle->id);
}

GameObject *checkHandleObject(lua_State *lua, int idx) {
  ObjectHandle *handle = checkHandle(lua, idx);
  GameObject *object = findHandleOpt(handle);
  if (object == nullptr) {
    luaL_error(lua, "invalid object id");
  }
  return object;
}

float checkField(lua_State *lua, int idx, const char *key) {
  lua_getfield(lua, idx, key);
  float value = static_cast<float>(luaL_checknumber(lua, -1));
  lua_pop(lua, 1);
  return value;
}

void setField(lua_State *lua, const char *key, float value) {
  lua_pushnumber(lua, value);
  lua_setfield(lua, -2, key);
}

Vec3 checkVec3(lua_State *lua, int idx) {
  luaL_checktype(lua, idx, LUA_TTABLE);
  float x = checkField(lua, idx, "x");
  float y = checkField(lua, idx, "y");
  float z = checkField(lua, idx, "z");
  return {x, y, z};
}

void pushVec3(lua_State *lua, Vec3 v) {
  lua_newtable(lua);
  setField(lua, "x", v.x);
  setField(lua, "y", v.y);
  setField(lua, "z", v.z);
}

Quat checkQuat(lua_State *lua, int idx) {
  luaL_checktype(lua, idx, LUA_TTABLE);
  float w = checkField(lua, idx, "w");
  float x = checkField(lua, idx, "x");
  float y = checkField(lua, idx, "y");
  float z = checkField(lua, idx, "z");
  return {w, x, y, z};
}

void pushQuat(lua_State *lua, Quat q) {
  lua_newtable(lua);
  setField(lua, "w", q.w);
  setField(lua, "x", q.x);
  setField(lua, "y", q.y);
  setField(lua, "z", q.z);
}

Quat normalizeOrError(lua_State *lua, Quat q) {
  if (q.length() < 1e-6f) {
    luaL_error(lua, "degenerate rotation");
  }
  return q.normalize();
}

int l_findByName(lua_State *lua) {
  Scene *scene = getScene(lua);
  const char *name = luaL_checkstring(lua, 1);
  if (GameObject *object = scene->findByName(name)) {
    pushGameObject(lua, scene, object->id);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_isValid(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, findOpt(scene, lua, 1) != nullptr);
  return 1;
}

int l_getName(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    lua_pushstring(lua, object->name.c_str());
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_findAll(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_newtable(lua);
  int i = 1;
  for (const GameObject &object : scene->objects()) {
    pushGameObject(lua, scene, object.id);
    lua_rawseti(lua, -2, i++);
  }
  return 1;
}

int l_mainCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *camera = scene->mainCamera()) {
    pushGameObject(lua, scene, camera->id);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_setMainCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  if (!object->camera.has_value()) {
    return luaL_error(lua, "object has no camera component");
  }
  scene->setMainCamera(object->id);
  return 0;
}

int l_getPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushVec3(lua, object->transform.position);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_setPosition(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.position = checkVec3(lua, 2);
  return 0;
}

int l_getRotation(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushQuat(lua, object->transform.rotation);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_setRotation(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.rotation = normalizeOrError(lua, checkQuat(lua, 2));
  return 0;
}

int l_setRotationEuler(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  Vec3 euler = checkVec3(lua, 2);
  object->transform.rotation = Quat::fromEuler(euler).normalize();
  return 0;
}

int l_getScale(lua_State *lua) {
  Scene *scene = getScene(lua);
  if (GameObject *object = findOpt(scene, lua, 1)) {
    pushVec3(lua, object->transform.scale);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_setScale(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  object->transform.scale = checkVec3(lua, 2);
  return 0;
}

int l_translate(lua_State *lua) {
  Scene *scene = getScene(lua);
  GameObject *object = checkObject(lua, scene, 1);
  Vec3 delta = checkVec3(lua, 2);
  object->transform.position = object->transform.position + delta;
  return 0;
}

int l_rotateAxisAngle(lua_State *lua) {
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

int l_hasRenderer(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->renderer.has_value());
  return 1;
}

int l_hasCamera(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->camera.has_value());
  return 1;
}

int l_hasLight(lua_State *lua) {
  Scene *scene = getScene(lua);
  lua_pushboolean(lua, checkObject(lua, scene, 1)->light.has_value());
  return 1;
}

int l_getCamera(lua_State *lua) {
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

int l_setCamera(lua_State *lua) {
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

int l_getLightColor(lua_State *lua) {
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

int l_setLightColor(lua_State *lua) {
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

int l_objGetId(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  lua_pushinteger(lua, handle->id);
  return 1;
}

int l_objIsValid(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  lua_pushboolean(lua, findHandleOpt(handle) != nullptr);
  return 1;
}

int l_objGetName(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  if (GameObject *object = findHandleOpt(handle)) {
    lua_pushstring(lua, object->name.c_str());
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_objGetPosition(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  if (GameObject *object = findHandleOpt(handle)) {
    pushVec3(lua, object->transform.position);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_objSetPosition(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  object->transform.position = checkVec3(lua, 2);
  return 0;
}

int l_objGetRotation(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  if (GameObject *object = findHandleOpt(handle)) {
    pushQuat(lua, object->transform.rotation);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_objSetRotation(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  object->transform.rotation = normalizeOrError(lua, checkQuat(lua, 2));
  return 0;
}

int l_objSetRotationEuler(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  Vec3 euler = checkVec3(lua, 2);
  object->transform.rotation = Quat::fromEuler(euler).normalize();
  return 0;
}

int l_objGetScale(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  if (GameObject *object = findHandleOpt(handle)) {
    pushVec3(lua, object->transform.scale);
  } else {
    lua_pushnil(lua);
  }
  return 1;
}

int l_objSetScale(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  object->transform.scale = checkVec3(lua, 2);
  return 0;
}

int l_objTranslate(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  Vec3 delta = checkVec3(lua, 2);
  object->transform.position = object->transform.position + delta;
  return 0;
}

int l_objRotateAxisAngle(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
  Vec3 axis = checkVec3(lua, 2);
  float angle = static_cast<float>(luaL_checknumber(lua, 3));
  if (axis.length() < 1e-6f) {
    return luaL_error(lua, "degenerate rotation axis");
  }
  Quat delta = Quat::fromAxisAngle(axis, angle);
  object->transform.rotation = (delta * object->transform.rotation).normalize();
  return 0;
}

int l_objHasRenderer(lua_State *lua) {
  lua_pushboolean(lua, checkHandleObject(lua, 1)->renderer.has_value());
  return 1;
}

int l_objHasCamera(lua_State *lua) {
  lua_pushboolean(lua, checkHandleObject(lua, 1)->camera.has_value());
  return 1;
}

int l_objHasLight(lua_State *lua) {
  lua_pushboolean(lua, checkHandleObject(lua, 1)->light.has_value());
  return 1;
}

int l_objGetCamera(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  GameObject *object = findHandleOpt(handle);
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

int l_objSetCamera(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
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

int l_objGetLightColor(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  GameObject *object = findHandleOpt(handle);
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

int l_objSetLightColor(lua_State *lua) {
  GameObject *object = checkHandleObject(lua, 1);
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

int l_objSetMainCamera(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  GameObject *object = findHandleOpt(handle);
  if (object == nullptr) {
    return luaL_error(lua, "invalid object id");
  }
  if (!object->camera.has_value()) {
    return luaL_error(lua, "object has no camera component");
  }
  handle->scene->setMainCamera(object->id);
  return 0;
}

int l_objIndex(lua_State *lua) {
  checkHandle(lua, 1);
  const char *key = luaL_checkstring(lua, 2);
  if (std::strcmp(key, "id") == 0) {
    lua_pushinteger(lua, handleId(lua, 1));
    return 1;
  }
  lua_getmetatable(lua, 1);
  lua_pushvalue(lua, 2);
  lua_rawget(lua, -2);
  return 1;
}

int l_objNewIndex(lua_State *lua) {
  checkHandle(lua, 1);
  return luaL_error(lua, "GameObject is read-only");
}

int l_objEq(lua_State *lua) {
  if (!isGameObject(lua, 1) || !isGameObject(lua, 2)) {
    lua_pushboolean(lua, false);
    return 1;
  }
  ObjectHandle *a = static_cast<ObjectHandle *>(lua_touserdata(lua, 1));
  ObjectHandle *b = static_cast<ObjectHandle *>(lua_touserdata(lua, 2));
  lua_pushboolean(lua, a->id == b->id && a->scene == b->scene);
  return 1;
}

int l_objToString(lua_State *lua) {
  ObjectHandle *handle = checkHandle(lua, 1);
  GameObject *object = findHandleOpt(handle);
  if (object != nullptr) {
    std::string s = "GameObject(id=" + std::to_string(handle->id) + ", name='" +
                    object->name + "')";
    lua_pushlstring(lua, s.c_str(), s.size());
  } else {
    std::string s =
        "GameObject(id=" + std::to_string(handle->id) + ", invalid)";
    lua_pushlstring(lua, s.c_str(), s.size());
  }
  return 1;
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

const Binding OBJECT_BINDINGS[] = {
    {"getId", l_objGetId},
    {"isValid", l_objIsValid},
    {"getName", l_objGetName},
    {"getPosition", l_objGetPosition},
    {"setPosition", l_objSetPosition},
    {"getRotation", l_objGetRotation},
    {"setRotation", l_objSetRotation},
    {"setRotationEuler", l_objSetRotationEuler},
    {"getScale", l_objGetScale},
    {"setScale", l_objSetScale},
    {"translate", l_objTranslate},
    {"rotateAxisAngle", l_objRotateAxisAngle},
    {"hasRenderer", l_objHasRenderer},
    {"hasCamera", l_objHasCamera},
    {"hasLight", l_objHasLight},
    {"getCamera", l_objGetCamera},
    {"setCamera", l_objSetCamera},
    {"getLightColor", l_objGetLightColor},
    {"setLightColor", l_objSetLightColor},
    {"setMainCamera", l_objSetMainCamera},
};

} // namespace

void registerSceneApi(lua_State *lua, Scene &scene) {
  luaL_newmetatable(lua, GAMEOBJECT_MT);
  for (const Binding &binding : OBJECT_BINDINGS) {
    lua_pushcfunction(lua, binding.fn);
    lua_setfield(lua, -2, binding.name);
  }
  lua_pushcfunction(lua, l_objIndex);
  lua_setfield(lua, -2, "__index");
  lua_pushcfunction(lua, l_objNewIndex);
  lua_setfield(lua, -2, "__newindex");
  lua_pushcfunction(lua, l_objEq);
  lua_setfield(lua, -2, "__eq");
  lua_pushcfunction(lua, l_objToString);
  lua_setfield(lua, -2, "__tostring");
  lua_pop(lua, 1);

  lua_newtable(lua);
  for (const Binding &binding : BINDINGS) {
    lua_pushlightuserdata(lua, &scene);
    lua_pushcclosure(lua, binding.fn, 1);
    lua_setfield(lua, -2, binding.name);
  }
  lua_setglobal(lua, "scene");
}
