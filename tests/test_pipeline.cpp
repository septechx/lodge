#include <catch2/catch_test_macros.hpp>

#include "render/pipelines/builder.hpp"

TEST_CASE("VertexInputKind mapping", "[pipeline]") {
  REQUIRE(vertexAttributeCount(VertexInputKind::None) == 0);
  REQUIRE(vertexAttributeCount(VertexInputKind::StaticMesh) == 3);
  REQUIRE_FALSE(needsVertexInput(VertexInputKind::None));
  REQUIRE(needsVertexInput(VertexInputKind::StaticMesh));
}

TEST_CASE("toVkCull mapping", "[pipeline]") {
  REQUIRE(toVkCull(Cull::None) == VK_CULL_MODE_NONE);
  REQUIRE(toVkCull(Cull::Back) == VK_CULL_MODE_BACK_BIT);
}

TEST_CASE("blendAttachment mapping", "[pipeline]") {
  auto off = blendAttachmentFor(false);
  REQUIRE(off.blendEnable == VK_FALSE);
  auto on = blendAttachmentFor(true);
  REQUIRE(on.blendEnable == VK_TRUE);
  REQUIRE(on.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA);
  REQUIRE(on.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
}

TEST_CASE("depthStencil mapping", "[pipeline]") {
  auto rw = depthStencilFor(true, true);
  REQUIRE(rw.depthTestEnable == VK_TRUE);
  REQUIRE(rw.depthWriteEnable == VK_TRUE);
  REQUIRE(rw.depthCompareOp == VK_COMPARE_OP_LESS);

  auto sky = depthStencilFor(false, false);
  REQUIRE(sky.depthTestEnable == VK_FALSE);
  REQUIRE(sky.depthWriteEnable == VK_FALSE);

  // Transparent: test on, write off.
  auto glass = depthStencilFor(true, false);
  REQUIRE(glass.depthTestEnable == VK_TRUE);
  REQUIRE(glass.depthWriteEnable == VK_FALSE);
}

TEST_CASE("PipelineDesc defaults describe opaque", "[pipeline]") {
  PipelineDesc desc;
  desc.colorFormats[0] = VK_FORMAT_B8G8R8A8_SRGB;
  desc.depthFormat = VK_FORMAT_D32_SFLOAT;
  REQUIRE(desc.vertexInput == VertexInputKind::StaticMesh);
  REQUIRE(vertexAttributeCount(desc.vertexInput) == 3);
  REQUIRE(toVkCull(desc.cull) == VK_CULL_MODE_BACK_BIT);
}
