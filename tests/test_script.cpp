#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "scene/scene.hpp"
#include "script/api/input.hpp"
#include "script/api/input_state.hpp"
#include "script/api/scene.hpp"
#include "script/api/time.hpp"
#include "script/manager.hpp"

#include <GLFW/glfw3.h>
#include <lua.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

struct LuaScene {
  lua_State *lua = nullptr;
  Scene scene;

  LuaScene() {
    lua = luaL_newstate();
    luaL_openlibs(lua);
    registerSceneApi(lua, scene);
  }

  ~LuaScene() { lua_close(lua); }
};

void requireOk(lua_State *lua, const std::string &code) {
  if (luaL_dostring(lua, code.c_str()) != LUA_OK) {
    std::string error = lua_tostring(lua, -1);
    FAIL("lua error: " << error << " in: " << code);
  }
  lua_settop(lua, 0);
}

void requireErr(lua_State *lua, const std::string &code) {
  INFO("expected lua error in: " << code);
  REQUIRE(luaL_dostring(lua, code.c_str()) != LUA_OK);
  lua_settop(lua, 0);
}

// Runs `return <expr>` and leaves the result on the stack.
void eval(lua_State *lua, const std::string &expr) {
  std::string code = "return " + expr;
  if (luaL_dostring(lua, code.c_str()) != LUA_OK) {
    std::string error = lua_tostring(lua, -1);
    FAIL("lua error: " << error << " in: " << code);
  }
}

bool evalNil(lua_State *lua, const std::string &expr) {
  eval(lua, expr);
  bool isNil = lua_isnil(lua, -1);
  lua_settop(lua, 0);
  return isNil;
}

int evalInt(lua_State *lua, const std::string &expr) {
  eval(lua, expr);
  int value = static_cast<int>(lua_tointeger(lua, -1));
  lua_settop(lua, 0);
  return value;
}

bool evalBool(lua_State *lua, const std::string &expr) {
  eval(lua, expr);
  bool value = lua_toboolean(lua, -1);
  lua_settop(lua, 0);
  return value;
}

float tableField(lua_State *lua, const char *key) {
  lua_getfield(lua, -1, key);
  float value = static_cast<float>(lua_tonumber(lua, -1));
  lua_pop(lua, 1);
  return value;
}

Vec3 evalVec3(lua_State *lua, const std::string &expr) {
  eval(lua, expr);
  REQUIRE(lua_istable(lua, -1));
  Vec3 v{tableField(lua, "x"), tableField(lua, "y"), tableField(lua, "z")};
  lua_settop(lua, 0);
  return v;
}

Quat evalQuat(lua_State *lua, const std::string &expr) {
  eval(lua, expr);
  REQUIRE(lua_istable(lua, -1));
  Quat q{tableField(lua, "w"), tableField(lua, "x"), tableField(lua, "y"),
         tableField(lua, "z")};
  lua_settop(lua, 0);
  return q;
}

void requireVec3Near(Vec3 actual, Vec3 expected) {
  REQUIRE(actual.x == Catch::Approx(expected.x).margin(1e-5f));
  REQUIRE(actual.y == Catch::Approx(expected.y).margin(1e-5f));
  REQUIRE(actual.z == Catch::Approx(expected.z).margin(1e-5f));
}

std::filesystem::path writeTempScript(const std::string &name,
                                      const std::string &source) {
  std::filesystem::path path = std::filesystem::temp_directory_path() / name;
  std::ofstream out(path);
  out << source;
  out.close();
  return path;
}

} // namespace
TEST_CASE("Scene API lookup", "[Script]") {
  LuaScene fix;
  GameObject &a = fix.scene.create("a");
  fix.scene.create("b");

  // findByName now returns a GameObject handle (userdata), not a raw integer.
  REQUIRE(evalInt(fix.lua, "scene.findByName('a'):getId()") ==
          static_cast<int>(a.id));
  REQUIRE(evalInt(fix.lua, "scene.findByName('a').id") ==
          static_cast<int>(a.id));
  REQUIRE(evalNil(fix.lua, "scene.findByName('missing')"));

  // Handles work wherever an id was previously accepted.
  REQUIRE(evalBool(fix.lua, "scene.isValid(scene.findByName('a'))"));
  eval(fix.lua, "scene.getName(scene.findByName('a'))");
  REQUIRE(std::string(lua_tostring(fix.lua, -1)) == "a");
  lua_settop(fix.lua, 0);

  // Raw integer ids keep working for backwards compatibility.
  REQUIRE(evalBool(fix.lua, "scene.isValid(" + std::to_string(a.id) + ")"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.isValid(9999)"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.isValid(nil)"));

  eval(fix.lua, "scene.getName(" + std::to_string(a.id) + ")");
  REQUIRE(std::string(lua_tostring(fix.lua, -1)) == "a");
  lua_settop(fix.lua, 0);
  REQUIRE(evalNil(fix.lua, "scene.getName(9999)"));
  REQUIRE(evalNil(fix.lua, "scene.getName(nil)"));

  REQUIRE(evalInt(fix.lua, "#scene.findAll()") == 2);
}

