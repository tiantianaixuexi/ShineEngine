#pragma once

// Minimal Matrix4 for wasm demo (no STL / no libc).
// Layout matches three.js style: 16 floats in column-major order.
//
// elements indices:
// [ 0  4  8 12 ]
// [ 1  5  9 13 ]
// [ 2  6 10 14 ]
// [ 3  7 11 15 ]

#include "../util/math_def.h"

class Matrix4 {
public:
  float e[16] = {};

  static Matrix4 identity() {
    Matrix4 m{};
    m.e[0] = 1.0f;
    m.e[5] = 1.0f;
    m.e[10] = 1.0f;
    m.e[15] = 1.0f;
    return m;
  }

  static Matrix4 translation(float x, float y, float z = 0.0f) {
    Matrix4 m = identity();
    m.e[12] = x;
    m.e[13] = y;
    m.e[14] = z;
    return m;
  }

  static Matrix4 scale(float x, float y, float z = 1.0f) {
    Matrix4 m{};
    m.e[0] = x;
    m.e[5] = y;
    m.e[10] = z;
    m.e[15] = 1.0f;
    return m;
  }


  static Matrix4 rotationZ(float radians) {
    float c = shine::math::cos_approx(radians);
    float s = shine::math::sin_approx(radians);

    Matrix4 m = identity();
    // column-major for Z rotation
    m.e[0] = c;
    m.e[4] = -s;
    m.e[1] = s;
    m.e[5] = c;
    return m;
  }

  static Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) noexcept {
    Matrix4 r{};
    // r = a * b (column-major)
    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        float sum = 0.0f;
        for (int k = 0; k < 4; ++k) {
          sum += a.e[row + k * 4] * b.e[k + col * 4];
        }
        r.e[row + col * 4] = sum;
      }
    }
    return r;
  }

  // Transform a 2D point (x,y,0,1) by this matrix.
  inline void transformPoint2(float x, float y, float &outX,
                              float &outY) const noexcept {
    outX = e[0] * x + e[4] * y + e[12];
    outY = e[1] * x + e[5] * y + e[13];
  }

  inline void translation_set(float x, float y, float z = 0.0f) noexcept {
    e[12] = x;
    e[13] = y;
    e[14] = z;
  }

  inline void identity_set() noexcept {
    e[0] = 1.0f;
    e[5] = 1.0f;
    e[10] = 1.0f;
    e[15] = 1.0f;
  }

  inline void rotationZ_set(float radians) noexcept {
    float c = shine::math::cos_approx(radians);
    float s = shine::math::sin_approx(radians);
    e[0] = c;
    e[4] = -s;
    e[1] = s;
    e[5] = c;
  }

  inline void scale_set(float x, float y, float z = 1.0f) noexcept {
    e[0] = x;
    e[5] = y;
    e[10] = z;
    e[15] = 1.0f;
  }

  inline void multiply_set(const Matrix4 &a) noexcept {

    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 4; ++row) {
        float sum = 0.0f;
        for (int k = 0; k < 4; ++k) {
          sum += a.e[row + k * 4] * e[k + col * 4];
        }
        e[row + col * 4] = sum;
      }
    }
  }

};