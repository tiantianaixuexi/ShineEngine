#pragma once


#include <array>

namespace shine
{


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


}