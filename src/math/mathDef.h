#pragma once


#include <type_traits>



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

