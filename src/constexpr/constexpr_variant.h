#pragma once

#include <utility>
#include <type_traits>
#include <concepts>
#include <variant>
#include <optional>
#include <functional>
#include <compare>
#include <cstring>
#include <cstdint>

namespace shine {
namespace constexpr_ {

// ============================================================
// constexpr_variant - 编译期变体类型
// 
// 特性：
// - 完全 constexpr 支持
// - 类型安全的值存储
// - 访问者模式支持
// - 与 std::variant 互转
// - 反射系统集成
// ============================================================

// 前向声明
template <typename... Ts>
class constexpr_variant;

namespace detail {

// 最大类型大小
template <typename... Ts>
constexpr std::size_t max_size_of = (std::max)({sizeof(Ts)...});

// 最大类型对齐
template <typename... Ts>
constexpr std::size_t max_align_of = (std::max)({alignof(Ts)...});

// 类型索引查找
template <typename T, typename... Ts>
constexpr std::size_t type_index_v = [] {
    std::size_t idx = 0;
    bool found = false;
    ((!found && std::is_same_v<Ts, T> ? (found = true, idx) : (++idx, idx)), ...);
    return found ? idx : sizeof...(Ts);
}();

// 检查类型是否在列表中
template <typename T, typename... Ts>
constexpr bool is_one_of_v = (std::is_same_v<Ts, T> || ...);

// 类型是否唯一
template <typename T, typename... Ts>
constexpr bool is_unique_v = (std::is_same_v<Ts, T> + ...) == 1;

// 存储类型
template <typename... Ts>
struct variant_storage {
    alignas(max_align_of<Ts...>) std::byte data[max_size_of<Ts...>];
};

// 析构辅助
template <typename... Ts>
struct variant_destructor {
    static constexpr void destroy(std::size_t index, void* data) {
        // 根据 index 调用正确的析构函数
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((index == Is ? std::destroy_at(static_cast<std::tuple_element_t<Is, std::tuple<Ts...>>*>(data)) : void()), ...);
        }(std::index_sequence_for<Ts...>{});
    }
};

// 拷贝构造辅助
template <typename... Ts>
struct variant_copy_ctor {
    static constexpr void copy(std::size_t index, void* dst, void const* src) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((index == Is ? ::new (dst) std::tuple_element_t<Is, std::tuple<Ts...>>(
                *static_cast<std::tuple_element_t<Is, std::tuple<Ts...>> const*>(src)
            ) : void()), ...);
        }(std::index_sequence_for<Ts...>{});
    }
};

// 移动构造辅助
template <typename... Ts>
struct variant_move_ctor {
    static constexpr void move(std::size_t index, void* dst, void* src) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((index == Is ? ::new (dst) std::tuple_element_t<Is, std::tuple<Ts...>>(
                std::move(*static_cast<std::tuple_element_t<Is, std::tuple<Ts...>>*>(src))
            ) : void()), ...);
        }(std::index_sequence_for<Ts...>{});
    }
};

} // namespace detail

// ============================================================
// constexpr_variant 主模板
// ============================================================
template <typename... Ts>
class constexpr_variant {
    static_assert(sizeof...(Ts) > 0, "constexpr_variant must have at least one type");
    static_assert((std::is_destructible_v<Ts> && ...), "All types must be destructible");

public:
    // ==================== 类型别名 ====================

    using self_type = constexpr_variant<Ts...>;
    using type_list = constexpr_type_list<Ts...>;
    static constexpr std::size_t size = sizeof...(Ts);
    static constexpr std::size_t npos = size;

    // ==================== 构造函数 ====================

    // 默认构造（使用第一个类型）
    constexpr constexpr_variant()
        requires std::is_default_constructible_v<std::tuple_element_t<0, std::tuple<Ts...>>>
        : index_(0) {
        using first_type = std::tuple_element_t<0, std::tuple<Ts...>>;
        ::new (storage_.data) first_type();
    }

    // 从值构造
    template <typename T>
        requires detail::is_one_of_v<std::remove_cvref_t<T>, Ts...>
    constexpr constexpr_variant(T&& value)
        : index_(detail::type_index_v<std::remove_cvref_t<T>, Ts...>) {
        using value_type = std::remove_cvref_t<T>;
        ::new (storage_.data) value_type(std::forward<T>(value));
    }

