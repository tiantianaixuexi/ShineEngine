#pragma once

#include <array>
#include <string_view>
#include <utility>
#include <type_traits>
#include <iterator>

namespace shine {
namespace constexpr_ {

template <size_t N>
struct constexpr_str;

namespace detail {
template <typename T>
concept format_convertible = requires(T t) {
    { T::constexpr_string_convertible() } -> std::same_as<std::true_type>;
    { constexpr_str{+t} };
};
} // namespace detail

template <size_t N>
struct constexpr_str {
    static_assert(N >= 1, "constexpr_str size must be at least 1");

    consteval constexpr_str() = default;

    consteval explicit(false) constexpr_str(char const (&str)[N]) {
        for (size_t i = 0; i < N; ++i) {
            value[i] = str[i];
        }
    }

    consteval explicit(true) constexpr_str(char const *str, size_t sz) {
        for (size_t i = 0; i < N && i < sz; ++i) {
            value[i] = str[i];
        }
    }

    template <detail::format_convertible T>
    consteval explicit(false) constexpr_str(T t) : constexpr_str(+t) {
    }

    consteval explicit(true) constexpr_str(std::string_view str)
        : constexpr_str(str.data(), str.size()) {
    }

    constexpr auto begin() { return value.begin(); }
    constexpr auto end() { return value.end() - 1; }
    constexpr auto begin() const { return value.begin(); }
    constexpr auto end() const { return value.end() - 1; }
    constexpr auto cbegin() const { return value.cbegin(); }
    constexpr auto cend() const { return value.cend() - 1; }
    constexpr auto rbegin() { return std::next(value.rbegin()); }
    constexpr auto rend() { return value.rend(); }
    constexpr auto rbegin() const { return std::next(value.rbegin()); }
    constexpr auto rend() const { return value.rend(); }
    constexpr auto crbegin() const { return std::next(value.crbegin()); }
    constexpr auto crend() const { return value.crend(); }

    constexpr static std::integral_constant<size_t, N> capacity{};
    constexpr static std::integral_constant<size_t, N - 1U> size{};
    constexpr static std::integral_constant<bool, N == 1U> empty{};

    constexpr explicit(true) operator std::string_view() const {
        return std::string_view(value.data(), size());
    }

    // 数据成员 - public 以支持外部函数访问
    std::array<char, N> value{};

private:
    // 友元声明 - 只声明操作符，不涉及类类型作为模板参数
    template <std::size_t N2, std::size_t M>
    friend constexpr auto operator==(constexpr_str<N2> const &lhs,
                                      constexpr_str<M> const &rhs) -> bool;

    template <std::size_t N2, std::size_t M>
    friend constexpr auto operator+(constexpr_str<N2> const &lhs,
                                     constexpr_str<M> const &rhs) -> constexpr_str<N2 + M - 1>;
};

template <detail::format_convertible T>
constexpr_str(T) -> constexpr_str<decltype(+std::declval<T>())::capacity()>;

template <std::size_t N, std::size_t M>
constexpr auto operator==(constexpr_str<N> const &lhs,
                           constexpr_str<M> const &rhs) -> bool {
    return static_cast<std::string_view>(lhs) ==
           static_cast<std::string_view>(rhs);
}

template <template <typename C, C...> typename T, char... Cs>
[[nodiscard]] consteval auto ct_string_from_type(T<char, Cs...>) {
    return constexpr_str<sizeof...(Cs) + 1U>{{Cs..., 0}};
}

// split 函数 - 定义在类外部，此时 constexpr_str 已完整定义
template <constexpr_str S, char C>
consteval auto split() {
    constexpr auto it = [] {
        for (auto i = S.value.cbegin(); i != S.value.cend(); ++i) {
            if (*i == C) {
                return i;
            }
        }
        return S.value.cend();
    }();
    if constexpr (it == S.value.cend()) {
        return std::pair{S, constexpr_str{""}};
    } else {
        constexpr auto prefix_size = static_cast<std::size_t>(it - S.value.cbegin());
        constexpr auto suffix_size = S.size() - prefix_size;
        return std::pair{
            constexpr_str<prefix_size + 1U>{&*S.value.cbegin(), prefix_size},
            constexpr_str<suffix_size>{&*(it + 1), suffix_size - 1U}};
    }
}

// constexpr_string_to_type - 定义在类外部
template <constexpr_str S, template <typename C, C...> typename T>
[[nodiscard]] consteval auto constexpr_string_to_type() {
    return [&]<auto... Is>(std::index_sequence<Is...>) {
        return T<char, std::get<Is>(S.value)...>{};
    }(std::make_index_sequence<S.size()>{});
}

template <constexpr_str S, template <typename C, C...> typename T>
using constexpr_string_to_type_t = decltype(constexpr_string_to_type<S, T>());

template <std::size_t N, std::size_t M>
constexpr auto operator+(constexpr_str<N> const &lhs, constexpr_str<M> const &rhs)
    -> constexpr_str<N + M - 1> {
    constexpr_str<N + M - 1> ret{};
    for (auto i = std::size_t{}; i < lhs.size(); ++i) {
        ret.value[i] = lhs.value[i];
    }
    for (auto i = std::size_t{}; i < rhs.size(); ++i) {
        ret.value[i + N - 1] = rhs.value[i];
    }
    return ret;
}

template <constexpr_str S>
struct cts_t {
    using value_type = decltype(S);
    constexpr static auto value = S;

    consteval static auto constexpr_string_convertible() -> std::true_type;
    friend constexpr auto operator+(cts_t const &) { return value; }
    constexpr auto operator()() const noexcept { return value; }
    using cx_value_t [[maybe_unused]] = void;
    constexpr static auto size = S.size;
};

template <constexpr_str X, constexpr_str Y>
constexpr auto operator==(cts_t<X>, cts_t<Y>) -> bool {
    return X == Y;
}

template <constexpr_str X, constexpr_str Y>
constexpr auto operator+(cts_t<X>, cts_t<Y>) {
    return cts_t<X + Y>{};
}

template <size_t N, constexpr_str S>
constexpr auto operator+(constexpr_str<N> const &lhs, cts_t<S> rhs) {
    return lhs + +rhs;
}

template <constexpr_str S, size_t N>
[[nodiscard]] constexpr auto operator+(cts_t<S> lhs, constexpr_str<N> const &rhs) {
    return +lhs + rhs;
}

namespace detail {
template <size_t N>
struct ct_helper {
    using type = constexpr_str<N>;
};
} // namespace detail

template <constexpr_str Value>
consteval auto ct() { return cts_t<Value>{}; }

inline namespace literals {
inline namespace ct_string_literals {
template <constexpr_str S>
consteval auto operator""_cts() { return S; }

template <constexpr_str S>
consteval auto operator""_ctst() {
    return cts_t<S>{};
}
} // namespace ct_string_literals
} // namespace literals

} // namespace constexpr_
} // namespace shine
