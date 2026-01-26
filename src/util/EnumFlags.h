#pragma once

#include <cstdint>
#include <type_traits>


template <typename E>
inline constexpr bool EnumFlagsEnabled = false;

#define ENABLE_ENUM_FLAGS(EnumType) \
    template <>                     \
    inline constexpr bool EnumFlagsEnabled<EnumType> = true;

template <typename E>
concept EnumFlags =
    std::is_enum_v<E> &&
    EnumFlagsEnabled<E>;

template <typename E>
constexpr auto ToUnderlying(E e) noexcept {
    static_assert(std::is_enum_v<E>);
    return static_cast<std::underlying_type_t<E>>(e);
}

template <EnumFlags E>
constexpr E operator|(E a, E b) noexcept {
    return static_cast<E>(ToUnderlying(a) | ToUnderlying(b));
}

template <EnumFlags E>
constexpr E operator&(E a, E b) noexcept {
    return static_cast<E>(ToUnderlying(a) & ToUnderlying(b));
}

template <EnumFlags E>
constexpr E operator^(E a, E b) noexcept {
    return static_cast<E>(ToUnderlying(a) ^ ToUnderlying(b));
}

template <EnumFlags E>
constexpr E operator~(E a) noexcept {
    return static_cast<E>(~ToUnderlying(a));
}

template <EnumFlags E>
constexpr E &operator|=(E &a, E b) noexcept {
    return a = a | b;
}

template <EnumFlags E>
constexpr E &operator&=(E &a, E b) noexcept {
    return a = a & b;
}

template <EnumFlags E>
constexpr E &operator^=(E &a, E b) noexcept {
    return a = a ^ b;
}

template <EnumFlags E>
constexpr bool HasFlag(E value, E flag) noexcept {
    return (value & flag) == flag;
}

template <EnumFlags E>
constexpr bool HasAnyFlag(E value, E flag) noexcept {
    return (ToUnderlying(value) & ToUnderlying(flag)) != 0;
}