TEST_CASE("Scene API position is table-only", "[Script]") {
  LuaScene fix;
  GameObject &box = fix.scene.create("Box");
  box.transform.position = {1.0f, 2.0f, 3.0f};

  Vec3 got = evalVec3(fix.lua, "scene.getPosition(scene.findByName('Box'))");
  requireVec3Near(got, {1.0f, 2.0f, 3.0f});

  requireOk(fix.lua,
            "scene.setPosition(scene.findByName('Box'), {x = 4, y = 5, z = "
            "6})");
  requireVec3Near(box.transform.position, {4.0f, 5.0f, 6.0f});

  requireOk(fix.lua,
            "scene.translate(scene.findByName('Box'), {x = 1, y = -2, z = "
            "0.5})");
  requireVec3Near(box.transform.position, {5.0f, 3.0f, 6.5f});

  REQUIRE(evalNil(fix.lua, "scene.getPosition(9999)"));
  REQUIRE(evalNil(fix.lua, "scene.getPosition(nil)"));
  requireErr(fix.lua, "scene.setPosition(9999, {x = 0, y = 0, z = 0})");
  requireErr(fix.lua, "scene.setPosition(nil, {x = 0, y = 0, z = 0})");
  requireErr(fix.lua, "scene.setPosition(scene.findByName('Box'), 1, 2, 3)");
  requireErr(fix.lua, "scene.translate(9999, {x = 0, y = 0, z = 0})");
}

TEST_CASE("Scene API rotation in radians", "[Script]") {
  LuaScene fix;
  GameObject &box = fix.scene.create("Box");

  Quat identity =
      evalQuat(fix.lua, "scene.getRotation(scene.findByName('Box'))");
  REQUIRE(identity.w == Catch::Approx(1.0f).margin(1e-6f));

  requireOk(fix.lua,
            "scene.setRotation(scene.findByName('Box'), {w = 2, x = 0, y = 0, "
            "z = 0})");
  Quat normalized =
      evalQuat(fix.lua, "scene.getRotation(scene.findByName('Box'))");
  REQUIRE(normalized.w == Catch::Approx(1.0f).margin(1e-6f));
  REQUIRE(normalized.x == Catch::Approx(0.0f).margin(1e-6f));

  requireErr(fix.lua,
             "scene.setRotation(scene.findByName('Box'), {w = 0, x = 0, y = 0, "
             "z = 0})");

  requireOk(fix.lua,
            "scene.setRotationEuler(scene.findByName('Box'), {x = 0, y = 0, z "
            "= 0})");
  Quat fromEuler =
      evalQuat(fix.lua, "scene.getRotation(scene.findByName('Box'))");
  REQUIRE(fromEuler.w == Catch::Approx(1.0f).margin(1e-6f));

  requireOk(fix.lua, "scene.rotateAxisAngle(scene.findByName('Box'), {x = 0, "
                     "y = 1, z = 0}, math.pi / 2)");
  Quat rotated =
      evalQuat(fix.lua, "scene.getRotation(scene.findByName('Box'))");
  Quat expected =
      Quat::fromAxisAngle({0.0f, 1.0f, 0.0f}, static_cast<float>(M_PI) / 2.0f);
  REQUIRE(rotated.w == Catch::Approx(expected.w).margin(1e-5f));
  REQUIRE(rotated.x == Catch::Approx(expected.x).margin(1e-5f));
  REQUIRE(rotated.y == Catch::Approx(expected.y).margin(1e-5f));
  REQUIRE(rotated.z == Catch::Approx(expected.z).margin(1e-5f));
  (void)box;

  requireErr(fix.lua, "scene.rotateAxisAngle(scene.findByName('Box'), {x = 0, "
                      "y = 0, z = 0}, 1.0)");
  REQUIRE(evalNil(fix.lua, "scene.getRotation(9999)"));
  requireErr(fix.lua, "scene.setRotation(9999, {w = 1, x = 0, y = 0, z = 0})");
}

