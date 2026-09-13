#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "scene/scene.hpp"

TEST_CASE("Scene assigns unique ids", "[Scene]") {
  Scene scene;
  GameObject &a = scene.create("a");
  GameObject &b = scene.create("b");
  GameObject &c = scene.create("c");

  REQUIRE(a.id != b.id);
  REQUIRE(b.id != c.id);
  REQUIRE(a.id != c.id);
  REQUIRE(scene.find(a.id) == &a);
  REQUIRE(scene.find(b.id) == &b);
  REQUIRE(scene.find(c.id) == &c);
}

TEST_CASE("Scene main camera lookup", "[Scene]") {
  Scene scene;
  GameObject &model = scene.create("model");
  model.renderer = ModelRenderer{ModelHandle{0}};

  GameObject &camera = scene.create("Main Camera");
  camera.camera = CameraParams{};
  scene.setMainCamera(camera.id);

  scene.create("Light");

  REQUIRE(scene.mainCamera() == &camera);
}

TEST_CASE("Scene rejects main camera without a camera component", "[Scene]") {
  Scene scene;
  GameObject &object = scene.create("not a camera");
  scene.setMainCamera(object.id);

  REQUIRE(scene.mainCamera() == nullptr);

  object.camera = CameraParams{};
  REQUIRE(scene.mainCamera() == &object);
}

static void requireVec3Near(Vec3 actual, Vec3 expected) {
  REQUIRE(actual.x == Catch::Approx(expected.x).margin(1e-6f));
  REQUIRE(actual.y == Catch::Approx(expected.y).margin(1e-6f));
  REQUIRE(actual.z == Catch::Approx(expected.z).margin(1e-6f));
}

static void requireQuatNear(Quat actual, Quat expected) {
  REQUIRE(actual.w == Catch::Approx(expected.w).margin(1e-6f));
  REQUIRE(actual.x == Catch::Approx(expected.x).margin(1e-6f));
  REQUIRE(actual.y == Catch::Approx(expected.y).margin(1e-6f));
  REQUIRE(actual.z == Catch::Approx(expected.z).margin(1e-6f));
}

TEST_CASE("Scene serialize empty scene", "[Scene]") {
  Scene scene;
  ser::Value value = scene.serialize();

  const auto &map = value.asMap();
  REQUIRE(map.contains("objects"));
  REQUIRE(map.at("objects").asArray().empty());

  Scene restored;
  restored.deserialize(std::move(value));
  REQUIRE(restored.objects().empty());
}

TEST_CASE("Scene round-trip preserves names, count and order", "[Scene]") {
  Scene scene;
  scene.create("a");
  scene.create("b");
  scene.create("c");

  Scene restored;
  restored.deserialize(scene.serialize());

  REQUIRE(restored.objects().size() == 3);
  REQUIRE(restored.objects()[0].name == "a");
  REQUIRE(restored.objects()[1].name == "b");
  REQUIRE(restored.objects()[2].name == "c");
  REQUIRE(restored.findByName("a") != nullptr);
  REQUIRE(restored.findByName("b") != nullptr);
  REQUIRE(restored.findByName("c") != nullptr);
}

TEST_CASE("Scene round-trip preserves transforms", "[Scene]") {
  Scene scene;
  GameObject &plain = scene.create("plain");
  (void)plain;

  GameObject &moved = scene.create("moved");
  moved.transform.position = {1.0f, 2.0f, 3.0f};
  moved.transform.rotation = Quat{0.5f, 0.5f, 0.5f, 0.5f};
  moved.transform.scale = {2.0f, 3.0f, 4.0f};

  GameObject &other = scene.create("other");
  other.transform.position = {-4.5f, 0.25f, 10.0f};
  other.transform.rotation = Quat::IDENTITY;
  other.transform.scale = {0.5f, 0.5f, 0.5f};

  Scene restored;
  restored.deserialize(scene.serialize());

  REQUIRE(restored.objects().size() == 3);

  const GameObject *plainOut = restored.findByName("plain");
  REQUIRE(plainOut != nullptr);
  requireVec3Near(plainOut->transform.position, {0.0f, 0.0f, 0.0f});
  requireQuatNear(plainOut->transform.rotation, Quat::IDENTITY);
  requireVec3Near(plainOut->transform.scale, {1.0f, 1.0f, 1.0f});

  const GameObject *movedOut = restored.findByName("moved");
  REQUIRE(movedOut != nullptr);
  requireVec3Near(movedOut->transform.position, {1.0f, 2.0f, 3.0f});
  requireQuatNear(movedOut->transform.rotation, {0.5f, 0.5f, 0.5f, 0.5f});
  requireVec3Near(movedOut->transform.scale, {2.0f, 3.0f, 4.0f});

  const GameObject *otherOut = restored.findByName("other");
  REQUIRE(otherOut != nullptr);
  requireVec3Near(otherOut->transform.position, {-4.5f, 0.25f, 10.0f});
  requireQuatNear(otherOut->transform.rotation, Quat::IDENTITY);
  requireVec3Near(otherOut->transform.scale, {0.5f, 0.5f, 0.5f});
}

