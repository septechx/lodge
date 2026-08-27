#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "math/Mat4.hpp"
#include "math/Quat.hpp"
#include "math/Vec2.hpp"
#include "math/Vec3.hpp"
#include "math/math.hpp"

#include <cmath>

#define PI LDG_PI

static constexpr float EPS = 1e-5f;

static bool approxEqual(float a, float b, float eps = EPS) {
  return std::abs(a - b) <= eps;
}

static bool vec3AlmostEqual(Vec3 a, Vec3 b, float eps = EPS) {
  return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps) &&
         approxEqual(a.z, b.z, eps);
}

static bool vec2AlmostEqual(Vec2 a, Vec2 b, float eps = EPS) {
  return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps);
}

static bool matAlmostEqual(const Mat4 &a, const Mat4 &b, float eps = EPS) {
  for (size_t r = 0; r < 4; ++r)
    for (size_t c = 0; c < 4; ++c)
      if (!approxEqual(a(r, c), b(r, c), eps))
        return false;
  return true;
}

static bool quatAlmostEqual(Quat a, Quat b, float eps = EPS) {
  // quaternions q and -q represent same rotation
  bool direct = approxEqual(a.w, b.w, eps) && approxEqual(a.x, b.x, eps) &&
                approxEqual(a.y, b.y, eps) && approxEqual(a.z, b.z, eps);
  bool neg = approxEqual(a.w, -b.w, eps) && approxEqual(a.x, -b.x, eps) &&
             approxEqual(a.y, -b.y, eps) && approxEqual(a.z, -b.z, eps);
  return direct || neg;
}

// Helper: apply Mat4 to Vec3 as point (w=1) or vector (w=0)
static Vec3 transformPoint(const Mat4 &m, Vec3 v) {
  float x = m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z + m(0, 3);
  float y = m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z + m(1, 3);
  float z = m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z + m(2, 3);
  float w = m(3, 0) * v.x + m(3, 1) * v.y + m(3, 2) * v.z + m(3, 3);
  if (w != 0.0f && w != 1.0f) {
    return {x / w, y / w, z / w};
  }
  return {x, y, z};
}

static Vec3 transformVector(const Mat4 &m, Vec3 v) {
  return {
      m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z,
      m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z,
      m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z,
  };
}

// ============================================================
// Vec3
// ============================================================

