#pragma once

#include "wasm_compat.h"

SHINE_INLINE_VAR constexpr float pi = 3.14159265358979323846f;
SHINE_INLINE_VAR constexpr float two_pi = 6.28318530717958647692f;


// sin 余弦
// cos 正弦
// abs 绝对值
// min 最小值
// max 最大值
// fma  a*b+c
// flor 向下取整
// ceil 向上取整
// sqrt 平方根


// frac 小数部分

#define sin(x) __builtin_elementwise_sin(x)
#define tan(x) __builtin_elementwise_tan(x)
#define cos(x) __builtin_elementwise_cos(x)
#define abs(x) __builtin_elementwise_abs(x)
#define fabs(x) __builtin_fabsf(x)
#define min(x,y) __builtin_elementwise_min(x,y)
#define max(x,y) __builtin_elementwise_max(x,y)
#define fma(a,b,c) __builtin_elementwise_fma(a,b,c)

#define floor(x) __builtin_elementwise_floor(x)
#define ceil(x) __builtin_elementwise_ceil(x)
#define sqrt(x) __builtin_elementwise_sqrt(x)
#define frac(x) ((x) - __builtin_elementwise_floor(x))

#define tri_wave(x) (1.0f - fabs(frac(x) - 0.5f) * 4.0f)

#define tri(x) fma(tri_wave(x),0.5f,0.5f)

namespace shine::math
{



    // inline float tri_wave(float x) {
    //     float f = frac(x);
    //     float t = fabs( f - 0.5f );
    //     return 1.0f - t * 4.0f;
    // }

    // inline float tri01(float x) {
    //     // map tri_wave [-1..1] to [0..1]
    //     return fma(tri_wave(x), 0.5f, 0.5f);
    // }



}