TEST_CASE("Scene round-trip preserves renderer", "[Scene]") {
  Scene scene;
  GameObject &withRenderer = scene.create("model");
  withRenderer.renderer = ModelRenderer{ModelHandle{7}};
  scene.create("empty");

  Scene restored;
  restored.deserialize(scene.serialize());

  const GameObject *modelOut = restored.findByName("model");
  REQUIRE(modelOut != nullptr);
  REQUIRE(modelOut->renderer.has_value());
  REQUIRE(modelOut->renderer->model.index == 7);

  const GameObject *emptyOut = restored.findByName("empty");
  REQUIRE(emptyOut != nullptr);
  REQUIRE_FALSE(emptyOut->renderer.has_value());
}

TEST_CASE("Scene round-trip preserves camera", "[Scene]") {
  Scene scene;
  GameObject &withCamera = scene.create("camera");
  withCamera.camera = CameraParams{.fovY = 60.0f, .nearZ = 0.5f, .farZ = 100.0f};
  scene.create("empty");

  Scene restored;
  restored.deserialize(scene.serialize());

  const GameObject *cameraOut = restored.findByName("camera");
  REQUIRE(cameraOut != nullptr);
  REQUIRE(cameraOut->camera.has_value());
  REQUIRE(cameraOut->camera->fovY == Catch::Approx(60.0f).margin(1e-6f));
  REQUIRE(cameraOut->camera->nearZ == Catch::Approx(0.5f).margin(1e-6f));
  REQUIRE(cameraOut->camera->farZ == Catch::Approx(100.0f).margin(1e-6f));

  const GameObject *emptyOut = restored.findByName("empty");
  REQUIRE(emptyOut != nullptr);
  REQUIRE_FALSE(emptyOut->camera.has_value());
}

TEST_CASE("Scene round-trip preserves light", "[Scene]") {
  Scene scene;
  GameObject &withLight = scene.create("light");
  withLight.light = LightParams{.color = {0.1f, 0.5f, 0.9f}};
  scene.create("empty");

  Scene restored;
  restored.deserialize(scene.serialize());

  const GameObject *lightOut = restored.findByName("light");
  REQUIRE(lightOut != nullptr);
  REQUIRE(lightOut->light.has_value());
  requireVec3Near(lightOut->light->color, {0.1f, 0.5f, 0.9f});

  const GameObject *emptyOut = restored.findByName("empty");
  REQUIRE(emptyOut != nullptr);
  REQUIRE_FALSE(emptyOut->light.has_value());
}

TEST_CASE("Scene round-trip preserves mixed components", "[Scene]") {
  Scene scene;

  GameObject &model = scene.create("model");
  model.transform.position = {1.0f, 0.0f, -2.0f};
  model.transform.rotation =
      Quat::fromAxisAngle({0.0f, 1.0f, 0.0f}, 0.7853982f);
  model.transform.scale = {1.0f, 2.0f, 1.0f};
  model.renderer = ModelRenderer{ModelHandle{3}};

  GameObject &camera = scene.create("camera");
  camera.transform.position = {0.0f, 1.0f, 5.0f};
  camera.camera = CameraParams{.fovY = 45.0f, .nearZ = 0.1f, .farZ = 50.0f};

  GameObject &light = scene.create("light");
  light.light = LightParams{.color = {1.0f, 0.8f, 0.6f}};

  scene.create("empty");

  Scene restored;
  restored.deserialize(scene.serialize());

  REQUIRE(restored.objects().size() == 4);

  const GameObject *modelOut = restored.findByName("model");
  REQUIRE(modelOut != nullptr);
  requireVec3Near(modelOut->transform.position, {1.0f, 0.0f, -2.0f});
  requireQuatNear(modelOut->transform.rotation,
                  Quat::fromAxisAngle({0.0f, 1.0f, 0.0f}, 0.7853982f));
  requireVec3Near(modelOut->transform.scale, {1.0f, 2.0f, 1.0f});
  REQUIRE(modelOut->renderer.has_value());
  REQUIRE(modelOut->renderer->model.index == 3);
  REQUIRE_FALSE(modelOut->camera.has_value());
  REQUIRE_FALSE(modelOut->light.has_value());

  const GameObject *cameraOut = restored.findByName("camera");
  REQUIRE(cameraOut != nullptr);
  requireVec3Near(cameraOut->transform.position, {0.0f, 1.0f, 5.0f});
  REQUIRE(cameraOut->camera.has_value());
  REQUIRE(cameraOut->camera->fovY == Catch::Approx(45.0f).margin(1e-6f));
  REQUIRE(cameraOut->camera->nearZ == Catch::Approx(0.1f).margin(1e-6f));
  REQUIRE(cameraOut->camera->farZ == Catch::Approx(50.0f).margin(1e-6f));
  REQUIRE_FALSE(cameraOut->renderer.has_value());
  REQUIRE_FALSE(cameraOut->light.has_value());

  const GameObject *lightOut = restored.findByName("light");
  REQUIRE(lightOut != nullptr);
  REQUIRE(lightOut->light.has_value());
  requireVec3Near(lightOut->light->color, {1.0f, 0.8f, 0.6f});
  REQUIRE_FALSE(lightOut->renderer.has_value());
  REQUIRE_FALSE(lightOut->camera.has_value());

  const GameObject *emptyOut = restored.findByName("empty");
  REQUIRE(emptyOut != nullptr);
  REQUIRE_FALSE(emptyOut->renderer.has_value());
  REQUIRE_FALSE(emptyOut->camera.has_value());
  REQUIRE_FALSE(emptyOut->light.has_value());
}