TEST_CASE("Scene API scale", "[Script]") {
  LuaScene fix;
  GameObject &box = fix.scene.create("Box");

  Vec3 def = evalVec3(fix.lua, "scene.getScale(scene.findByName('Box'))");
  requireVec3Near(def, {1.0f, 1.0f, 1.0f});

  requireOk(fix.lua,
            "scene.setScale(scene.findByName('Box'), {x = 2, y = 3, z = 4})");
  requireVec3Near(box.transform.scale, {2.0f, 3.0f, 4.0f});

  REQUIRE(evalNil(fix.lua, "scene.getScale(9999)"));
  requireErr(fix.lua, "scene.setScale(9999, {x = 1, y = 1, z = 1})");
}

TEST_CASE("Scene API camera", "[Script]") {
  LuaScene fix;
  GameObject &cam = fix.scene.create("cam");
  cam.camera = CameraParams{.fovY = 60.0f, .nearZ = 0.5f, .farZ = 100.0f};
  GameObject &plain = fix.scene.create("plain");
  (void)plain;

  REQUIRE(evalBool(fix.lua, "scene.hasCamera(scene.findByName('cam'))"));
  REQUIRE_FALSE(
      evalBool(fix.lua, "scene.hasCamera(scene.findByName('plain'))"));

  eval(fix.lua, "scene.getCamera(scene.findByName('cam'))");
  REQUIRE(tableField(fix.lua, "fovY") == Catch::Approx(60.0f).margin(1e-6f));
  REQUIRE(tableField(fix.lua, "nearZ") == Catch::Approx(0.5f).margin(1e-6f));
  REQUIRE(tableField(fix.lua, "farZ") == Catch::Approx(100.0f).margin(1e-6f));
  lua_settop(fix.lua, 0);

  REQUIRE(evalNil(fix.lua, "scene.getCamera(scene.findByName('plain'))"));
  REQUIRE(evalNil(fix.lua, "scene.getCamera(9999)"));

  requireOk(fix.lua, "scene.setCamera(scene.findByName('cam'), {fovY = 45, "
                     "nearZ = 0.1, farZ = 50})");
  REQUIRE(cam.camera->fovY == Catch::Approx(45.0f).margin(1e-6f));

  requireErr(fix.lua, "scene.setCamera(scene.findByName('plain'), {fovY = 45, "
                      "nearZ = 0.1, farZ = 50})");
  requireErr(fix.lua, "scene.setCamera(scene.findByName('cam'), {fovY = 45, "
                      "nearZ = 1.0, farZ = 0.5})");
  requireErr(fix.lua, "scene.setCamera(scene.findByName('cam'), {fovY = -10, "
                      "nearZ = 0.1, farZ = 50})");
  requireErr(fix.lua, "scene.setCamera(scene.findByName('cam'), {fovY = 45, "
                      "nearZ = 0, farZ = 50})");
}

