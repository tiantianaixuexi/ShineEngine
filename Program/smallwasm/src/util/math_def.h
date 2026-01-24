#pragma once



inline constexpr float pi = 3.14159265358979323846f;
inline constexpr float two_pi = 6.28318530717958647692f;

#define sin(x) __builtin_elementwise_sin(x)
#define cos(x) __builtin_elementwise_cos(x)
#define abs(x) __builtin_elementwise_abs(x)

namespace shine::math
{



    inline float frac(float x) {
        int i = (int)x;
        float f = x - (float)i;
        if (f < 0.0f) f += 1.0f;
        return f;
    }

    inline float tri_wave(float x) {
        float f = frac(x);
        float t = (f < 0.5f) ? (f * 2.0f) : ((1.0f - f) * 2.0f);
        return t * 2.0f - 1.0f;
    }

    inline float tri01(float x) {
        // map tri_wave [-1..1] to [0..1]
        return tri_wave(x) * 0.5f + 0.5f;
    }



}