TEST_CASE("Scene deserialize clears existing objects", "[Scene]") {
  Scene scene;
  scene.create("a");
  scene.create("b");

  Scene target;
  target.create("stale");
  target.create("old");
  REQUIRE(target.objects().size() == 2);

  target.deserialize(scene.serialize());

  REQUIRE(target.objects().size() == 2);
  REQUIRE(target.findByName("a") != nullptr);
  REQUIRE(target.findByName("b") != nullptr);
  REQUIRE(target.findByName("stale") == nullptr);
  REQUIRE(target.findByName("old") == nullptr);
}

TEST_CASE("Scene deserialize assigns findable unique ids", "[Scene]") {
  Scene scene;
  scene.create("a");
  scene.create("b");
  scene.create("c");

  Scene restored;
  restored.deserialize(scene.serialize());

  REQUIRE(restored.objects().size() == 3);
  uint32_t first = restored.objects()[0].id;
  uint32_t second = restored.objects()[1].id;
  uint32_t third = restored.objects()[2].id;
  REQUIRE(first != second);
  REQUIRE(second != third);
  REQUIRE(first != third);
  REQUIRE(restored.find(first) != nullptr);
  REQUIRE(restored.find(second) != nullptr);
  REQUIRE(restored.find(third) != nullptr);

  GameObject &fresh = restored.create("fresh");
  REQUIRE(fresh.id != first);
  REQUIRE(fresh.id != second);
  REQUIRE(fresh.id != third);
}

TEST_CASE("Scene serialize encodes absent components as null", "[Scene]") {
  Scene scene;
  GameObject &object = scene.create("bare");
  (void)object;

  ser::Value value = scene.serialize();
  const auto &objects = value.asMap().at("objects").asArray();
  REQUIRE(objects.size() == 1);

  const auto &encoded = objects[0].asMap();
  REQUIRE(encoded.at("name").asString() == "bare");
  REQUIRE(encoded.at("renderer").isNull());
  REQUIRE(encoded.at("camera").isNull());
  REQUIRE(encoded.at("light").isNull());

  const auto &transform = encoded.at("transform").asMap();
  const auto &position = transform.at("position").asArray();
  REQUIRE(position.size() == 3);
  const auto &rotation = transform.at("rotation").asArray();
  REQUIRE(rotation.size() == 4);
  REQUIRE(rotation[0].asReal() == Catch::Approx(1.0f).margin(1e-6f));
  const auto &scale = transform.at("scale").asArray();
  REQUIRE(scale.size() == 3);
}

TEST_CASE("Scene double round-trip is stable", "[Scene]") {
  Scene scene;
  GameObject &model = scene.create("model");
  model.transform.position = {3.0f, -1.0f, 0.5f};
  model.renderer = ModelRenderer{ModelHandle{1}};

  GameObject &camera = scene.create("camera");
  camera.camera = CameraParams{.fovY = 70.0f, .nearZ = 0.2f, .farZ = 30.0f};

  Scene once;
  once.deserialize(scene.serialize());
  Scene twice;
  twice.deserialize(once.serialize());

  REQUIRE(twice.objects().size() == 2);
  const GameObject *modelOut = twice.findByName("model");
  REQUIRE(modelOut != nullptr);
  requireVec3Near(modelOut->transform.position, {3.0f, -1.0f, 0.5f});
  REQUIRE(modelOut->renderer.has_value());
  REQUIRE(modelOut->renderer->model.index == 1);

  const GameObject *cameraOut = twice.findByName("camera");
  REQUIRE(cameraOut != nullptr);
  REQUIRE(cameraOut->camera.has_value());
  REQUIRE(cameraOut->camera->fovY == Catch::Approx(70.0f).margin(1e-6f));
}

