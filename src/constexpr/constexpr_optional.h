#pragma once

#include <utility>
#include <type_traits>
#include <concepts>
#include <optional>
#include <functional>
#include <compare>
#include <new>

namespace shine {
namespace constexpr_ {

// ============================================================
// constexpr_optional - 编译期可选值
// 
// 特性：
// - 完全 constexpr 支持
// - 单调(monadic)操作支持
// - 与 std::optional 互转
// - 引用类型支持
// ============================================================

// 无值标记
struct nullopt_t {
    explicit constexpr nullopt_t(int) {}
};

inline constexpr nullopt_t nullopt{0};

// 就地构造标记
using in_place_t = std::in_place_t;
inline constexpr in_place_t in_place{};

// ============================================================
// constexpr_optional 主模板
// ============================================================
template <typename T>
class constexpr_optional {
public:
    using value_type = T;
    using self_type = constexpr_optional<T>;

private:
    alignas(alignof(T)) std::byte storage_[sizeof(T)];
    bool has_value_ = false;

    constexpr T* ptr() noexcept {
        return std::launder(reinterpret_cast<T*>(storage_));
    }

    constexpr T const* ptr() const noexcept {
        return std::launder(reinterpret_cast<T const*>(storage_));
    }

public:
    // ==================== 构造函数 ====================

    // 默认构造（空）
    constexpr constexpr_optional() noexcept : has_value_(false) {}

    // 空值构造
    constexpr constexpr_optional(nullopt_t) noexcept : has_value_(false) {}

    // 值构造
    template <typename U = T>
        requires std::constructible_from<T, U&&>
    constexpr explicit(!std::convertible_to<U&&, T>)
    constexpr_optional(U&& value)
        : has_value_(true) {
        ::new (storage_) T(std::forward<U>(value));
    }

    // 拷贝构造
    constexpr constexpr_optional(constexpr_optional const& other)
        requires std::copy_constructible<T>
        : has_value_(other.has_value_) {
        if (has_value_) {
            ::new (storage_) T(*other);
        }
    }

    // 移动构造
    constexpr constexpr_optional(constexpr_optional&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>
    ) : has_value_(other.has_value_) {
        if (has_value_) {
            ::new (storage_) T(std::move(*other));
        }
    }

    // 就地构造
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr explicit constexpr_optional(in_place_t, Args&&... args)
        : has_value_(true) {
        ::new (storage_) T(std::forward<Args>(args)...);
    }

    // 从 std::optional 构造
    constexpr constexpr_optional(std::optional<T> const& other)
        requires std::copy_constructible<T>
        : has_value_(other.has_value()) {
        if (has_value_) {
            ::new (storage_) T(*other);
        }
    }

    constexpr constexpr_optional(std::optional<T>&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>
    ) : has_value_(other.has_value()) {
        if (has_value_) {
            ::new (storage_) T(std::move(*other));
        }
    }

    // 条件构造
    template <typename U = T>
        requires std::constructible_from<T, U&&>
    constexpr explicit constexpr_optional(bool condition, U&& value)
        : has_value_(condition) {
        if (has_value_) {
            ::new (storage_) T(std::forward<U>(value));
        }
    }

    // 析构函数
    constexpr ~constexpr_optional() {
        if (has_value_) {
            std::destroy_at(ptr());
        }
    }

    // ==================== 赋值运算符 ====================

    constexpr constexpr_optional& operator=(nullopt_t) noexcept {
        reset();
        return *this;
    }

    constexpr constexpr_optional& operator=(constexpr_optional const& other)
        requires std::copy_constructible<T> && std::copy_assignable<T> {
        if (has_value_ && other.has_value_) {
            **this = *other;
        } else if (other.has_value_) {
            emplace(*other);
        } else {
            reset();
        }
        has_value_ = other.has_value_;
        return *this;
    }

    constexpr constexpr_optional& operator=(constexpr_optional&& other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>
    ) requires std::move_constructible<T> && std::move_assignable<T> {
        if (has_value_ && other.has_value_) {
            **this = std::move(*other);
        } else if (other.has_value_) {
            emplace(std::move(*other));
        } else {
            reset();
        }
        has_value_ = other.has_value_;
        return *this;
    }

    template <typename U = T>
        requires std::constructible_from<T, U&&> && std::assignable_from<T&, U&&>
    constexpr constexpr_optional& operator=(U&& value) {
        if (has_value_) {
            **this = std::forward<U>(value);
        } else {
            emplace(std::forward<U>(value));
            has_value_ = true;
        }
        return *this;
    }

    // ==================== 状态查询 ====================

    constexpr explicit operator bool() const noexcept { return has_value_; }
    constexpr bool has_value() const noexcept { return has_value_; }

    // ==================== 值访问 ====================

    constexpr T& operator*() & noexcept {
        return *ptr();
    }

    constexpr T const& operator*() const& noexcept {
        return *ptr();
    }