TEST_CASE("Scene API light", "[Script]") {
  LuaScene fix;
  GameObject &lamp = fix.scene.create("lamp");
  lamp.light = LightParams{.color = {0.1f, 0.5f, 0.9f}};
  fix.scene.create("plain");

  REQUIRE(evalBool(fix.lua, "scene.hasLight(scene.findByName('lamp'))"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.hasLight(scene.findByName('plain'))"));
  REQUIRE_FALSE(
      evalBool(fix.lua, "scene.hasRenderer(scene.findByName('plain'))"));

  eval(fix.lua, "scene.getLightColor(scene.findByName('lamp'))");
  REQUIRE(tableField(fix.lua, "r") == Catch::Approx(0.1f).margin(1e-6f));
  REQUIRE(tableField(fix.lua, "g") == Catch::Approx(0.5f).margin(1e-6f));
  REQUIRE(tableField(fix.lua, "b") == Catch::Approx(0.9f).margin(1e-6f));
  lua_settop(fix.lua, 0);

  REQUIRE(evalNil(fix.lua, "scene.getLightColor(scene.findByName('plain'))"));

  requireOk(fix.lua,
            "scene.setLightColor(scene.findByName('lamp'), {r = 1, g = 0.8, b "
            "= 0.6})");
  requireVec3Near(lamp.light->color, {1.0f, 0.8f, 0.6f});

  requireErr(fix.lua, "scene.setLightColor(scene.findByName('plain'), {r = 1, "
                      "g = 1, b = 1})");
}

TEST_CASE("Scene API main camera", "[Script]") {
  LuaScene fix;
  GameObject &cam = fix.scene.create("cam");
  cam.camera = CameraParams{};
  GameObject &plain = fix.scene.create("plain");
  (void)plain;

  REQUIRE(evalNil(fix.lua, "scene.mainCamera()"));

  requireOk(fix.lua, "scene.setMainCamera(scene.findByName('cam'))");
  REQUIRE(fix.scene.mainCamera() == &cam);
  REQUIRE(evalInt(fix.lua, "scene.mainCamera():getId()") ==
          static_cast<int>(cam.id));
  REQUIRE(evalInt(fix.lua, "scene.mainCamera().id") ==
          static_cast<int>(cam.id));

  requireErr(fix.lua, "scene.setMainCamera(scene.findByName('plain'))");
  requireErr(fix.lua, "scene.setMainCamera(9999)");
}

TEST_CASE("ScriptManager runs Start once and Update per frame", "[Script]") {
  Scene scene;
  GameObject &box = scene.create("Box");
  box.transform.position = {0.0f, 0.0f, 0.0f};

  std::filesystem::path path = writeTempScript(
      "lodge_test_lifecycle.lua",
      "function Start()\n"
      "  scene.setPosition(scene.findByName('Box'), {x = 7, y = 7, z = 7})\n"
      "end\n"
      "function Update(dt)\n"
      "  scene.translate(scene.findByName('Box'), {x = 0, y = 0, z = dt})\n"
      "end\n");

  ScriptManager scripts(scene);
  scripts.loadScript(path);
  requireVec3Near(box.transform.position, {7.0f, 7.0f, 7.0f});

  scripts.onUpdate(1.0f);
  requireVec3Near(box.transform.position, {7.0f, 7.0f, 8.0f});
}

TEST_CASE("ScriptManager isolates scripts sharing Update", "[Script]") {
  Scene scene;
  GameObject &a = scene.create("A");
  GameObject &b = scene.create("B");

  std::filesystem::path pathA = writeTempScript(
      "lodge_test_iso_a.lua",
      "function Update(dt)\n"
      "  scene.setPosition(scene.findByName('A'), {x = 1, y = 0, z = 0})\n"
      "end\n");
  std::filesystem::path pathB = writeTempScript(
      "lodge_test_iso_b.lua",
      "function Update(dt)\n"
      "  scene.setPosition(scene.findByName('B'), {x = 2, y = 0, z = 0})\n"
      "end\n");

  ScriptManager scripts(scene);
  scripts.loadScript(pathA);
  scripts.loadScript(pathB);
  scripts.onUpdate(0.016f);

  requireVec3Near(a.transform.position, {1.0f, 0.0f, 0.0f});
  requireVec3Near(b.transform.position, {2.0f, 0.0f, 0.0f});
}

TEST_CASE("ScriptManager tolerates broken scripts", "[Script]") {
  Scene scene;
  GameObject &box = scene.create("Box");

  std::filesystem::path good = writeTempScript(
      "lodge_test_good.lua",
      "function Update(dt)\n"
      "  scene.setPosition(scene.findByName('Box'), {x = 3, y = 0, z = 0})\n"
      "end\n");
  std::filesystem::path bad =
      writeTempScript("lodge_test_bad.lua", "this is not valid lua (((\n");
  std::filesystem::path noUpdate =
      writeTempScript("lodge_test_noupdate.lua", "local x = 1\n");

  ScriptManager scripts(scene);
  scripts.loadScript(good);
  scripts.loadScript(bad);
  scripts.loadScript(noUpdate);
  scripts.onUpdate(0.016f);

  requireVec3Near(box.transform.position, {3.0f, 0.0f, 0.0f});
}

TEST_CASE("GameObject method syntax (test.lua style)", "[Script]") {
  LuaScene fix;
  GameObject &car = fix.scene.create("Car");
  car.transform.position = {2.0f, 3.5f, -2.0f};

  // Same calls as test.lua: find + if car + car:translate + car:rotateAxisAngle
  requireOk(fix.lua, "car = scene.findByName('Car');"
                     "assert(car, 'Car should exist');"
                     "car:translate({x = 0.0, y = 0.0, z = 1.0})");
  requireVec3Near(car.transform.position, {2.0f, 3.5f, -1.0f});

  requireOk(fix.lua,
            "car:rotateAxisAngle({x = 0.0, y = 0.0, z = 1.0}, math.rad(45.0))");
  Quat expected =
      Quat::fromAxisAngle({0.0f, 0.0f, 1.0f}, static_cast<float>(M_PI) / 4.0f);
  REQUIRE(car.transform.rotation.w == Catch::Approx(expected.w).margin(1e-5f));
  REQUIRE(car.transform.rotation.x == Catch::Approx(expected.x).margin(1e-5f));
  REQUIRE(car.transform.rotation.y == Catch::Approx(expected.y).margin(1e-5f));
  REQUIRE(car.transform.rotation.z == Catch::Approx(expected.z).margin(1e-5f));

  // Missing objects stay nil/falsy so `if car then` guards work.
  REQUIRE(evalNil(fix.lua, "scene.findByName('Missing')"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.findByName('Missing') ~= nil"));
  REQUIRE(evalBool(fix.lua, "scene.findByName('Car') ~= nil"));
  requireOk(fix.lua, "if scene.findByName('Missing') then error('should be "
                     "falsy') end");
  requireOk(fix.lua,
            "if not scene.findByName('Car') then error('should be truthy') "
            "end");

  // Degenerate axis still errors via method syntax.
  requireErr(fix.lua, "scene.findByName('Car'):rotateAxisAngle({x = 0, y = 0, "
                      "z = 0}, 1.0)");
}

TEST_CASE("GameObject methods mirror scene API", "[Script]") {
  LuaScene fix;
  GameObject &box = fix.scene.create("Box");
  GameObject &cam = fix.scene.create("cam");
  cam.camera = CameraParams{.fovY = 60.0f, .nearZ = 0.5f, .farZ = 100.0f};
  GameObject &lamp = fix.scene.create("lamp");
  lamp.light = LightParams{.color = {0.1f, 0.5f, 0.9f}};

  // Transform getters/setters.
  requireOk(fix.lua, "scene.findByName('Box'):setPosition({x = 4, y = 5, z = "
                     "6})");
  requireVec3Near(box.transform.position, {4.0f, 5.0f, 6.0f});
  requireVec3Near(evalVec3(fix.lua, "scene.findByName('Box'):getPosition()"),
                  {4.0f, 5.0f, 6.0f});

  requireOk(fix.lua, "scene.findByName('Box'):setScale({x = 2, y = 3, z = 4})");
  requireVec3Near(box.transform.scale, {2.0f, 3.0f, 4.0f});
  requireVec3Near(evalVec3(fix.lua, "scene.findByName('Box'):getScale()"),
                  {2.0f, 3.0f, 4.0f});

  requireOk(fix.lua, "scene.findByName('Box'):setRotation({w = 2, x = 0, y = "
                     "0, z = 0})");
  REQUIRE(box.transform.rotation.w == Catch::Approx(1.0f).margin(1e-6f));
  requireErr(fix.lua, "scene.findByName('Box'):setRotation({w = 0, x = 0, y = "
                      "0, z = 0})");

  requireOk(fix.lua, "scene.findByName('Box'):setRotationEuler({x = 0, y = 0, "
                     "z = 0})");
  REQUIRE(box.transform.rotation.w == Catch::Approx(1.0f).margin(1e-6f));

  // Name / validity / components.
  eval(fix.lua, "scene.findByName('Box'):getName()");
  REQUIRE(std::string(lua_tostring(fix.lua, -1)) == "Box");
  lua_settop(fix.lua, 0);
  REQUIRE(evalBool(fix.lua, "scene.findByName('Box'):isValid()"));
  REQUIRE(evalBool(fix.lua, "scene.findByName('Box'):hasRenderer()") == false);
  box.renderer = ModelRenderer{ModelHandle{0}};
  REQUIRE(evalBool(fix.lua, "scene.findByName('Box'):hasRenderer()"));
  REQUIRE(evalBool(fix.lua, "scene.findByName('cam'):hasCamera()"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.findByName('Box'):hasCamera()"));
  REQUIRE(evalBool(fix.lua, "scene.findByName('lamp'):hasLight()"));

  // Camera / light through methods.
  eval(fix.lua, "scene.findByName('cam'):getCamera()");
  REQUIRE(tableField(fix.lua, "fovY") == Catch::Approx(60.0f).margin(1e-6f));
  lua_settop(fix.lua, 0);
  requireOk(fix.lua, "scene.findByName('cam'):setCamera({fovY = 45, nearZ = "
                     "0.1, farZ = 50})");
  REQUIRE(cam.camera->fovY == Catch::Approx(45.0f).margin(1e-6f));
  requireErr(fix.lua, "scene.findByName('Box'):setCamera({fovY = 45, nearZ = "
                      "0.1, farZ = 50})");

  eval(fix.lua, "scene.findByName('lamp'):getLightColor()");
  REQUIRE(tableField(fix.lua, "r") == Catch::Approx(0.1f).margin(1e-6f));
  lua_settop(fix.lua, 0);
  requireOk(fix.lua, "scene.findByName('lamp'):setLightColor({r = 1, g = 0.8, "
                     "b = 0.6})");
  requireVec3Near(lamp.light->color, {1.0f, 0.8f, 0.6f});

  // setMainCamera as a method with no extra args.
  requireOk(fix.lua, "scene.findByName('cam'):setMainCamera()");
  REQUIRE(fix.scene.mainCamera() == &cam);
  requireErr(fix.lua, "scene.findByName('Box'):setMainCamera()");

  // Handles support ==, tostring, .id and are read-only.
  REQUIRE(evalBool(fix.lua, "scene.findByName('Box') == "
                            "scene.findByName('Box')"));
  REQUIRE_FALSE(evalBool(fix.lua, "scene.findByName('Box') == "
                                  "scene.findByName('cam')"));
  eval(fix.lua, "tostring(scene.findByName('Box'))");
  REQUIRE(std::string(lua_tostring(fix.lua, -1)).find("Box") !=
          std::string::npos);
  lua_settop(fix.lua, 0);
  requireErr(fix.lua, "scene.findByName('Box').id = 5");
  requireErr(fix.lua, "scene.findByName('Box').foo = 1");

  // findAll returns handles and mainCamera returns a handle.
  REQUIRE(evalInt(fix.lua, "#scene.findAll()") == 3);
  REQUIRE(evalInt(fix.lua, "scene.findAll()[1]:getId()") >= 1);
  REQUIRE(evalBool(fix.lua, "scene.mainCamera():hasCamera()"));
}

TEST_CASE("ScriptManager runs method-style Update", "[Script]") {
  Scene scene;
  GameObject &car = scene.create("Car");
  car.transform.position = {0.0f, 0.0f, 0.0f};

  std::filesystem::path path = writeTempScript(
      "lodge_test_method_update.lua",
      "function Update(dt)\n"
      "  local c = scene.findByName('Car')\n"
      "  if c then\n"
      "    c:translate({x = 0, y = 0, z = dt * 1.0})\n"
      "    c:rotateAxisAngle({x = 0, y = 0, z = 1.0}, math.rad(dt * 45.0))\n"
      "  end\n"
      "end\n");

  ScriptManager scripts(scene);
  scripts.loadScript(path);
  scripts.onUpdate(1.0f);

  requireVec3Near(car.transform.position, {0.0f, 0.0f, 1.0f});
  Quat expected =
      Quat::fromAxisAngle({0.0f, 0.0f, 1.0f}, static_cast<float>(M_PI) / 4.0f);
  REQUIRE(car.transform.rotation.w == Catch::Approx(expected.w).margin(1e-5f));
  REQUIRE(car.transform.rotation.z == Catch::Approx(expected.z).margin(1e-5f));
}

TEST_CASE("InputState tracks keys and buttons", "[Script]") {
  InputState input;
  REQUIRE_FALSE(input.isKeyDown(GLFW_KEY_W));
  REQUIRE_FALSE(input.isMouseDown(GLFW_MOUSE_BUTTON_LEFT));

  input.onEvent(Event{events::KeyPressed{GLFW_KEY_W, 0}});
  REQUIRE(input.isKeyDown(GLFW_KEY_W));
  REQUIRE_FALSE(input.isKeyDown(GLFW_KEY_S));

  input.onEvent(Event{events::KeyReleased{GLFW_KEY_W, 0}});
  REQUIRE_FALSE(input.isKeyDown(GLFW_KEY_W));

  input.onEvent(Event{events::MouseButtonPressed{GLFW_MOUSE_BUTTON_LEFT, 0}});
  REQUIRE(input.isMouseDown(GLFW_MOUSE_BUTTON_LEFT));
  input.onEvent(Event{events::MouseButtonReleased{GLFW_MOUSE_BUTTON_LEFT, 0}});
  REQUIRE_FALSE(input.isMouseDown(GLFW_MOUSE_BUTTON_LEFT));

  // Unrelated event types don't affect held state.
  input.onEvent(Event{events::WindowResized{800, 600}});
  REQUIRE_FALSE(input.isKeyDown(GLFW_KEY_W));
}

TEST_CASE("InputState resolves key and button names", "[Script]") {
  REQUIRE(InputState::keyFromString("W") == GLFW_KEY_W);
  REQUIRE(InputState::keyFromString("w") == GLFW_KEY_W);
  REQUIRE(InputState::keyFromString("space") == GLFW_KEY_SPACE);
  REQUIRE(InputState::keyFromString("Escape") == GLFW_KEY_ESCAPE);
  REQUIRE(InputState::keyFromString("esc") == GLFW_KEY_ESCAPE);
  REQUIRE(InputState::keyFromString("Up") == GLFW_KEY_UP);
  REQUIRE(InputState::keyFromString("F1") == GLFW_KEY_F1);
  REQUIRE(InputState::keyFromString("f12") == GLFW_KEY_F12);
  REQUIRE(InputState::keyFromString("5") == GLFW_KEY_5);
  REQUIRE_FALSE(InputState::keyFromString("NoSuchKey").has_value());
  REQUIRE_FALSE(InputState::keyFromString("F99").has_value());

  REQUIRE(InputState::buttonFromString("left") == GLFW_MOUSE_BUTTON_LEFT);
  REQUIRE(InputState::buttonFromString("Right") == GLFW_MOUSE_BUTTON_RIGHT);
  REQUIRE(InputState::buttonFromString("middle") == GLFW_MOUSE_BUTTON_MIDDLE);
  REQUIRE_FALSE(InputState::buttonFromString("sideways").has_value());
}

TEST_CASE("InputState accumulates mouse and scroll per frame", "[Script]") {
  InputState input;
  REQUIRE_FALSE(input.hasMousePos());

  input.onEvent(Event{events::MouseMoved{100.0, 200.0}});
  REQUIRE(input.hasMousePos());
  // First sighting establishes position without a delta spike.
  REQUIRE(input.deltaX() == Catch::Approx(0.0));
  REQUIRE(input.deltaY() == Catch::Approx(0.0));

  input.onEvent(Event{events::MouseMoved{110.0, 190.0}});
  REQUIRE(input.deltaX() == Catch::Approx(10.0));
  REQUIRE(input.deltaY() == Catch::Approx(-10.0));

  input.onEvent(Event{events::MouseScrolled{0.0, 2.0}});
  input.onEvent(Event{events::MouseScrolled{1.0, -0.5}});
  REQUIRE(input.scrollX() == Catch::Approx(1.0));
  REQUIRE(input.scrollY() == Catch::Approx(1.5));

  input.endFrame();
  REQUIRE(input.deltaX() == Catch::Approx(0.0));
  REQUIRE(input.deltaY() == Catch::Approx(0.0));
  REQUIRE(input.scrollX() == Catch::Approx(0.0));
  REQUIRE(input.scrollY() == Catch::Approx(0.0));
  // Position persists across frames.
  REQUIRE(input.mouseX() == Catch::Approx(110.0));
  REQUIRE(input.mouseY() == Catch::Approx(190.0));
}

namespace {

struct LuaInput {
  lua_State *lua = nullptr;
  InputState input;
  double elapsed = 0.0;

  LuaInput() {
    lua = luaL_newstate();
    luaL_openlibs(lua);
    registerInputApi(lua, &input);
    registerTimeApi(lua, &elapsed);
  }

  ~LuaInput() { lua_close(lua); }
};

} // namespace

TEST_CASE("Lua input bindings read held state", "[Script]") {
  LuaInput fix;
  fix.input.onEvent(Event{events::KeyPressed{GLFW_KEY_W, 0}});
  fix.input.onEvent(
      Event{events::MouseButtonPressed{GLFW_MOUSE_BUTTON_LEFT, 0}});
  fix.input.onEvent(Event{events::MouseMoved{10.0, 20.0}});
  fix.input.onEvent(Event{events::MouseMoved{13.0, 16.0}});
  fix.input.onEvent(Event{events::MouseScrolled{0.0, 1.0}});

  REQUIRE(evalBool(fix.lua, "input.isKeyDown('W')"));
  REQUIRE(evalBool(fix.lua, "input.isKeyDown('w')"));
  REQUIRE_FALSE(evalBool(fix.lua, "input.isKeyDown('S')"));
  REQUIRE(evalBool(fix.lua, "input.isKeyDown(87)")); // GLFW_KEY_W
  REQUIRE(evalBool(fix.lua, "input.isMouseDown('left')"));
  REQUIRE(evalBool(fix.lua, "input.isMouseDown(0)"));
  REQUIRE_FALSE(evalBool(fix.lua, "input.isMouseDown('right')"));

  Vec3 delta = evalVec3(fix.lua, "{x = input.mouseDelta().x, y = "
                                 "input.mouseDelta().y, z = 0}");
  REQUIRE(delta.x == Catch::Approx(3.0));
  REQUIRE(delta.y == Catch::Approx(-4.0));

  eval(fix.lua, "input.scroll()");
  REQUIRE(tableField(fix.lua, "y") == Catch::Approx(1.0));
  lua_settop(fix.lua, 0);

  eval(fix.lua, "input.mousePos()");
  REQUIRE(tableField(fix.lua, "x") == Catch::Approx(13.0));
  REQUIRE(tableField(fix.lua, "y") == Catch::Approx(16.0));
  lua_settop(fix.lua, 0);

  requireErr(fix.lua, "input.isKeyDown('NoSuchKey')");
  requireErr(fix.lua, "input.isMouseDown('sideways')");
  requireErr(fix.lua, "input.isMouseDown(99)");
}

TEST_CASE("Lua time.now tracks elapsed time", "[Script]") {
  LuaInput fix;
  eval(fix.lua, "time.now()");
  REQUIRE(lua_tonumber(fix.lua, -1) == Catch::Approx(0.0));
  lua_settop(fix.lua, 0);

  fix.elapsed = 1.5;
  eval(fix.lua, "time.now()");
  REQUIRE(lua_tonumber(fix.lua, -1) == Catch::Approx(1.5));
  lua_settop(fix.lua, 0);
}

TEST_CASE("ScriptManager exposes input and time to scripts", "[Script]") {
  Scene scene;
  GameObject &box = scene.create("Box");
  box.transform.position = {0.0f, 0.0f, 0.0f};

  std::filesystem::path path = writeTempScript(
      "lodge_test_input.lua",
      "function Update(dt)\n"
      "  if input.isKeyDown('D') then\n"
      "    scene.translate(scene.findByName('Box'), {x = dt * 2.0, y = 0, z = "
      "0})\n"
      "  end\n"
      "  local d = input.mouseDelta()\n"
      "  scene.translate(scene.findByName('Box'), {x = d.x, y = 0, z = 0})\n"
      "  local t = time.now()\n"
      "  scene.setPosition(scene.findByName('Clock'), {x = t, y = 0, z = 0})\n"
      "end\n");
  scene.create("Clock");

  ScriptManager scripts(scene);
  scripts.loadScript(path);

  // No input: only the clock moves with elapsed time.
  scripts.onUpdate(1.0f);
  requireVec3Near(box.transform.position, {0.0f, 0.0f, 0.0f});
  REQUIRE(scripts.elapsed() == Catch::Approx(1.0));

  // Hold D and move the mouse: box moves by key speed + delta.
  scripts.onEvent(Event{events::KeyPressed{GLFW_KEY_D, 0}});
  scripts.onEvent(Event{events::MouseMoved{0.0, 0.0}});
  scripts.onEvent(Event{events::MouseMoved{5.0, 0.0}});
  scripts.onUpdate(1.0f);
  requireVec3Near(box.transform.position, {7.0f, 0.0f, 0.0f});
  REQUIRE(scripts.elapsed() == Catch::Approx(2.0));

  // Deltas clear each frame: without new mouse events only the key applies.
  scripts.onUpdate(1.0f);
  requireVec3Near(box.transform.position, {9.0f, 0.0f, 0.0f});

  // Release D: box stays put, clock keeps tracking time.
  scripts.onEvent(Event{events::KeyReleased{GLFW_KEY_D, 0}});
  scripts.onUpdate(1.0f);
  requireVec3Near(box.transform.position, {9.0f, 0.0f, 0.0f});
  GameObject *clock = scene.findByName("Clock");
  REQUIRE(clock != nullptr);
  REQUIRE(clock->transform.position.x == Catch::Approx(4.0));
}
