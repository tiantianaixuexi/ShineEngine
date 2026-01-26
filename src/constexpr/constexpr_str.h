#pragma once

#include <array>
#include <string_view>

namespace shine {
namespace constexpr_ {

template <size_t N>
struct constexpr_str {

    template <typename T>
    concept format_convertible = requires(T t) {
        { T::constexpr_string_convertible() } -> std::same_as<std::true_type>;
        { constexpr_str{+t} };
    };

    consteval constexpr_str() = default;

    consteval explicit(false) constexpr_str(char const (&str)[N]) {
        for (size_t i = 0; i < N; ++i) {
            values[i] = str[i];
        }
    }

    consteval explicit(true) constexpr_str(char const *str, size_t sz) {
        for (size_t i = 0; i < N; ++i) {
            values[i] = str[i];
        }
    }


    template <format_convertible T>
    consteval explicit(false) constexpr_str(T t) : constexpr_str(+t){};

    consteval explicit(true) constexpr_str(std::string_view str)
        : constexpr_str{str.data(), str.size()} {}



private:
    std::array<char, N> values{};
};

} // namespace constexpr_
} // namespace shine