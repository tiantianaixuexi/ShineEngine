#pragma once


#include <concepts>
#include <cmath>

constexpr  float SMALL_NUMBER        = 1.e-8f;
constexpr  float KINDA_SMALL_NUMBER  = 1.e-4f;

constexpr  double DOUBLE_SMALL_NUMBER		=	1.e-8;
constexpr  double DOUBLE_KINDA_SMALL_NUMBER	=   1.e-4;
// 数学模块专用的浮点数概念（只接受float和double）
template <typename T>
concept FloatingPoint = std::same_as<T, float> || std::same_as<T, double>;

template <typename T>
concept IntPoint = std::is_integral_v<T>;

template <typename T>
concept MathPoint = FloatingPoint<T> || IntPoint<T>;

namespace shine::math
{


    inline float Sqrt(float x){
        return sqrtf(x);
    }

    inline double Sqrt(double x){
        return sqrt(x);
    }

    inline float InvSqrt(float x)
    {
        return 1.0f / Sqrt(x);
    }

    inline double InvSqrt(double x)
    {
        return 1.0 / Sqrt(x);
    }
};