TEST_CASE("Vec3 dot product", "[Vec3]") {
  SECTION("orthogonal vectors dot to zero") {
    Vec3 a{1, 0, 0}, b{0, 1, 0};
    REQUIRE(a.dot(b) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(b.dot(a) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("parallel vectors") {
    Vec3 a{1, 0, 0}, b{1, 0, 0};
    REQUIRE(a.dot(b) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("self dot equals length squared") {
    Vec3 a{1, 2, 3};
    REQUIRE(a.dot(a) == Catch::Approx(a.length() * a.length()).margin(EPS));
  }
  SECTION("commutativity") {
    Vec3 a{1, 2, 3}, b{4, -5, 6};
    REQUIRE(a.dot(b) == Catch::Approx(b.dot(a)).margin(EPS));
  }
  SECTION("zero vector") {
    Vec3 z{0, 0, 0}, a{1, 2, 3};
    REQUIRE(z.dot(a) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("negative values") {
    Vec3 a{-1, -2, -3}, b{1, 2, 3};
    REQUIRE(a.dot(b) == Catch::Approx(-14.0f).margin(EPS));
  }
  SECTION("known values") {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    REQUIRE(a.dot(b) == Catch::Approx(32.0f).margin(EPS));
  }
}

TEST_CASE("Vec3 cross product", "[Vec3]") {
  SECTION("basis vectors") {
    Vec3 i{1, 0, 0}, j{0, 1, 0}, k{0, 0, 1};
    REQUIRE(vec3AlmostEqual(i.cross(j), k));
    REQUIRE(vec3AlmostEqual(j.cross(k), i));
    REQUIRE(vec3AlmostEqual(k.cross(i), j));
  }
  SECTION("anti-commutativity") {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    Vec3 ab = a.cross(b);
    Vec3 ba = b.cross(a);
    REQUIRE(vec3AlmostEqual(ab, {-ba.x, -ba.y, -ba.z}));
  }
  SECTION("parallel vectors cross zero") {
    Vec3 a{1, 2, 3}, b{2, 4, 6};
    REQUIRE(vec3AlmostEqual(a.cross(b), {0, 0, 0}));
    REQUIRE(vec3AlmostEqual(a.cross(a), {0, 0, 0}));
  }
  SECTION("orthogonal magnitude") {
    Vec3 a{1, 0, 0}, b{0, 2, 0};
    Vec3 c = a.cross(b);
    REQUIRE(c.length() == Catch::Approx(2.0f).margin(EPS));
  }
  SECTION("right-hand rule") {
    Vec3 a{1, 0, 0}, b{0, 1, 0};
    Vec3 c = a.cross(b);
    REQUIRE(c.z == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(c.x == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(c.y == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("general cross") {
    Vec3 a{2, 3, 4}, b{5, 6, 7};
    REQUIRE(vec3AlmostEqual(a.cross(b), {-3, 6, -3}));
  }
}

TEST_CASE("Vec3 length", "[Vec3]") {
  SECTION("zero vector") {
    Vec3 z{0, 0, 0};
    REQUIRE(z.length() == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("unit vectors") {
    REQUIRE(Vec3{1, 0, 0}.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(Vec3{0, 1, 0}.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(Vec3{0, 0, 1}.length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("3-4-5 triangle") {
    REQUIRE(Vec3{3, 4, 0}.length() == Catch::Approx(5.0f).margin(EPS));
    REQUIRE(Vec3{0, 3, 4}.length() == Catch::Approx(5.0f).margin(EPS));
  }
  SECTION("general") {
    REQUIRE(Vec3{1, 2, 2}.length() == Catch::Approx(3.0f).margin(EPS));
  }
  SECTION("negative components") {
    REQUIRE(Vec3{-3, -4, 0}.length() == Catch::Approx(5.0f).margin(EPS));
  }
  SECTION("length squared") {
    Vec3 a{1, 2, 3};
    REQUIRE(a.length() * a.length() == Catch::Approx(a.dot(a)).margin(EPS));
  }
}

TEST_CASE("Vec3 normalize", "[Vec3]") {
  SECTION("already normalized") {
    Vec3 a{1, 0, 0};
    REQUIRE(vec3AlmostEqual(a.normalize(), {1, 0, 0}));
  }
  SECTION("scaling to unit") {
    Vec3 a{2, 0, 0};
    REQUIRE(vec3AlmostEqual(a.normalize(), {1, 0, 0}));
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("arbitrary vector") {
    Vec3 a{0, 3, 4};
    REQUIRE(vec3AlmostEqual(a.normalize(), {0, 0.6f, 0.8f}));
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("normalized length is one") {
    Vec3 a{1, 2, 3};
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(1e-4f));
  }
  SECTION("direction preserved") {
    Vec3 a{5, -3, 2};
    Vec3 n = a.normalize();
    REQUIRE(n.dot(a.normalize()) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("negative vector") {
    Vec3 a{-2, 0, 0};
    REQUIRE(vec3AlmostEqual(a.normalize(), {-1, 0, 0}));
  }
}

TEST_CASE("Vec3 arithmetic", "[Vec3]") {
  SECTION("addition") {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    REQUIRE(vec3AlmostEqual(a + b, {5, 7, 9}));
  }
  SECTION("addition with zero") {
    Vec3 a{1, 2, 3}, z{0, 0, 0};
    REQUIRE(vec3AlmostEqual(a + z, a));
  }
  SECTION("addition commutes") {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    REQUIRE(vec3AlmostEqual(a + b, b + a));
  }
  SECTION("subtraction") {
    Vec3 a{4, 5, 6}, b{1, 2, 3};
    REQUIRE(vec3AlmostEqual(a - b, {3, 3, 3}));
  }
  SECTION("subtract self is zero") {
    Vec3 a{1, 2, 3};
    REQUIRE(vec3AlmostEqual(a - a, {0, 0, 0}));
  }
  SECTION("scalar multiplication") {
    Vec3 a{1, 2, 3};
    REQUIRE(vec3AlmostEqual(a * 2.0f, {2, 4, 6}));
    REQUIRE(vec3AlmostEqual(a * 0.0f, {0, 0, 0}));
    REQUIRE(vec3AlmostEqual(a * -1.0f, {-1, -2, -3}));
  }
  SECTION("scalar multiplication commutes with free function") {
    Vec3 a{1, 2, 3};
    REQUIRE(vec3AlmostEqual(a * 3.0f, 3.0f * a));
  }
  SECTION("distributivity") {
    Vec3 a{1, 2, 3}, b{4, 5, 6};
    float s = 2.0f;
    REQUIRE(vec3AlmostEqual((a + b) * s, a * s + b * s));
  }
}

// ============================================================
// Vec2
// ============================================================

TEST_CASE("Vec2 dot product", "[Vec2]") {
  SECTION("orthogonal vectors dot to zero") {
    Vec2 a{1, 0}, b{0, 1};
    REQUIRE(a.dot(b) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(b.dot(a) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("parallel vectors") {
    Vec2 a{1, 0}, b{1, 0};
    REQUIRE(a.dot(b) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("self dot equals length squared") {
    Vec2 a{1, 2};
    REQUIRE(a.dot(a) == Catch::Approx(a.length() * a.length()).margin(EPS));
  }
  SECTION("commutativity") {
    Vec2 a{1, 2}, b{4, -6};
    REQUIRE(a.dot(b) == Catch::Approx(b.dot(a)).margin(EPS));
  }
  SECTION("zero vector") {
    Vec2 z{0, 0}, a{1, 2};
    REQUIRE(z.dot(a) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("negative values") {
    Vec2 a{-1, -2}, b{1, 2};
    REQUIRE(a.dot(b) == Catch::Approx(-5.0f).margin(EPS));
  }
  SECTION("known values") {
    Vec2 a{1, 2}, b{4, 5};
    REQUIRE(a.dot(b) == Catch::Approx(14.0f).margin(EPS));
  }
}

TEST_CASE("Vec2 length", "[Vec2]") {
  SECTION("zero vector") {
    Vec2 z{0, 0};
    REQUIRE(z.length() == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("unit vectors") {
    REQUIRE(Vec2{1, 0}.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(Vec2{0, 1}.length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("general") {
    REQUIRE(Vec2{2, 2}.length() == Catch::Approx(std::sqrt(8)).margin(EPS));
  }
  SECTION("negative components") {
    REQUIRE(Vec2{-2, -2}.length() == Catch::Approx(std::sqrt(8)).margin(EPS));
  }
  SECTION("length squared") {
    Vec2 a{1, 2};
    REQUIRE(a.length() * a.length() == Catch::Approx(a.dot(a)).margin(EPS));
  }
}

TEST_CASE("Vec2 normalize", "[Vec2]") {
  SECTION("already normalized") {
    Vec2 a{1, 0};
    REQUIRE(vec2AlmostEqual(a.normalize(), {1, 0}));
  }
  SECTION("scaling to unit") {
    Vec2 a{2, 0};
    REQUIRE(vec2AlmostEqual(a.normalize(), {1, 0}));
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("arbitrary vector") {
    Vec2 a{3, 4};
    REQUIRE(vec2AlmostEqual(a.normalize(), {0.6f, 0.8f}));
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("normalized length is one") {
    Vec2 a{1, 2};
    REQUIRE(a.normalize().length() == Catch::Approx(1.0f).margin(1e-4f));
  }
  SECTION("direction preserved") {
    Vec2 a{5, -3};
    Vec2 n = a.normalize();
    REQUIRE(n.dot(a.normalize()) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("negative vector") {
    Vec2 a{-2, 0};
    REQUIRE(vec2AlmostEqual(a.normalize(), {-1, 0}));
  }
}

TEST_CASE("Vec2 arithmetic", "[Vec2]") {
  SECTION("addition") {
    Vec2 a{1, 2}, b{4, 5};
    REQUIRE(vec2AlmostEqual(a + b, {5, 7}));
  }
  SECTION("addition with zero") {
    Vec2 a{1, 2}, z{0, 0};
    REQUIRE(vec2AlmostEqual(a + z, a));
  }
  SECTION("addition commutes") {
    Vec2 a{1, 2}, b{4, 5};
    REQUIRE(vec2AlmostEqual(a + b, b + a));
  }
  SECTION("subtraction") {
    Vec2 a{4, 5}, b{1, 2};
    REQUIRE(vec2AlmostEqual(a - b, {3, 3}));
  }
  SECTION("subtract self is zero") {
    Vec2 a{1, 2};
    REQUIRE(vec2AlmostEqual(a - a, {0, 0}));
  }
  SECTION("scalar multiplication") {
    Vec2 a{1, 2};
    REQUIRE(vec2AlmostEqual(a * 2.0f, {2, 4}));
    REQUIRE(vec2AlmostEqual(a * 0.0f, {0, 0}));
    REQUIRE(vec2AlmostEqual(a * -1.0f, {-1, -2}));
  }
  SECTION("scalar multiplication commutes with free function") {
    Vec2 a{1, 2};
    REQUIRE(vec2AlmostEqual(a * 3.0f, 3.0f * a));
  }
  SECTION("distributivity") {
    Vec2 a{1, 2}, b{4, 5};
    float s = 2.0f;
    REQUIRE(vec2AlmostEqual((a + b) * s, a * s + b * s));
  }
}

// ============================================================
// Mat4
// ============================================================

TEST_CASE("Mat4 identity", "[Mat4]") {
  SECTION("diagonal is one, off diagonal zero") {
    for (size_t r = 0; r < 4; ++r) {
      for (size_t c = 0; c < 4; ++c) {
        float expected = (r == c) ? 1.0f : 0.0f;
        REQUIRE(Mat4::IDENTITY(r, c) == Catch::Approx(expected).margin(EPS));
      }
    }
  }
  SECTION("multiply by identity is no-op") {
    Mat4 a = Mat4::translate({1, 2, 3}) * Mat4::scale({2, 3, 4});
    REQUIRE(matAlmostEqual(a * Mat4::IDENTITY, a));
    REQUIRE(matAlmostEqual(Mat4::IDENTITY * a, a));
  }
  SECTION("column constructor maps correctly") {
    Mat4 m{
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16},
    };
    REQUIRE(m(0, 0) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(1, 0) == Catch::Approx(2.0f).margin(EPS));
    REQUIRE(m(2, 0) == Catch::Approx(3.0f).margin(EPS));
    REQUIRE(m(3, 0) == Catch::Approx(4.0f).margin(EPS));
    REQUIRE(m(0, 1) == Catch::Approx(5.0f).margin(EPS));
    REQUIRE(m(1, 1) == Catch::Approx(6.0f).margin(EPS));
    REQUIRE(m(0, 3) == Catch::Approx(13.0f).margin(EPS));
    REQUIRE(m(3, 3) == Catch::Approx(16.0f).margin(EPS));
  }
}

TEST_CASE("Mat4 perspective", "[Mat4]") {
  SECTION("basic 90 deg aspect 1 near 0.1 far 100") {
    Mat4 p = Mat4::perspective(90.0f, 1.0f, 0.1f, 100.0f);
    float f = 1.0f / std::tan(90.0f * PI / 180.0f / 2.0f);
    REQUIRE(p(0, 0) == Catch::Approx(f / 1.0f).margin(EPS));
    REQUIRE(p(1, 1) == Catch::Approx(-f).margin(EPS));
    REQUIRE(p(2, 2) == Catch::Approx(100.0f / (0.1f - 100.0f)).margin(EPS));
    REQUIRE(p(2, 3) ==
            Catch::Approx(100.0f * 0.1f / (0.1f - 100.0f)).margin(EPS));
    REQUIRE(p(3, 2) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(p(0, 1) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(p(0, 2) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(p(1, 0) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(p(3, 3) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("aspect ratio scales x") {
    Mat4 p1 = Mat4::perspective(90.0f, 1.0f, 0.1f, 100.0f);
    Mat4 p2 = Mat4::perspective(90.0f, 2.0f, 0.1f, 100.0f);
    REQUIRE(p2(0, 0) == Catch::Approx(p1(0, 0) / 2.0f).margin(EPS));
  }
  SECTION("60 deg FOV") {
    Mat4 p = Mat4::perspective(60.0f, 16.0f / 9.0f, 0.1f, 1000.0f);
    float f = 1.0f / std::tan(60.0f * PI / 180.0f / 2.0f);
    REQUIRE(p(0, 0) == Catch::Approx(f / (16.0f / 9.0f)).margin(1e-4f));
    REQUIRE(p(1, 1) == Catch::Approx(-f).margin(1e-4f));
  }
}

TEST_CASE("Mat4 lookAt", "[Mat4]") {
  SECTION("eye at origin looking down -Z gives identity") {
    Vec3 eye{0, 0, 0}, center{0, 0, -1}, up{0, 1, 0};
    Mat4 v = Mat4::lookAt(eye, center, up);
    REQUIRE(matAlmostEqual(v, Mat4::IDENTITY));
  }
  SECTION("eye at (0,0,5) looking at origin") {
    Vec3 eye{0, 0, 5}, center{0, 0, 0}, up{0, 1, 0};
    Mat4 v = Mat4::lookAt(eye, center, up);
    REQUIRE(v(0, 0) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(v(1, 1) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(v(2, 2) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(v(0, 3) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(v(1, 3) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(v(2, 3) == Catch::Approx(-5.0f).margin(EPS));
    REQUIRE(v(3, 3) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("transforms eye to origin") {
    Vec3 eye{2, 3, 5}, center{0, 0, 0}, up{0, 1, 0};
    Mat4 v = Mat4::lookAt(eye, center, up);
    Vec3 transformed = transformPoint(v, eye);
    REQUIRE(vec3AlmostEqual(transformed, {0, 0, 0}, 1e-4f));
  }
  SECTION("columns are orthonormal") {
    Vec3 eye{1, 2, 3}, center{0, 0, 0}, up{0, 1, 0};
    Mat4 v = Mat4::lookAt(eye, center, up);
    Vec3 right{v(0, 0), v(1, 0), v(2, 0)};
    Vec3 camUp{v(0, 1), v(1, 1), v(2, 1)};
    Vec3 back{v(0, 2), v(1, 2), v(2, 2)};
    REQUIRE(right.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(camUp.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(back.length() == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(right.dot(camUp) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(right.dot(back) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(camUp.dot(back) == Catch::Approx(0.0f).margin(EPS));
  }
}

TEST_CASE("Mat4 translate", "[Mat4]") {
  SECTION("zero translation is identity") {
    REQUIRE(matAlmostEqual(Mat4::translate({0, 0, 0}), Mat4::IDENTITY));
  }
  SECTION("translation values stored in last column") {
    Vec3 pos{1, 2, 3};
    Mat4 t = Mat4::translate(pos);
    REQUIRE(t(0, 3) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(t(1, 3) == Catch::Approx(2.0f).margin(EPS));
    REQUIRE(t(2, 3) == Catch::Approx(3.0f).margin(EPS));
    REQUIRE(t(3, 3) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(t(0, 0) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(t(1, 1) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(t(2, 2) == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("transforms point") {
    Mat4 t = Mat4::translate({5, -2, 3});
    Vec3 p{1, 1, 1};
    REQUIRE(vec3AlmostEqual(transformPoint(t, p), {6, -1, 4}));
  }
  SECTION("composition is additive") {
    Mat4 a = Mat4::translate({1, 2, 3});
    Mat4 b = Mat4::translate({4, 5, 6});
    Mat4 c = a * b;
    REQUIRE(c(0, 3) == Catch::Approx(5.0f).margin(EPS));
    REQUIRE(c(1, 3) == Catch::Approx(7.0f).margin(EPS));
    REQUIRE(c(2, 3) == Catch::Approx(9.0f).margin(EPS));
  }
}

TEST_CASE("Mat4 scale", "[Mat4]") {
  SECTION("scale matrix diagonal") {
    Vec3 s{2, 3, 4};
    Mat4 m = Mat4::scale(s);
    REQUIRE(m(0, 0) == Catch::Approx(2.0f).margin(EPS));
    REQUIRE(m(1, 1) == Catch::Approx(3.0f).margin(EPS));
    REQUIRE(m(2, 2) == Catch::Approx(4.0f).margin(EPS));
    REQUIRE(m(3, 3) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(0, 1) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(m(1, 0) == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("uniform scale transforms") {
    Mat4 m = Mat4::scale({2, 2, 2});
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 1, 1}), {2, 2, 2}));
  }
  SECTION("non-uniform scale") {
    Mat4 m = Mat4::scale({2, 3, 4});
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 1, 1}), {2, 3, 4}));
  }
  SECTION("scale by one is identity") {
    REQUIRE(matAlmostEqual(Mat4::scale({1, 1, 1}), Mat4::IDENTITY));
  }
}

TEST_CASE("Mat4 rotX", "[Mat4]") {
  SECTION("zero rotation is identity") {
    REQUIRE(matAlmostEqual(Mat4::rotX(0.0f), Mat4::IDENTITY));
  }
  SECTION("pi/2 rotation") {
    Mat4 m = Mat4::rotX(PI / 2.0f);
    REQUIRE(m(0, 0) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(1, 1) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(m(1, 2) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(m(2, 1) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(2, 2) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(vec3AlmostEqual(transformVector(m, {0, 1, 0}), {0, 0, 1}, 1e-4f));
  }
  SECTION("pi rotation") {
    Mat4 m = Mat4::rotX(PI);
    REQUIRE(m(1, 1) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(m(2, 2) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(vec3AlmostEqual(transformVector(m, {0, 1, 0}), {0, -1, 0}, 1e-4f));
  }
  SECTION("2pi returns to identity") {
    Mat4 m = Mat4::rotX(2.0f * PI);
    REQUIRE(matAlmostEqual(m, Mat4::IDENTITY, 1e-4f));
  }
  SECTION("negative angle is transpose") {
    Mat4 a = Mat4::rotX(PI / 4.0f);
    Mat4 b = Mat4::rotX(-PI / 4.0f);
    REQUIRE(matAlmostEqual(a * b, Mat4::IDENTITY, 1e-4f));
  }
}

TEST_CASE("Mat4 rotY", "[Mat4]") {
  SECTION("zero is identity") {
    REQUIRE(matAlmostEqual(Mat4::rotY(0.0f), Mat4::IDENTITY));
  }
  SECTION("pi/2 rotation") {
    Mat4 m = Mat4::rotY(PI / 2.0f);
    REQUIRE(m(0, 0) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(m(0, 2) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(2, 0) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(m(2, 2) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 0, 0}), {0, 0, -1}, 1e-4f));
    REQUIRE(vec3AlmostEqual(transformVector(m, {0, 0, 1}), {1, 0, 0}, 1e-4f));
  }
  SECTION("pi rotation") {
    Mat4 m = Mat4::rotY(PI);
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 0, 0}), {-1, 0, 0}, 1e-4f));
  }
}

TEST_CASE("Mat4 rotZ", "[Mat4]") {
  SECTION("zero is identity") {
    REQUIRE(matAlmostEqual(Mat4::rotZ(0.0f), Mat4::IDENTITY));
  }
  SECTION("pi/2 rotation") {
    Mat4 m = Mat4::rotZ(PI / 2.0f);
    REQUIRE(m(0, 0) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(m(0, 1) == Catch::Approx(-1.0f).margin(EPS));
    REQUIRE(m(1, 0) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(m(1, 1) == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 0, 0}), {0, 1, 0}, 1e-4f));
    REQUIRE(vec3AlmostEqual(transformVector(m, {0, 1, 0}), {-1, 0, 0}, 1e-4f));
  }
  SECTION("pi rotation") {
    Mat4 m = Mat4::rotZ(PI);
    REQUIRE(vec3AlmostEqual(transformVector(m, {1, 0, 0}), {-1, 0, 0}, 1e-4f));
  }
}

TEST_CASE("Mat4 rotEuler", "[Mat4]") {
  SECTION("zero euler is identity") {
    REQUIRE(matAlmostEqual(Mat4::rotEuler({0, 0, 0}), Mat4::IDENTITY));
  }
  SECTION("equals Rx*Ry*Rz") {
    Vec3 e{PI / 4.0f, PI / 3.0f, PI / 6.0f};
    Mat4 m = Mat4::rotEuler(e);
    Mat4 expected = Mat4::rotX(e.x) * Mat4::rotY(e.y) * Mat4::rotZ(e.z);
    REQUIRE(matAlmostEqual(m, expected, 1e-4f));
  }
  SECTION("single axis euler matches rot") {
    REQUIRE(matAlmostEqual(Mat4::rotEuler({PI / 2.0f, 0, 0}),
                           Mat4::rotX(PI / 2.0f), 1e-4f));
    REQUIRE(matAlmostEqual(Mat4::rotEuler({0, PI / 2.0f, 0}),
                           Mat4::rotY(PI / 2.0f), 1e-4f));
    REQUIRE(matAlmostEqual(Mat4::rotEuler({0, 0, PI / 2.0f}),
                           Mat4::rotZ(PI / 2.0f), 1e-4f));
  }
}

TEST_CASE("Mat4 fromQuat", "[Mat4][Quat]") {
  SECTION("identity quat gives identity") {
    REQUIRE(matAlmostEqual(Mat4::fromQuat(Quat::IDENTITY), Mat4::IDENTITY));
  }
  SECTION("matches rotX") {
    Quat q = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    REQUIRE(matAlmostEqual(Mat4::fromQuat(q), Mat4::rotX(PI / 2.0f), 1e-4f));
  }
  SECTION("matches rotY") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, PI / 2.0f);
    REQUIRE(matAlmostEqual(Mat4::fromQuat(q), Mat4::rotY(PI / 2.0f), 1e-4f));
  }
  SECTION("matches rotZ") {
    Quat q = Quat::fromAxisAngle({0, 0, 1}, PI / 2.0f);
    REQUIRE(matAlmostEqual(Mat4::fromQuat(q), Mat4::rotZ(PI / 2.0f), 1e-4f));
  }
  SECTION("fromQuat vs rotEuler via fromEuler") {
    Vec3 euler{PI / 4.0f, PI / 3.0f, PI / 6.0f};
    Quat q = Quat::fromEuler(euler);
    Mat4 m1 = Mat4::fromQuat(q);
    Mat4 m2 = Mat4::rotEuler(euler);
    REQUIRE(matAlmostEqual(m1, m2, 1e-4f));
  }
  SECTION("fromQuat transforms same as quat rotate") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, PI / 3.0f).normalize();
    Mat4 m = Mat4::fromQuat(q);
    Vec3 v{1, 2, 3};
    Vec3 rQuat = q.rotate(v);
    Vec3 rMat = transformVector(m, v);
    REQUIRE(vec3AlmostEqual(rQuat, rMat, 1e-4f));
  }
  SECTION("handles non-normalized quat") {
    Quat q = Quat::fromAxisAngle({0, 0, 1}, PI / 2.0f);
    Quat qScaled{q.w * 2.0f, q.x * 2.0f, q.y * 2.0f, q.z * 2.0f};
    REQUIRE(
        matAlmostEqual(Mat4::fromQuat(qScaled), Mat4::rotZ(PI / 2.0f), 1e-4f));
  }
}

TEST_CASE("Mat4 multiplication", "[Mat4]") {
  SECTION("associative") {
    Mat4 a = Mat4::translate({1, 2, 3});
    Mat4 b = Mat4::scale({2, 2, 2});
    Mat4 c = Mat4::rotZ(PI / 4.0f);
    REQUIRE(matAlmostEqual((a * b) * c, a * (b * c), 1e-4f));
  }
  SECTION("non-commutative") {
    Mat4 t = Mat4::translate({1, 0, 0});
    Mat4 s = Mat4::scale({2, 2, 2});
    REQUIRE(!matAlmostEqual(t * s, s * t));
    REQUIRE((t * s)(0, 3) == Catch::Approx(1.0f).margin(EPS));
    REQUIRE((s * t)(0, 3) == Catch::Approx(2.0f).margin(EPS));
  }
  SECTION("transposed vs multiplied") {
    Mat4 r = Mat4::rotX(PI / 2.0f) * Mat4::rotY(PI / 2.0f);
    REQUIRE(!matAlmostEqual(r, Mat4::IDENTITY));
    for (size_t rr = 0; rr < 4; ++rr)
      for (size_t cc = 0; cc < 4; ++cc)
        REQUIRE(std::isfinite(r(rr, cc)));
  }
}

// ============================================================
// Quat
// ============================================================

TEST_CASE("Quat identity", "[Quat]") {
  REQUIRE(Quat::IDENTITY.w == Catch::Approx(1.0f).margin(EPS));
  REQUIRE(Quat::IDENTITY.x == Catch::Approx(0.0f).margin(EPS));
  REQUIRE(Quat::IDENTITY.y == Catch::Approx(0.0f).margin(EPS));
  REQUIRE(Quat::IDENTITY.z == Catch::Approx(0.0f).margin(EPS));
  REQUIRE(Quat::IDENTITY.length() == Catch::Approx(1.0f).margin(EPS));
}

TEST_CASE("Quat fromAxisAngle", "[Quat]") {
  SECTION("zero angle is identity") {
    Quat q = Quat::fromAxisAngle({1, 0, 0}, 0.0f);
    REQUIRE(quatAlmostEqual(q, Quat::IDENTITY));
  }
  SECTION("90 deg around X") {
    Quat q = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    REQUIRE(q.w == Catch::Approx(std::cos(PI / 4.0f)).margin(EPS));
    REQUIRE(q.x == Catch::Approx(std::sin(PI / 4.0f)).margin(EPS));
    REQUIRE(q.y == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(q.z == Catch::Approx(0.0f).margin(EPS));
  }
  SECTION("90 deg around Z") {
    Quat q = Quat::fromAxisAngle({0, 0, 1}, PI / 2.0f);
    REQUIRE(q.w == Catch::Approx(std::cos(PI / 4.0f)).margin(EPS));
    REQUIRE(q.z == Catch::Approx(std::sin(PI / 4.0f)).margin(EPS));
  }
  SECTION("pi angle") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, PI);
    REQUIRE(q.w == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(q.y == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("non-normalized axis is normalized") {
    Quat q1 = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    Quat q2 = Quat::fromAxisAngle({2, 0, 0}, PI / 2.0f);
    REQUIRE(quatAlmostEqual(q1, q2));
  }
  SECTION("length is 1 for unit axis") {
    Quat q = Quat::fromAxisAngle({1, 1, 1}, PI / 3.0f);
    REQUIRE(q.length() == Catch::Approx(1.0f).margin(EPS));
  }
}

TEST_CASE("Quat fromEuler", "[Quat]") {
  SECTION("zero euler is identity") {
    Quat q = Quat::fromEuler({0, 0, 0});
    REQUIRE(quatAlmostEqual(q, Quat::IDENTITY));
  }
  SECTION("single axis matches fromAxisAngle") {
    Quat qx1 = Quat::fromEuler({PI / 2.0f, 0, 0});
    Quat qx2 = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    REQUIRE(quatAlmostEqual(qx1, qx2, 1e-4f));

    Quat qy1 = Quat::fromEuler({0, PI / 2.0f, 0});
    Quat qy2 = Quat::fromAxisAngle({0, 1, 0}, PI / 2.0f);
    REQUIRE(quatAlmostEqual(qy1, qy2, 1e-4f));

    Quat qz1 = Quat::fromEuler({0, 0, PI / 2.0f});
    Quat qz2 = Quat::fromAxisAngle({0, 0, 1}, PI / 2.0f);
    REQUIRE(quatAlmostEqual(qz1, qz2, 1e-4f));
  }
  SECTION("manual composition qx*qy*qz") {
    Vec3 euler{PI / 4.0f, PI / 3.0f, PI / 6.0f};
    float cx = std::cos(euler.x / 2), sx = std::sin(euler.x / 2);
    float cy = std::cos(euler.y / 2), sy = std::sin(euler.y / 2);
    float cz = std::cos(euler.z / 2), sz = std::sin(euler.z / 2);
    Quat qx{cx, sx, 0, 0}, qy{cy, 0, sy, 0}, qz{cz, 0, 0, sz};
    Quat expected = qx * qy * qz;
    REQUIRE(quatAlmostEqual(Quat::fromEuler(euler), expected, 1e-4f));
  }
}

TEST_CASE("Quat length and normalize", "[Quat]") {
  SECTION("identity length 1") {
    REQUIRE(Quat::IDENTITY.length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("length calculation") {
    Quat q{2, 0, 0, 0};
    REQUIRE(q.length() == Catch::Approx(2.0f).margin(EPS));
    Quat q2{1, 2, 3, 4};
    REQUIRE(q2.length() ==
            Catch::Approx(std::sqrt(1 + 4 + 9 + 16)).margin(EPS));
  }
  SECTION("normalize makes length 1") {
    Quat q{1, 2, 3, 4};
    REQUIRE(q.normalize().length() == Catch::Approx(1.0f).margin(EPS));
  }
  SECTION("normalize is idempotent") {
    Quat q{1, 2, 3, 4};
    Quat n = q.normalize();
    REQUIRE(quatAlmostEqual(n, n.normalize(), 1e-4f));
  }
  SECTION("normalize identity stays identity") {
    REQUIRE(quatAlmostEqual(Quat::IDENTITY.normalize(), Quat::IDENTITY));
  }
}

TEST_CASE("Quat conjugate", "[Quat]") {
  SECTION("identity conjugate is identity") {
    REQUIRE(quatAlmostEqual(Quat::IDENTITY.conjugate(), Quat::IDENTITY));
  }
  SECTION("double conjugate is original") {
    Quat q{0.5f, 1, 2, 3};
    REQUIRE(quatAlmostEqual(q.conjugate().conjugate(), q));
  }
  SECTION("conjugate negates xyz") {
    Quat q{1, 2, 3, 4};
    Quat c = q.conjugate();
    REQUIRE(c.w == Catch::Approx(1.0f).margin(EPS));
    REQUIRE(c.x == Catch::Approx(-2.0f).margin(EPS));
    REQUIRE(c.y == Catch::Approx(-3.0f).margin(EPS));
    REQUIRE(c.z == Catch::Approx(-4.0f).margin(EPS));
  }
  SECTION("q * conjugate gives length squared") {
    Quat q{1, 2, 3, 4};
    Quat prod = q * q.conjugate();
    float lenSq = q.length() * q.length();
    REQUIRE(prod.w == Catch::Approx(lenSq).margin(EPS));
    REQUIRE(prod.x == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(prod.y == Catch::Approx(0.0f).margin(EPS));
    REQUIRE(prod.z == Catch::Approx(0.0f).margin(EPS));
  }
}

TEST_CASE("Quat multiplication", "[Quat]") {
  SECTION("identity multiplication") {
    Quat q{0.5f, 1, 2, 3};
    REQUIRE(quatAlmostEqual(q * Quat::IDENTITY, q));
    REQUIRE(quatAlmostEqual(Quat::IDENTITY * q, q));
  }
  SECTION("associative") {
    Quat a = Quat::fromAxisAngle({1, 0, 0}, PI / 3.0f);
    Quat b = Quat::fromAxisAngle({0, 1, 0}, PI / 4.0f);
    Quat c = Quat::fromAxisAngle({0, 0, 1}, PI / 6.0f);
    REQUIRE(quatAlmostEqual((a * b) * c, a * (b * c), 1e-4f));
  }
  SECTION("non-commutative") {
    Quat a = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    Quat b = Quat::fromAxisAngle({0, 1, 0}, PI / 2.0f);
    REQUIRE(!quatAlmostEqual(a * b, b * a));
  }
  SECTION("length of product equals product of lengths") {
    Quat a{1, 2, 3, 4};
    Quat b{5, 6, 7, 8};
    REQUIRE((a * b).length() ==
            Catch::Approx(a.length() * b.length()).margin(1e-4f));
  }
  SECTION("unit quaternions product is unit") {
    Quat a = Quat::fromAxisAngle({1, 0, 0}, PI / 3.0f);
    Quat b = Quat::fromAxisAngle({0, 1, 0}, PI / 4.0f);
    REQUIRE((a * b).length() == Catch::Approx(1.0f).margin(EPS));
  }
}

TEST_CASE("Quat rotate", "[Quat]") {
  SECTION("identity rotate is no-op") {
    Vec3 v{1, 2, 3};
    REQUIRE(vec3AlmostEqual(Quat::IDENTITY.rotate(v), v));
  }
  SECTION("90 deg around Z") {
    Quat q = Quat::fromAxisAngle({0, 0, 1}, PI / 2.0f);
    REQUIRE(vec3AlmostEqual(q.normalize().rotate({1, 0, 0}), {0, 1, 0}, 1e-4f));
    REQUIRE(
        vec3AlmostEqual(q.normalize().rotate({0, 1, 0}), {-1, 0, 0}, 1e-4f));
  }
  SECTION("90 deg around Y") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, PI / 2.0f);
    REQUIRE(
        vec3AlmostEqual(q.normalize().rotate({1, 0, 0}), {0, 0, -1}, 1e-4f));
    REQUIRE(vec3AlmostEqual(q.normalize().rotate({0, 0, 1}), {1, 0, 0}, 1e-4f));
  }
  SECTION("90 deg around X") {
    Quat q = Quat::fromAxisAngle({1, 0, 0}, PI / 2.0f);
    REQUIRE(vec3AlmostEqual(q.normalize().rotate({0, 1, 0}), {0, 0, 1}, 1e-4f));
    REQUIRE(
        vec3AlmostEqual(q.normalize().rotate({0, 0, 1}), {0, -1, 0}, 1e-4f));
  }
  SECTION("180 deg") {
    Quat q = Quat::fromAxisAngle({0, 0, 1}, PI);
    REQUIRE(
        vec3AlmostEqual(q.normalize().rotate({1, 0, 0}), {-1, 0, 0}, 1e-4f));
  }
  SECTION("360 deg is identity") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, 2.0f * PI);
    Vec3 v{1, 2, 3};
    REQUIRE(vec3AlmostEqual(q.normalize().rotate(v), v, 1e-4f));
  }
  SECTION("inverse rotation returns to original") {
    Quat q = Quat::fromAxisAngle({1, 1, 1}, PI / 3.0f).normalize();
    Vec3 v{1, 2, 3};
    Vec3 rotated = q.rotate(v);
    Vec3 back = q.conjugate().rotate(rotated);
    REQUIRE(vec3AlmostEqual(back, v, 1e-4f));
  }
  SECTION("rotate vs matrix fromQuat consistency") {
    Quat q = Quat::fromAxisAngle({0, 1, 0}, PI / 4.0f).normalize();
    Vec3 v{1, 0, 0};
    Vec3 rQuat = q.rotate(v);
    Mat4 m = Mat4::fromQuat(q);
    Vec3 rMat = transformVector(m, v);
    REQUIRE(vec3AlmostEqual(rQuat, rMat, 1e-4f));
  }
  SECTION("rotate composition equals quat multiplication") {
    Quat a = Quat::fromAxisAngle({1, 0, 0}, PI / 4.0f).normalize();
    Quat b = Quat::fromAxisAngle({0, 1, 0}, PI / 4.0f).normalize();
    Vec3 v{1, 0, 0};
    Quat combined = a * b;
    Vec3 rCombined = combined.rotate(v);
    Vec3 rSequential = a.rotate(b.rotate(v));
    REQUIRE(vec3AlmostEqual(rCombined, rSequential, 1e-4f));
  }
}
