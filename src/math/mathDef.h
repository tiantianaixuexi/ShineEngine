#pragma once

#include "shine_define.h"
#include <type_traits>
#include <cmath>



// 数学模块专用的浮点数概念（只接受float和double）
template <typename T>
concept FloatingPoint = std::is_same_v<T, float> || std::is_same_v<T, double>;

// 通用整数概念
template <typename T>
concept Integral = std::is_integral_v<T>;

// 通用算术类型概念
template <typename T>
concept IsNumber = std::is_arithmetic_v<T>;


// C++23 数学常量
namespace shine::math::constants {
    
    template<FloatingPoint T>
    constexpr T pi = static_cast<T>(3.141592653589793);
    
    template<FloatingPoint T>
    constexpr T half_pi = pi<T> / T(2);
    
    template<FloatingPoint T>
    constexpr T two_pi = pi<T> * T(2);
    
    template<FloatingPoint T>
    constexpr T deg_to_rad = pi<T> / T(180);
    
    template<FloatingPoint T>
    constexpr T rad_to_deg = T(180) / pi<T>;
}

SHINE_MODULE_EXPORT namespace shine::math {


    // 数学常量（使用constexpr，更现代的方式）


    constexpr float  PI_F = constants::pi<float>;
	constexpr double PI_D = constants::pi<double>;
	constexpr float  TWO_PI_F = constants::two_pi<float>;
	constexpr double TWO_PI_D = constants::two_pi<double>;
	constexpr float  HALF_PI_F = constants::half_pi<float>;
	constexpr double HALF_PI_D = constants::half_pi<double>;
	constexpr float  DEG_TO_RAD_F = constants::deg_to_rad<float>;
	constexpr double DEG_TO_RAD_D = constants::deg_to_rad<double>;
	constexpr float  RAD_TO_DEG_F = constants::rad_to_deg<float>;
	constexpr double RAD_TO_DEG_D = constants::rad_to_deg<double>;


    constexpr float  SMALL_NUMBER = 1.e-8f;
    constexpr float  KINDA_SMALL_NUMBER = 1.e-4f;

    constexpr double DOUBLE_SMALL_NUMBER = 1.e-8;
    constexpr double DOUBLE_KINDA_SMALL_NUMBER = 1.e-4;


    template<class T>
    T Abs(const T& A)
    {
        return ( A < (T)0 ) ?  -A : A;
    }

    template<class T>
    constexpr T Max(const T &A , const T &B)
    {
        return A > B ? A : B;
    }

    template<class T>
    constexpr T Min(const T &A , const T &B)
    {
        return A < B ? A : B;
    }

    template<class T>
    constexpr T Clamp(const T &A , const T &Min , const T &Max)
    {
        return A < Min ? Min : ( A > Max ? Max : A );
    }

    template<class T>
    constexpr T Sign(const T &A)
    {
        return ( A > static_cast<T>(0) ) ? static_cast<T>(1) : ( A < static_cast<T>(0) ? static_cast<T>(-1) : static_cast<T>(0) );
    }

    inline float Fmod(float A, float B)
    {
        const float absY = Abs(B);
        if (absY < SMALL_NUMBER) {
            return 0.0f; // 避免除以零
        }
        return fmodf(A, B);
    }

    inline double Fmod(double A, double B){
        const double absY = Abs(B);
        if (absY < DOUBLE_KINDA_SMALL_NUMBER) {
            return 0.0; // 避免除以零
        }
        return fmod(A, B);
    }


    // 角度转弧度
    [[nodiscard]] constexpr float radians(float degrees) noexcept {
        return degrees * 0.01745329251f;
    }

    [[nodiscard]] constexpr double radians(double degrees) noexcept {
        return degrees * 0.017453292519943295769;
    }

    // 弧度转角度
    [[nodiscard]] constexpr float degrees(float radians) noexcept {
        return radians * 57.295779513082320876f;
    }

    [[nodiscard]] constexpr double degrees(double radians) noexcept {
        return radians * 57.295779513082320876;
    }

    // 线性插值
    template<FloatingPoint T>
    [[nodiscard]] constexpr T lerp(T a, T b, T t) noexcept {
        t = Clamp(t, static_cast<T>(0), static_cast<T>(1));
        return a + (b - a) * t;
    }

    // 平滑插值（smoothstep）
    template<FloatingPoint T>
    [[nodiscard]] constexpr T smoothstep(T edge0, T edge1, T x) noexcept {
        x = Clamp((x - edge0) / (edge1 - edge0), static_cast<T>(0), static_cast<T>(1));
        return x * x * (static_cast<T>(3) - static_cast<T>(2) * x);
    }

    // 更平滑的插值（smootherstep）
    template<FloatingPoint T>
    [[nodiscard]] constexpr T smootherstep(T edge0, T edge1, T x) noexcept {
        x = Clamp((x - edge0) / (edge1 - edge0), static_cast<T>(0), static_cast<T>(1));
        return x * x * x * (x * (x * static_cast<T>(6) - static_cast<T>(15)) + static_cast<T>(10));
    }

    // 重映射值
    template<FloatingPoint T>
    [[nodiscard]] constexpr T remap(T value, T inMin, T inMax, T outMin, T outMax) noexcept {
        return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
    }

    // 检查值是否在范围内
    template<IsNumber T>
    [[nodiscard]] constexpr bool isInRange(T value, T min, T max) noexcept {
        return value >= min && value <= max;
    }

    // 将角度限制在 [0, 360) 度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T wrapAngleDegrees(T angle) noexcept {
        angle = Fmod(angle, static_cast<T>(360));
        if (angle < static_cast<T>(0)) {
            angle += static_cast<T>(360);
        }
        return angle;
    }

    // 将角度限制在 [-180, 180) 度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T normalizeAngleDegrees(T angle) noexcept {
        angle = wrapAngleDegrees(angle);
        if (angle > static_cast<T>(180)) {
            angle -= static_cast<T>(360);
        }
        return angle;
    }

    // 将角度限制在 [0, 2π) 弧度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T wrapAngleRadians(T angle) noexcept {
        constexpr T twoPi = constants::two_pi<T>;
        angle = Fmod(angle, twoPi);
        if (angle < static_cast<T>(0)) {
            angle += twoPi;
        }
        return angle;
    }

    // 将角度限制在 [-π, π) 弧度
    template<FloatingPoint T>
    [[nodiscard]] constexpr T normalizeAngleRadians(T angle) noexcept {
        constexpr T pi = constants::pi<T>;
        angle = wrapAngleRadians(angle);
        if (angle > pi) {
            angle -= constants::two_pi<T>;
        }
        return angle;
    }

    // 检查浮点数是否接近零
    template<FloatingPoint T>
    [[nodiscard]] constexpr bool isNearlyZero(T value, T tolerance = SMALL_NUMBER) noexcept {
        return Abs(value) <= tolerance;
    }

    // 检查两个浮点数是否接近相等
    template<FloatingPoint T>
    [[nodiscard]] constexpr bool isNearlyEqual(T a, T b, T tolerance = KINDA_SMALL_NUMBER) noexcept {
        return Abs(a - b) <= tolerance;
    }


}