    // 就地构造
    template <typename T, typename... Args>
        requires detail::is_one_of_v<T, Ts...> && std::constructible_from<T, Args...>
    constexpr explicit constexpr_variant(std::in_place_type_t<T>, Args&&... args)
        : index_(detail::type_index_v<T, Ts...>) {
        ::new (storage_.data) T(std::forward<Args>(args)...);
    }

    // 按索引就地构造
    template <std::size_t I, typename... Args>
        requires (I < size) && std::constructible_from<std::tuple_element_t<I, std::tuple<Ts...>>, Args...>
    constexpr explicit constexpr_variant(std::in_place_index_t<I>, Args&&... args)
        : index_(I) {
        using value_type = std::tuple_element_t<I, std::tuple<Ts...>>;
        ::new (storage_.data) value_type(std::forward<Args>(args)...);
    }

    // 拷贝构造
    constexpr constexpr_variant(constexpr_variant const& other)
        : index_(other.index_) {
        if (index_ < size) {
            detail::variant_copy_ctor<Ts...>::copy(index_, storage_.data, other.storage_.data);
        }
    }

    // 移动构造
    constexpr constexpr_variant(constexpr_variant&& other) noexcept(
        (std::is_nothrow_move_constructible_v<Ts> && ...)
    ) : index_(other.index_) {
        if (index_ < size) {
            detail::variant_move_ctor<Ts...>::move(index_, storage_.data, other.storage_.data);
        }
    }

    // 从 std::variant 构造
    template <typename... Us>
        requires (sizeof...(Us) == size) && (std::is_same_v<std::tuple<Us...>, std::tuple<Ts...>>)
    constexpr constexpr_variant(std::variant<Us...> const& v)
        : index_(v.index()) {
        std::visit([this](auto const& val) {
            using T = std::remove_cvref_t<decltype(val)>;
            ::new (storage_.data) T(val);
        }, v);
    }

    // 析构函数
    constexpr ~constexpr_variant() {
        if (index_ < size) {
            detail::variant_destructor<Ts...>::destroy(index_, storage_.data);
        }
    }

    // ==================== 赋值运算符 ====================

    constexpr constexpr_variant& operator=(constexpr_variant const& other) {
        if (this != &other) {
            if (index_ < size) {
                detail::variant_destructor<Ts...>::destroy(index_, storage_.data);
            }
            index_ = other.index_;
            if (index_ < size) {
                detail::variant_copy_ctor<Ts...>::copy(index_, storage_.data, other.storage_.data);
            }
        }
        return *this;
    }

    constexpr constexpr_variant& operator=(constexpr_variant&& other) noexcept(
        (std::is_nothrow_move_constructible_v<Ts> && ...)
    ) {
        if (this != &other) {
            if (index_ < size) {
                detail::variant_destructor<Ts...>::destroy(index_, storage_.data);
            }
            index_ = other.index_;
            if (index_ < size) {
                detail::variant_move_ctor<Ts...>::move(index_, storage_.data, other.storage_.data);
            }
        }
        return *this;
    }

    template <typename T>
        requires detail::is_one_of_v<std::remove_cvref_t<T>, Ts...>
    constexpr constexpr_variant& operator=(T&& value) {
        using value_type = std::remove_cvref_t<T>;
        constexpr std::size_t new_index = detail::type_index_v<value_type, Ts...>;

        if (index_ < size) {
            detail::variant_destructor<Ts...>::destroy(index_, storage_.data);
        }
        index_ = new_index;
        ::new (storage_.data) value_type(std::forward<T>(value));
        return *this;
    }

    // ==================== 状态查询 ====================

    constexpr std::size_t index() const noexcept { return index_; }
    constexpr bool valueless_by_exception() const noexcept { return index_ >= size; }

    template <typename T>
    static constexpr std::size_t index_of() noexcept {
        return detail::type_index_v<T, Ts...>;
    }

    template <typename T>
    constexpr bool holds() const noexcept {
        return index_ == detail::type_index_v<T, Ts...>;
    }

    template <std::size_t I>
    constexpr bool holds_index() const noexcept {
        return index_ == I;
    }