    constexpr T&& operator*() && noexcept {
        return std::move(*ptr());
    }

    constexpr T const&& operator*() const&& noexcept {
        return std::move(*ptr());
    }

    constexpr T* operator->() noexcept { return ptr(); }
    constexpr T const* operator->() const noexcept { return ptr(); }

    // 带检查访问
    constexpr T& value() & {
        if (!has_value_) {
            throw std::bad_optional_access{};
        }
        return **this;
    }

    constexpr T const& value() const& {
        if (!has_value_) {
            throw std::bad_optional_access{};
        }
        return **this;
    }

    constexpr T&& value() && {
        if (!has_value_) {
            throw std::bad_optional_access{};
        }
        return std::move(**this);
    }

    // 默认值访问
    template <typename U>
        requires std::copy_constructible<T> && std::convertible_to<U&&, T>
    constexpr T value_or(U&& default_value) const& {
        return has_value_ ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
        requires std::move_constructible<T> && std::convertible_to<U&&, T>
    constexpr T value_or(U&& default_value) && {
        return has_value_ ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    // 默认值构造
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr T& emplace(Args&&... args) {
        if (has_value_) {
            std::destroy_at(ptr());
        }
        ::new (storage_) T(std::forward<Args>(args)...);
        has_value_ = true;
        return **this;
    }

    // ==================== 重置 ====================

    constexpr void reset() noexcept {
        if (has_value_) {
            std::destroy_at(ptr());
            has_value_ = false;
        }
    }

    // ==================== 单调操作 (Monadic operations) ====================

    // map: 转换值
    template <typename F>
        requires std::invocable<F, T const&> && std::copy_constructible<T>
    constexpr auto map(F&& f) const& -> constexpr_optional<std::invoke_result_t<F, T const&>> {
        using result_type = std::invoke_result_t<F, T const&>;
        if (has_value_) {
            return constexpr_optional<result_type>(std::invoke(std::forward<F>(f), **this));
        }
        return constexpr_optional<result_type>{};
    }

    template <typename F>
        requires std::invocable<F, T&&> && std::move_constructible<T>
    constexpr auto map(F&& f) && -> constexpr_optional<std::invoke_result_t<F, T&&>> {
        using result_type = std::invoke_result_t<F, T&&>;
        if (has_value_) {
            return constexpr_optional<result_type>(std::invoke(std::forward<F>(f), std::move(**this)));
        }
        return constexpr_optional<result_type>{};
    }

    // and_then: 链式操作
    template <typename F>
        requires std::invocable<F, T const&> && std::copy_constructible<T>
    constexpr auto and_then(F&& f) const& -> std::invoke_result_t<F, T const&> {
        using result_type = std::invoke_result_t<F, T const&>;
        if (has_value_) {
            return std::invoke(std::forward<F>(f), **this);
        }
        return result_type{};
    }

    template <typename F>
        requires std::invocable<F, T&&> && std::move_constructible<T>
    constexpr auto and_then(F&& f) && -> std::invoke_result_t<F, T&&> {
        using result_type = std::invoke_result_t<F, T&&>;
        if (has_value_) {
            return std::invoke(std::forward<F>(f), std::move(**this));
        }
        return result_type{};
    }

    // or_else: 提供替代值
    template <typename F>
        requires std::copy_constructible<T> && std::invocable<F>
    constexpr constexpr_optional or_else(F&& f) const& {
        return has_value_ ? *this : std::invoke(std::forward<F>(f));
    }

    template <typename F>
        requires std::move_constructible<T> && std::invocable<F>
    constexpr constexpr_optional or_else(F&& f) && {
        return has_value_ ? std::move(*this) : std::invoke(std::forward<F>(f));
    }

    // transform: C++23 风格
    template <typename F>
        requires std::invocable<F, T&>
    constexpr auto transform(F&& f) & -> constexpr_optional<std::invoke_result_t<F, T&>> {
        using result_type = std::invoke_result_t<F, T&>;
        if (has_value_) {
            return constexpr_optional<result_type>(std::invoke(std::forward<F>(f), **this));
        }
        return {};
    }

    template <typename F>
        requires std::invocable<F, T const&>
    constexpr auto transform(F&& f) const& -> constexpr_optional<std::invoke_result_t<F, T const&>> {
        using result_type = std::invoke_result_t<F, T const&>;
        if (has_value_) {
            return constexpr_optional<result_type>(std::invoke(std::forward<F>(f), **this));
        }
        return {};
    }

    template <typename F>
        requires std::invocable<F, T>
    constexpr auto transform(F&& f) && -> constexpr_optional<std::invoke_result_t<F, T>> {
        using result_type = std::invoke_result_t<F, T>;
        if (has_value_) {
            return constexpr_optional<result_type>(std::invoke(std::forward<F>(f), std::move(**this)));
        }
        return {};
    }

    // ==================== 转换操作 ====================

    // 转换为 std::optional
    constexpr std::optional<T> to_optional() const& 
        requires std::copy_constructible<T> {
        return has_value_ ? std::optional<T>(**this) : std::nullopt;
    }

    constexpr std::optional<T> to_optional() && 
        requires std::move_constructible<T> {
        return has_value_ ? std::optional<T>(std::move(**this)) : std::nullopt;
    }

    // ==================== 比较运算符 ====================

    constexpr bool operator==(constexpr_optional const& other) const
        requires std::equality_comparable<T> {
        return (has_value_ == other.has_value_) && (!has_value_ || **this == *other);
    }

    constexpr auto operator<=>(constexpr_optional const& other) const
        requires std::three_way_comparable<T> {
        if (has_value_ != other.has_value_) {
            return has_value_ <=> other.has_value_;
        }
        if (!has_value_) {
            return std::strong_ordering::equal;
        }
        return **this <=> *other;
    }

    // 与 nullopt 比较
    constexpr bool operator==(nullopt_t) const noexcept { return !has_value_; }
    constexpr auto operator<=>(nullopt_t) const noexcept {
        return has_value_ <=> false;
    }

    // 与值比较
    template <typename U>
        requires std::equality_comparable_with<T, U>
    constexpr bool operator==(U const& value) const {
        return has_value_ && **this == value;
    }

    // ==================== 交换 ====================

    constexpr void swap(constexpr_optional& other) noexcept(
        std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>
    ) {
        if (has_value_ && other.has_value_) {
            using std::swap;
            swap(**this, *other);
        } else if (has_value_) {
            other.emplace(std::move(**this));
            reset();
        } else if (other.has_value_) {
            emplace(std::move(*other));
            other.reset();
        }
    }
};

// ============================================================
// CTAD 推导指引
// ============================================================

template <typename T>
constexpr_optional(T) -> constexpr_optional<T>;

// ============================================================
// 非成员函数
// ============================================================

// make_optional
template <typename T>
constexpr constexpr_optional<std::decay_t<T>> make_optional(T&& value) {
    return constexpr_optional<std::decay_t<T>>(std::forward<T>(value));
}

template <typename T, typename... Args>
constexpr constexpr_optional<T> make_optional(Args&&... args) {
    return constexpr_optional<T>(in_place, std::forward<Args>(args)...);
}

// swap
template <typename T>
constexpr void swap(constexpr_optional<T>& lhs, constexpr_optional<T>& rhs) noexcept(
    noexcept(lhs.swap(rhs))
) {
    lhs.swap(rhs);
}

// ============================================================
// 引用特化
// ============================================================
template <typename T>
class constexpr_optional<T&> {
public:
    using value_type = T&;
    using self_type = constexpr_optional<T&>;

private:
    T* ptr_ = nullptr;

public:
    constexpr constexpr_optional() noexcept = default;
    constexpr constexpr_optional(nullopt_t) noexcept : ptr_(nullptr) {}

    constexpr explicit constexpr_optional(T& ref) noexcept : ptr_(std::addressof(ref)) {}
    constexpr explicit constexpr_optional(T&&) = delete;  // 禁止绑定临时对象

    template <typename U>
        requires std::convertible_to<U&, T&>
    constexpr constexpr_optional(constexpr_optional<U&> const& other) noexcept
        : ptr_(other ? std::addressof(*other) : nullptr) {}

    constexpr constexpr_optional(constexpr_optional const&) = default;
    constexpr constexpr_optional& operator=(constexpr_optional const&) = default;

    constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
    constexpr bool has_value() const noexcept { return ptr_ != nullptr; }

    constexpr T& operator*() const noexcept { return *ptr_; }
    constexpr T* operator->() const noexcept { return ptr_; }

    constexpr T& value() const {
        if (!ptr_) throw std::bad_optional_access{};
        return *ptr_;
    }

    template <typename U>
    constexpr T value_or(U&& default_value) const {
        return ptr_ ? *ptr_ : static_cast<T>(std::forward<U>(default_value));
    }

    constexpr void reset() noexcept { ptr_ = nullptr; }

    constexpr bool operator==(constexpr_optional const& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    constexpr auto operator<=>(constexpr_optional const& other) const noexcept {
        return ptr_ <=> other.ptr_;
    }

    constexpr bool operator==(nullopt_t) const noexcept { return ptr_ == nullptr; }
};

} // namespace constexpr_

// ============================================================
// 简化别名
// ============================================================

template <typename T>
using ct_optional = constexpr_::constexpr_optional<T>;

using constexpr_::nullopt;
using constexpr_::nullopt_t;

} // namespace shine

// ============================================================
// 标准库特化
// ============================================================

template <typename T>
struct std::hash<shine::constexpr_::constexpr_optional<T>> {
    constexpr std::size_t operator()(shine::constexpr_::constexpr_optional<T> const& opt) const noexcept {
        return opt ? std::hash<T>{}(*opt) : 0;
    }
};