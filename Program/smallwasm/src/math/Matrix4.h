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
    float c = cos(radians);
    float s = sin(radians);

    Matrix4 m = identity();
    // column-major for Z rotation
    m.e[0] = c;
    m.e[4] = -s;
    m.e[1] = s;
    m.e[5] = c;
    return m;
  }

  static Matrix4 fromQuatScaleTranslation(float qx, float qy, float qz, float qw,
                                          float sx, float sy, float sz,
                                          float tx, float ty, float tz) {
    const float xx = qx * qx;
    const float yy = qy * qy;
    const float zz = qz * qz;
    const float xy = qx * qy;
    const float xz = qx * qz;
    const float yz = qy * qz;
    const float wx = qw * qx;
    const float wy = qw * qy;
    const float wz = qw * qz;

    Matrix4 m{};
    m.e[0] = (1.0f - 2.0f * (yy + zz)) * sx;
    m.e[1] = (2.0f * (xy + wz)) * sx;
    m.e[2] = (2.0f * (xz - wy)) * sx;
    m.e[3] = 0.0f;

    m.e[4] = (2.0f * (xy - wz)) * sy;
    m.e[5] = (1.0f - 2.0f * (xx + zz)) * sy;
    m.e[6] = (2.0f * (yz + wx)) * sy;
    m.e[7] = 0.0f;

    m.e[8] = (2.0f * (xz + wy)) * sz;
    m.e[9] = (2.0f * (yz - wx)) * sz;
    m.e[10]= (1.0f - 2.0f * (xx + yy)) * sz;
    m.e[11]= 0.0f;

    m.e[12]= tx;
    m.e[13]= ty;
    m.e[14]= tz;
    m.e[15]= 1.0f;
    return m;
  }

  static Matrix4 multiply(const Matrix4 &a, const Matrix4 &b) noexcept {
    Matrix4 r{};
    const float* ae = a.e;
    const float* be = b.e;
    float* re = r.e;

    re[0]  = ae[0] * be[0]  + ae[4] * be[1]  + ae[8]  * be[2]  + ae[12] * be[3];
    re[1]  = ae[1] * be[0]  + ae[5] * be[1]  + ae[9]  * be[2]  + ae[13] * be[3];
    re[2]  = ae[2] * be[0]  + ae[6] * be[1]  + ae[10] * be[2]  + ae[14] * be[3];
    re[3]  = ae[3] * be[0]  + ae[7] * be[1]  + ae[11] * be[2]  + ae[15] * be[3];

    re[4]  = ae[0] * be[4]  + ae[4] * be[5]  + ae[8]  * be[6]  + ae[12] * be[7];
    re[5]  = ae[1] * be[4]  + ae[5] * be[5]  + ae[9]  * be[6]  + ae[13] * be[7];
    re[6]  = ae[2] * be[4]  + ae[6] * be[5]  + ae[10] * be[6]  + ae[14] * be[7];
    re[7]  = ae[3] * be[4]  + ae[7] * be[5]  + ae[11] * be[6]  + ae[15] * be[7];

    re[8]  = ae[0] * be[8]  + ae[4] * be[9]  + ae[8]  * be[10] + ae[12] * be[11];
    re[9]  = ae[1] * be[8]  + ae[5] * be[9]  + ae[9]  * be[10] + ae[13] * be[11];
    re[10] = ae[2] * be[8]  + ae[6] * be[9]  + ae[10] * be[10] + ae[14] * be[11];
    re[11] = ae[3] * be[8]  + ae[7] * be[9]  + ae[11] * be[10] + ae[15] * be[11];

    re[12] = ae[0] * be[12] + ae[4] * be[13] + ae[8]  * be[14] + ae[12] * be[15];
    re[13] = ae[1] * be[12] + ae[5] * be[13] + ae[9]  * be[14] + ae[13] * be[15];
    re[14] = ae[2] * be[12] + ae[6] * be[13] + ae[10] * be[14] + ae[14] * be[15];
    re[15] = ae[3] * be[12] + ae[7] * be[13] + ae[11] * be[14] + ae[15] * be[15];
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
    float c = cos(radians);
    float s = sin(radians);
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
    float be[16];
    for (int i = 0; i < 16; ++i) be[i] = e[i];

    const float* ae = a.e;
    float* re = e;

    re[0]  = ae[0] * be[0]  + ae[4] * be[1]  + ae[8]  * be[2]  + ae[12] * be[3];
    re[1]  = ae[1] * be[0]  + ae[5] * be[1]  + ae[9]  * be[2]  + ae[13] * be[3];
    re[2]  = ae[2] * be[0]  + ae[6] * be[1]  + ae[10] * be[2]  + ae[14] * be[3];
    re[3]  = ae[3] * be[0]  + ae[7] * be[1]  + ae[11] * be[2]  + ae[15] * be[3];

    re[4]  = ae[0] * be[4]  + ae[4] * be[5]  + ae[8]  * be[6]  + ae[12] * be[7];
    re[5]  = ae[1] * be[4]  + ae[5] * be[5]  + ae[9]  * be[6]  + ae[13] * be[7];
    re[6]  = ae[2] * be[4]  + ae[6] * be[5]  + ae[10] * be[6]  + ae[14] * be[7];
    re[7]  = ae[3] * be[4]  + ae[7] * be[5]  + ae[11] * be[6]  + ae[15] * be[7];

    re[8]  = ae[0] * be[8]  + ae[4] * be[9]  + ae[8]  * be[10] + ae[12] * be[11];
    re[9]  = ae[1] * be[8]  + ae[5] * be[9]  + ae[9]  * be[10] + ae[13] * be[11];
    re[10] = ae[2] * be[8]  + ae[6] * be[9]  + ae[10] * be[10] + ae[14] * be[11];
    re[11] = ae[3] * be[8]  + ae[7] * be[9]  + ae[11] * be[10] + ae[15] * be[11];

    re[12] = ae[0] * be[12] + ae[4] * be[13] + ae[8]  * be[14] + ae[12] * be[15];
    re[13] = ae[1] * be[12] + ae[5] * be[13] + ae[9]  * be[14] + ae[13] * be[15];
    re[14] = ae[2] * be[12] + ae[6] * be[13] + ae[10] * be[14] + ae[14] * be[15];
    re[15] = ae[3] * be[12] + ae[7] * be[13] + ae[11] * be[14] + ae[15] * be[15];
  }

};