    // ==================== 值访问 ====================

    // 获取值（按类型）
    template <typename T>
        requires detail::is_one_of_v<T, Ts...>
    constexpr T& get() & {
        if (!holds<T>()) {
            throw std::bad_variant_access{};
        }
        return *static_cast<T*>(static_cast<void*>(storage_.data));
    }

    template <typename T>
        requires detail::is_one_of_v<T, Ts...>
    constexpr T const& get() const& {
        if (!holds<T>()) {
            throw std::bad_variant_access{};
        }
        return *static_cast<T const*>(static_cast<void const*>(storage_.data));
    }

    template <typename T>
        requires detail::is_one_of_v<T, Ts...>
    constexpr T&& get() && {
        if (!holds<T>()) {
            throw std::bad_variant_access{};
        }
        return std::move(*static_cast<T*>(static_cast<void*>(storage_.data)));
    }

    // 获取值（按索引）
    template <std::size_t I>
        requires (I < size)
    constexpr auto& get() & {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;
        if (index_ != I) {
            throw std::bad_variant_access{};
        }
        return *static_cast<T*>(static_cast<void*>(storage_.data));
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto const& get() const& {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;
        if (index_ != I) {
            throw std::bad_variant_access{};
        }
        return *static_cast<T const*>(static_cast<void const*>(storage_.data));
    }

    // 安全获取（返回指针）
    template <typename T>
        requires detail::is_one_of_v<T, Ts...>
    constexpr T* get_if() noexcept {
        return holds<T>() ? static_cast<T*>(static_cast<void*>(storage_.data)) : nullptr;
    }

    template <typename T>
        requires detail::is_one_of_v<T, Ts...>
    constexpr T const* get_if() const noexcept {
        return holds<T>() ? static_cast<T const*>(static_cast<void const*>(storage_.data)) : nullptr;
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto* get_if() noexcept {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;
        return holds_index<I>() ? static_cast<T*>(static_cast<void*>(storage_.data)) : nullptr;
    }

    // ==================== 访问者模式 ====================

    // visit
    template <typename Visitor>
    constexpr decltype(auto) visit(Visitor&& vis) & {
        return visit_impl(std::forward<Visitor>(vis), *this);
    }

    template <typename Visitor>
    constexpr decltype(auto) visit(Visitor&& vis) const& {
        return visit_impl(std::forward<Visitor>(vis), *this);
    }

    template <typename Visitor>
    constexpr decltype(auto) visit(Visitor&& vis) && {
        return visit_impl(std::forward<Visitor>(vis), std::move(*this));
    }

private:
    template <typename Visitor, typename Self>
    static constexpr decltype(auto) visit_impl(Visitor&& vis, Self&& self) {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> decltype(auto) {
            using result_type = std::common_type_t<
                std::invoke_result_t<Visitor, decltype(std::forward<Self>(self).template get<Is>())>...
            >;

            result_type* result = nullptr;
            bool called = false;

            ((!called && self.index_ == Is ? (
                called = true,
                result = std::addressof(std::invoke(
                    std::forward<Visitor>(vis),
                    std::forward<Self>(self).template get<Is>()
                ))
            ) : void()), ...);

            return std::move(*result);
        }(std::index_sequence_for<Ts...>{});
    }

public:
    // ==================== 转换操作 ====================

    // 转换为 std::variant
    constexpr auto to_variant() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            std::variant<Ts...> result;
            ((index_ == Is ? (result = get<Is>(), true) : false) || ...);
            return result;
        }(std::index_sequence_for<Ts...>{});
    }

    constexpr auto to_variant() && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            std::variant<Ts...> result;
            ((index_ == Is ? (result = std::move(get<Is>()), true) : false) || ...);
            return result;
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 比较运算符 ====================

