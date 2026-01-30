#pragma once


#include <array>



template <typename...> 
constexpr bool always_false_v = false;;

template <typename T> struct ct_capacity_fail {
static_assert(always_false_v<std::remove_cvref_t<T>>,
              "Type does not support compile-time capacity");
};

template <typename T>
constexpr auto ct_capacity_v = ct_capacity_fail<T>{};

template <typename T, size_t N>
constexpr auto ct_capacity_v<std::array<T, N>> = N;


template <typename>
constexpr auto is_ct_v = false;

template <typename T, T V>
constexpr auto is_ct_v<std::integral_constant<T, V>> = true;

template <typename T>
constexpr auto is_ct_v<std::type_identity<T>> = true;

template <typename T>
constexpr auto is_ct_v<T const> = is_ct_v<T>;