namespace shine::math
{
    // 快速平方根倒数（Fast Inverse Square Root）
    // 使用 Quake III 算法优化，但在编译时常量时使用标准库
    [[nodiscard]] inline  float FastInvSqrt(float x) noexcept
    {

#ifdef SHINE_PLATFORM_WASM
            // WASM 环境下使用标准实现，避免平台特定代码
            return 1.0f / std::sqrtf(x);
#else
            // 使用快速平方根倒数算法
            union {
                float f;
                u32 i;
            } conv = { .f = x };
            
            conv.i = 0x5f3759df - (conv.i >> 1);
            conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
            
            return conv.f;
        #endif
    }

    [[nodiscard]] inline  double FastInvSqrt(double x) noexcept
    {
        
#ifdef SHINE_PLATFORM_WASM
            return 1.0 / std::sqrt(x);
#else
            union {
                double d;
                u64 i;
            } conv = { .d = x };
            
            conv.i = 0x5fe6eb50c7b537a9ULL - (conv.i >> 1);
            conv.d *= 1.5 - (x * 0.5 * conv.d * conv.d);
            
            return conv.d;
        #endif
    }

    [[nodiscard]] inline float Sqrt(float x) noexcept
    {
        return std::sqrtf(x);
    }

    [[nodiscard]] inline double Sqrt(double x) noexcept
    {
        return std::sqrt(x);
    }

    [[nodiscard]] inline float InvSqrt(float x) noexcept
    {
        return FastInvSqrt(x);
    }

    [[nodiscard]] inline double InvSqrt(double x) noexcept
    {
        return FastInvSqrt(x);
    }

	[[nodiscard]] inline float wrap_pi(float a) noexcept
    {
	    if (a > PI_F)
			a -= TWO_PI_F;

		if (a < -PI_F)
			a += TWO_PI_F;

		return a;
    }

    [[nodiscard]] inline float f_abs(float x) noexcept {
        return x < 0.0f ? -x : x;
    }

	[[nodiscard]] inline float sin(float x) noexcept {
        // sin(x) ~ x - x^3/6 + x^5/120
        x = wrap_pi(x);
        const float x2 = x * x;
        return x * (1.0f - x2 * (1.0f / 6.0f) + x2 * x2 * (1.0f / 120.0f));
	}

	[[nodiscard]] inline float cos(float x) noexcept {
        // cos(x) ~ 1 - x^2/2 + x^4/24 - x^6/720
        x = wrap_pi(x);
        const float x2 = x * x;
        return 1.0f - x2 * 0.5f + x2 * x2 * (1.0f / 24.0f) - x2 * x2 * x2 * (1.0f / 720.0f);
	}

	[[nodiscard]] inline float frac(float x) noexcept {
        int i = static_cast<int>(x);
        float f = x - static_cast<float>(i);
        if (f < 0.0f) f += 1.0f;
        return f;
	}

    [[nodiscard]] inline float tri_wave(float x) noexcept {
        float f = frac(x);
        float t = (f < 0.5f) ? (f * 2.0f) : ((1.0f - f) * 2.0f);
        return t * 2.0f - 1.0f;
	}

    [[nodiscard]] inline float tri01(float x) noexcept {
        // map tri_wave [-1..1] to [0..1]
        return tri_wave(x) * 0.5f + 0.5f;
	}
}