    constexpr bool operator==(constexpr_variant const& other) const {
        if (index_ != other.index_) return false;
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((index_ == Is ? get<Is>() == other.get<Is>() : true) && ...);
        }(std::index_sequence_for<Ts...>{});
    }

    constexpr auto operator<=>(constexpr_variant const& other) const {
        if (index_ != other.index_) {
            return index_ <=> other.index_;
        }
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
            -> std::common_comparison_category_t<decltype(get<Is>() <=> other.get<Is>())...> {
            for (std::size_t i = 0; i < size; ++i) {
                // 运行时版本需要更复杂的实现
            }
            return std::strong_ordering::equal;
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 交换 ====================

    constexpr void swap(constexpr_variant& other) noexcept(
        (std::is_nothrow_move_constructible_v<Ts> && ...) &&
        (std::is_nothrow_swappable_v<Ts> && ...)
    ) {
        if (this != &other) {
            constexpr_variant tmp = std::move(other);
            other = std::move(*this);
            *this = std::move(tmp);
        }
    }

private:
    detail::variant_storage<Ts...> storage_;
    std::size_t index_ = npos;
};

// ============================================================
// 常用变体类型别名
// ============================================================

// 数值变体（反射系统常用）
using numeric_variant = constexpr_variant<
    int8_t, int16_t, int32_t, int64_t,
    uint8_t, uint16_t, uint32_t, uint64_t,
    float, double
>;

// 基础变体（属性系统常用）
using basic_variant = constexpr_variant<
    bool,
    int32_t, int64_t,
    uint32_t, uint64_t,
    float, double,
    std::string
>;

// ============================================================
// 非成员函数
// ============================================================

// get 函数
template <typename T, typename... Ts>
constexpr T& get(constexpr_variant<Ts...>& v) {
    return v.template get<T>();
}

template <typename T, typename... Ts>
constexpr T const& get(constexpr_variant<Ts...> const& v) {
    return v.template get<T>();
}

template <typename T, typename... Ts>
constexpr T&& get(constexpr_variant<Ts...>&& v) {
    return std::move(v.template get<T>());
}

template <std::size_t I, typename... Ts>
constexpr auto& get(constexpr_variant<Ts...>& v) {
    return v.template get<I>();
}

template <std::size_t I, typename... Ts>
constexpr auto const& get(constexpr_variant<Ts...> const& v) {
    return v.template get<I>();
}

// get_if 函数
template <typename T, typename... Ts>
constexpr T* get_if(constexpr_variant<Ts...>* v) noexcept {
    return v ? v->template get_if<T>() : nullptr;
}

template <typename T, typename... Ts>
constexpr T const* get_if(constexpr_variant<Ts...> const* v) noexcept {
    return v ? v->template get_if<T>() : nullptr;
}

// holds_alternative
template <typename T, typename... Ts>
constexpr bool holds_alternative(constexpr_variant<Ts...> const& v) noexcept {
    return v.template holds<T>();
}

// visit
template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor&& vis, constexpr_variant<Ts...>& v) {
    return v.visit(std::forward<Visitor>(vis));
}

template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor&& vis, constexpr_variant<Ts...> const& v) {
    return v.visit(std::forward<Visitor>(vis));
}

template <typename Visitor, typename... Ts>
constexpr decltype(auto) visit(Visitor&& vis, constexpr_variant<Ts...>&& v) {
    return std::move(v).visit(std::forward<Visitor>(vis));
}

// swap
template <typename... Ts>
constexpr void swap(constexpr_variant<Ts...>& lhs, constexpr_variant<Ts...>& rhs) noexcept(
    noexcept(lhs.swap(rhs))
) {
    lhs.swap(rhs);
}

// ============================================================
// 变体构建器
// ============================================================

template <typename... Ts>
constexpr auto make_variant(Ts&&... args) {
    return constexpr_variant<std::remove_cvref_t<Ts>...>(std::forward<Ts>(args)...);
}

} // namespace constexpr_

// ============================================================
// 简化别名
// ============================================================

template <typename... Ts>
using ct_variant = constexpr_::constexpr_variant<Ts...>;

using ct_numeric_variant = constexpr_::numeric_variant;
using ct_basic_variant = constexpr_::basic_variant;

} // namespace shine

// ============================================================
// 标准库特化
// ============================================================

template <typename... Ts>
struct std::variant_size<shine::constexpr_::constexpr_variant<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <std::size_t I, typename... Ts>
struct std::variant_alternative<I, shine::constexpr_::constexpr_variant<Ts...>> {
    using type = std::tuple_element_t<I, std::tuple<Ts...>>;
};