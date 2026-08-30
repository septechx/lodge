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

TEST_CASE("Scene main light lookup", "[Scene]") {
  Scene scene;
  scene.create("model");
  GameObject &light = scene.create("Light");
  light.light = LightParams{};

  REQUIRE(scene.mainLight() == &light);
}
