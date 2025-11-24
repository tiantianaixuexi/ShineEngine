#pragma once

#ifndef SHINE_IMPORT_STD
#include <type_traits>
#include <concepts>
#include <string_view>
#include <array>
#include <cmath>
#include <tuple>
#include <utility>
#else
// WASM缂栬瘧鏃朵笉浣跨敤妯″潡
#endif


#ifndef KINDA_SMALL_NUMBER
#define KINDA_SMALL_NUMBER 1.e-4f
#endif

#ifndef KINDA_SMALL_NUMBER_DOUBLE
#define KINDA_SMALL_NUMBER_DOUBLE 1.e-8
#endif


#ifndef SHINE_CONCEPTS_DEFINED
#define SHINE_CONCEPTS_DEFINED

template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

#endif

template <typename T>
constexpr T Abs(T value) {
    return value < T(0) ? -value : value;
}

template <typename T>
constexpr T Fmod(T x, T y) {
    return x - std::floor(x / y) * y;
}

