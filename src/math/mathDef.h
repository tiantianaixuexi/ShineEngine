#pragma once

#include <concepts>
#include <type_traits>
#include <numbers>
#include <cmath>
#include <cstdint>


// 数学常量（使用constexpr，更现代的方式）
constexpr float SMALL_NUMBER        = 1.e-8f;
constexpr float KINDA_SMALL_NUMBER  = 1.e-4f;

constexpr double DOUBLE_SMALL_NUMBER        = 1.e-8;
constexpr double DOUBLE_KINDA_SMALL_NUMBER = 1.e-4;

// 数学模块专用的浮点数概念（只接受float和double）
template <typename T>
concept FloatingPoint = std::same_as<T, float> || std::same_as<T, double>;

// 通用整数概念
template <typename T>
concept Integral = std::is_integral_v<T>;

// 通用算术类型概念
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

// 数学模块内部使用的概念
template <typename T>
concept IntPoint = std::is_integral_v<T>;

template <typename T>
concept MathPoint = FloatingPoint<T> || IntPoint<T>;

// C++23 数学常量
namespace shine::math::constants {
    template<FloatingPoint T>
    constexpr T pi = std::numbers::pi_v<T>;
    
    template<FloatingPoint T>
    constexpr T half_pi = std::numbers::pi_v<T> / T(2);
    
    template<FloatingPoint T>
    constexpr T two_pi = std::numbers::pi_v<T> * T(2);
    
    template<FloatingPoint T>
    constexpr T deg_to_rad = std::numbers::pi_v<T> / T(180);
    
    template<FloatingPoint T>
    constexpr T rad_to_deg = T(180) / std::numbers::pi_v<T>;
}


namespace shine::math
{
    // 快速平方根倒数（Fast Inverse Square Root）
    // 使用 Quake III 算法优化，但在编译时常量时使用标准库
    [[nodiscard]] inline  float FastInvSqrt(float x) noexcept
    {

        // 运行时使用快速算法（WASM 兼容）
        #ifdef __EMSCRIPTEN__
            // WASM 环境下使用标准实现，避免平台特定代码
            return 1.0f / std::sqrtf(x);
        #else
            // 使用快速平方根倒数算法
            union {
                float f;
                std::uint32_t i;
            } conv = { .f = x };
            
            conv.i = 0x5f3759df - (conv.i >> 1);
            conv.f *= 1.5f - (x * 0.5f * conv.f * conv.f);
            
            return conv.f;
        #endif
    }

    [[nodiscard]] inline  double FastInvSqrt(double x) noexcept
    {

        
        #ifdef __EMSCRIPTEN__
            return 1.0 / std::sqrt(x);
        #else
            union {
                double d;
                std::uint64_t i;
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
}