TEST_CASE("Scene round-trip preserves main camera by name", "[Scene]") {
  Scene scene;
  GameObject &camera = scene.create("Main Camera");
  camera.camera = CameraParams{};
  scene.setMainCamera(camera.id);
  scene.create("other");

  Scene restored;
  restored.deserialize(scene.serialize());

  REQUIRE(restored.mainCamera() != nullptr);
  REQUIRE(restored.mainCamera()->name == "Main Camera");
}

TEST_CASE("Scene model path round-trip via resolver", "[Scene]") {
  Scene scene;
  GameObject &a = scene.create("a");
  a.renderer = ModelRenderer{ModelHandle{1}};
  GameObject &b = scene.create("b");
  b.renderer = ModelRenderer{ModelHandle{2}};

  auto pathFor = [](ModelHandle h) -> std::optional<std::string> {
    if (h.index == 1)
      return std::string("models/Box6.glb");
    if (h.index == 2)
      return std::string("models/Car3.glb");
    return std::nullopt;
  };
  ser::Value value = scene.serializeWithModels(pathFor);

  const auto &objects = value.asMap().at("objects").asArray();
  REQUIRE(objects[0].asMap().at("renderer").asMap().at("model").asString() ==
          "models/Box6.glb");
  REQUIRE(objects[1].asMap().at("renderer").asMap().at("model").asString() ==
          "models/Car3.glb");

  auto handleFor = [](const std::string &p) -> std::optional<ModelHandle> {
    if (p == "models/Box6.glb")
      return ModelHandle{1};
    if (p == "models/Car3.glb")
      return ModelHandle{2};
    return std::nullopt;
  };
  Scene restored;
  restored.deserializeWithModels(value, handleFor);

  REQUIRE(restored.findByName("a")->renderer->model.index == 1);
  REQUIRE(restored.findByName("b")->renderer->model.index == 2);
}

TEST_CASE("Scene falls back to int index when no path known", "[Scene]") {
  Scene scene;
  GameObject &a = scene.create("a");
  a.renderer = ModelRenderer{ModelHandle{4}};

  ser::Value value = scene.serializeWithModels(
      [](ModelHandle) -> std::optional<std::string> { return std::nullopt; });
  REQUIRE(value.asMap()
              .at("objects")
              .asArray()[0]
              .asMap()
              .at("renderer")
              .asMap()
              .at("model")
              .asInt() == 4);

  // Legacy int payloads load without a resolver.
  Scene restored;
  restored.deserializeWithModels(
      value, [](const std::string &) -> std::optional<ModelHandle> {
        return std::nullopt;
      });
  REQUIRE(restored.findByName("a")->renderer->model.index == 4);
}

TEST_CASE("Scene skips renderer when model path cannot be resolved",
          "[Scene]") {
  Scene scene;
  GameObject &a = scene.create("a");
  a.renderer = ModelRenderer{ModelHandle{1}};

  ser::Value value = scene.serializeWithModels(
      [](ModelHandle) -> std::optional<std::string> {
        return std::string("models/Missing.glb");
      });

  Scene restored;
  restored.deserializeWithModels(
      value, [](const std::string &) -> std::optional<ModelHandle> {
        return std::nullopt;
      });
  REQUIRE_FALSE(restored.findByName("a")->renderer.has_value());
}

TEST_CASE("Scene accepts concise objects with missing keys", "[Scene]") {
  ser::Value value = ser::Value::Map{
      {"objects",
       ser::Value::Array{
           ser::Value::Map{{"name", "bare"}},
           ser::Value::Map{
               {"name", "cam"},
               {"camera", ser::Value::Map{}},
               {"transform",
                ser::Value::Map{
                    {"position", ser::Value::Array{0.0f, 1.0f, 2.0f}}}},
           },
       }},
      {"mainCamera", "cam"},
  };

  Scene restored;
  restored.deserializeWithModels(
      value, [](const std::string &) -> std::optional<ModelHandle> {
        return std::nullopt;
      });

  REQUIRE(restored.objects().size() == 2);
  const GameObject *bare = restored.findByName("bare");
  REQUIRE(bare != nullptr);
  requireVec3Near(bare->transform.position, {0.0f, 0.0f, 0.0f});
  requireQuatNear(bare->transform.rotation, Quat::IDENTITY);
  REQUIRE_FALSE(bare->renderer.has_value());

  const GameObject *cam = restored.findByName("cam");
  REQUIRE(cam != nullptr);
  REQUIRE(cam->camera.has_value());
  requireVec3Near(cam->transform.position, {0.0f, 1.0f, 2.0f});
  REQUIRE(restored.mainCamera() == cam);
}
