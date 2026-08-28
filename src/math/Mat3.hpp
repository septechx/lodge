#pragma once

#include <array>
#include <cmath>

struct Mat3 {
  static const Mat3 IDENTITY;

  constexpr Mat3() = default;
  constexpr Mat3(std::array<float, 3> col0, std::array<float, 3> col1,
                 std::array<float, 3> col2)
      : m_data{col0[0], col0[1], col0[2], col1[0], col1[1],
               col1[2], col2[0], col2[1], col2[2]} {}

  constexpr float &operator()(std::size_t row, std::size_t col) {
    return m_data[col * 3 + row];
  }
  constexpr const float &operator()(std::size_t row, std::size_t col) const {
    return m_data[col * 3 + row];
  }

  constexpr float &operator[](std::size_t i) { return m_data[i]; }
  constexpr const float &operator[](std::size_t i) const { return m_data[i]; }

  constexpr Mat3 transpose() const {
    Mat3 r;
    for (std::size_t row = 0; row < 3; ++row)
      for (std::size_t col = 0; col < 3; ++col)
        r(row, col) = (*this)(col, row);
    return r;
  }

  constexpr float determinant() const {
    float a00 = (*this)(0, 0), a01 = (*this)(0, 1), a02 = (*this)(0, 2);
    float a10 = (*this)(1, 0), a11 = (*this)(1, 1), a12 = (*this)(1, 2);
    float a20 = (*this)(2, 0), a21 = (*this)(2, 1), a22 = (*this)(2, 2);
    return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) +
           a02 * (a10 * a21 - a11 * a20);
  }

  constexpr Mat3 inverse() const {
    float a00 = (*this)(0, 0), a01 = (*this)(0, 1), a02 = (*this)(0, 2);
    float a10 = (*this)(1, 0), a11 = (*this)(1, 1), a12 = (*this)(1, 2);
    float a20 = (*this)(2, 0), a21 = (*this)(2, 1), a22 = (*this)(2, 2);

    float det = determinant();
    if (std::abs(det) < 1e-8f) {
      return IDENTITY;
    }
    float invDet = 1.0f / det;

    Mat3 r;
    r(0, 0) = (a11 * a22 - a12 * a21) * invDet;
    r(0, 1) = (a02 * a21 - a01 * a22) * invDet;
    r(0, 2) = (a01 * a12 - a02 * a11) * invDet;
    r(1, 0) = (a12 * a20 - a10 * a22) * invDet;
    r(1, 1) = (a00 * a22 - a02 * a20) * invDet;
    r(1, 2) = (a02 * a10 - a00 * a12) * invDet;
    r(2, 0) = (a10 * a21 - a11 * a20) * invDet;
    r(2, 1) = (a01 * a20 - a00 * a21) * invDet;
    r(2, 2) = (a00 * a11 - a01 * a10) * invDet;
    return r;
  }

private:
  float m_data[9]{};
};

inline constexpr Mat3 Mat3::IDENTITY{
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, 1},
};
