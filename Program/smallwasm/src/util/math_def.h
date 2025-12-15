#pragma once



const float pi = 3.14159265358979323846f;
const float two_pi = 6.28318530717958647692f;

namespace shine::math
{
    float wrap_pi(float a) {
        while (a > pi)
            a -= two_pi;
        while (a < -pi)
            a += two_pi;
        return a;
    }

    // abs
    static float f_abs(float x) { return x < 0.0f ? -x : x; }


    static float frac(float x) {
        int i = (int)x;
        float f = x - (float)i;
        if (f < 0.0f) f += 1.0f;
        return f;
    }

    static float tri_wave(float x) {
        float f = frac(x);
        float t = (f < 0.5f) ? (f * 2.0f) : ((1.0f - f) * 2.0f);
        return t * 2.0f - 1.0f;
    }

    static float tri01(float x) {
        // map tri_wave [-1..1] to [0..1]
        return tri_wave(x) * 0.5f + 0.5f;
    }

    static float sin_approx(float x) {
        // sin(x) ~ x - x^3/6 + x^5/120
        x = wrap_pi(x);
        float x2 = x * x;
        return x * (1.0f - x2 * (1.0f / 6.0f) + x2 * x2 * (1.0f / 120.0f));
    }
    
    static float cos_approx(float x) {
        // cos(x) ~ 1 - x^2/2 + x^4/24 - x^6/720
        x = wrap_pi(x);
        float x2 = x * x;
        float x4 = x2 * x2;
        float x6 = x4 * x2;
        return 1.0f - x2 * 0.5f + x4 * (1.0f / 24.0f) - x6 * (1.0f / 720.0f);
    }
}