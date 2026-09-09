#include <catch2/catch_test_macros.hpp>

#include "render/draw_groups.hpp"
#include "render/material_store.hpp"
#include "render/pipelines/selection.hpp"
#include "scene/material.hpp"

TEST_CASE("pipelineFor maps kind and pass", "[material]") {
  REQUIRE(pipelineFor(MaterialKind::Opaque, Pass::Grab) ==
          PipelineId::OpaqueGrab);
  REQUIRE(pipelineFor(MaterialKind::Opaque, Pass::Main) ==
          PipelineId::OpaqueComp);
  REQUIRE(pipelineFor(MaterialKind::Opaque, Pass::Bake) ==
          PipelineId::OpaqueBake);
  REQUIRE(pipelineFor(MaterialKind::Transparent, Pass::Main) ==
          PipelineId::Transparent);
  REQUIRE_FALSE(pipelineFor(MaterialKind::Transparent, Pass::Grab).has_value());
  REQUIRE_FALSE(pipelineFor(MaterialKind::Transparent, Pass::Bake).has_value());
}

TEST_CASE("cullFor follows doubleSided", "[material]") {
  REQUIRE(cullFor(true) == Cull::None);
  REQUIRE(cullFor(false) == Cull::Back);
}

TEST_CASE("MaterialStore dedups identical materials", "[material]") {
  MaterialStore store;
  Material a;
  a.texture = TextureHandle{2};
  a.baseColorFactor = Vec4{1.0f, 0.5f, 0.25f, 1.0f};
  a.metallicFactor = 0.3f;

  Material same = a;
  Material diffTex = a;
  diffTex.texture = TextureHandle{3};
  Material diffColor = a;
  diffColor.baseColorFactor = Vec4{0.0f, 0.0f, 0.0f, 1.0f};
  Material diffKind = a;
  diffKind.kind = MaterialKind::Transparent;
  Material diffMr = a;
  diffMr.metallicRoughness = TextureHandle{7};
  Material diffNormal = a;
  diffNormal.normal = TextureHandle{8};

  REQUIRE(store.intern(a) == 0);
  REQUIRE(store.intern(same) == 0);
  REQUIRE(store.size() == 1);
  REQUIRE(store.intern(diffTex) == 1);
  REQUIRE(store.intern(diffColor) == 2);
  REQUIRE(store.intern(diffKind) == 3);
  REQUIRE(store.intern(diffMr) == 4);
  REQUIRE(store.intern(diffNormal) == 5);
  REQUIRE(store.size() == 6);
  REQUIRE(store.at(0).texture.index == 2);
}

TEST_CASE("late gizmo material triggers descriptor rebuild", "[material]") {
  // Regression for brick-textured light gizmo: init-time keys contain only
  // the brick box material, the gizmo (yellow, white, flat-normal) appears
  // later via the debug UI.
  Material brick;
  brick.texture = TextureHandle{3};
  brick.metallicRoughness = TextureHandle{4};
  brick.normal = TextureHandle{5};

  Material gizmo;
  gizmo.texture = TextureHandle{1};
  gizmo.metallicRoughness = TextureHandle{0};
  gizmo.normal = TextureHandle{2};

  REQUIRE_FALSE(materialsEqual(brick, gizmo));

  std::vector<Material> initKeys{brick};
  REQUIRE_FALSE(materialsContain(initKeys, gizmo));

  std::vector<Material> flat{brick, gizmo};
  std::vector<Material> currentUnique = collectUniqueMaterials(flat);
  REQUIRE(currentUnique.size() == 2);
  REQUIRE(uniqueMaterialsNeedRebuild(initKeys, currentUnique));

  // After rebuild the gizmo must be found at its own index, not fallback 0.
  std::vector<Material> rebuiltKeys = collectUniqueMaterials(flat);
  REQUIRE(materialsContain(rebuiltKeys, gizmo));
  REQUIRE_FALSE(uniqueMaterialsNeedRebuild(rebuiltKeys, currentUnique));

  // Same set in different order needs no rebuild (find is order-insensitive).
  std::vector<Material> reordered{gizmo, brick};
  std::vector<Material> reorderedUnique = collectUniqueMaterials(reordered);
  REQUIRE_FALSE(uniqueMaterialsNeedRebuild(rebuiltKeys, reorderedUnique));
}

TEST_CASE("groupDraws filters and sorts", "[material]") {
  std::vector<DrawItem> items = {
      {MaterialKind::Transparent, false, 5},
      {MaterialKind::Opaque, false, 2},
      {MaterialKind::Opaque, false, 1},
  };
  auto main = groupDraws(items, Pass::Main);
  REQUIRE(main.size() == 3);
  // OpaqueComp (grab/main/bake ordering) sorts before Transparent.
  REQUIRE(main.front().pipeline == PipelineId::OpaqueComp);
  REQUIRE(main.front().matIdx == 1);
  REQUIRE(main[1].matIdx == 2);
  REQUIRE(main.back().pipeline == PipelineId::Transparent);

  auto grab = groupDraws(items, Pass::Grab);
  REQUIRE(grab.size() == 2);
  for (const Draw &d : grab) {
    REQUIRE(d.pipeline == PipelineId::OpaqueGrab);
  }

  auto bake = groupDraws(items, Pass::Bake);
  REQUIRE(bake.size() == 2);
  for (const Draw &d : bake) {
    REQUIRE(d.pipeline == PipelineId::OpaqueBake);
  }
}
