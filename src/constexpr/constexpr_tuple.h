#pragma once

#include <utility>
#include <type_traits>
#include <concepts>
#include <tuple>
#include <array>
#include <functional>
#include <compare>

namespace shine {
namespace constexpr_ {

// ============================================================
// constexpr_tuple - 编译期可修改元组
// 
// 特性：
// - 完全 constexpr 支持
// - 编译期元素添加/移除
// - 与 std::tuple 互转
// - 元组拼接、切片、变换
// - 结构化绑定支持
// ============================================================

// 前向声明
template <typename... Ts>
struct constexpr_tuple;

namespace detail {

// 元组元素存储
template <std::size_t I, typename T>
struct tuple_element {
    T value;

    constexpr tuple_element() = default;
    constexpr explicit tuple_element(T const& v) : value(v) {}
    constexpr explicit tuple_element(T&& v) : value(std::move(v)) {}

    template <typename... Args>
    constexpr explicit tuple_element(std::in_place_t, Args&&... args)
        : value(std::forward<Args>(args)...) {}
};

// 元组存储基类（使用多重继承）
template <typename IndexSeq, typename... Ts>
struct tuple_storage;

template <std::size_t... Is, typename... Ts>
struct tuple_storage<std::index_sequence<Is...>, Ts...>
    : tuple_element<Is, Ts>... {

    constexpr tuple_storage() = default;

    template <typename... Args>
    constexpr explicit tuple_storage(Args&&... args)
        : tuple_element<Is, Ts>(std::forward<Args>(args))... {}

    template <typename... Args>
    constexpr explicit tuple_storage(std::in_place_t, Args&&... args)
        : tuple_element<Is, Ts>(std::in_place_t{}, std::forward<Args>(args))... {}
};

// 获取元素辅助函数
template <std::size_t I, typename T>
constexpr T& get_element(tuple_element<I, T>& elem) noexcept {
    return elem.value;
}

template <std::size_t I, typename T>
constexpr T const& get_element(tuple_element<I, T> const& elem) noexcept {
    return elem.value;
}

template <std::size_t I, typename T>
constexpr T&& get_element(tuple_element<I, T>&& elem) noexcept {
    return std::move(elem.value);
}

template <std::size_t I, typename T>
constexpr T const&& get_element(tuple_element<I, T> const&& elem) noexcept {
    return std::move(elem.value);
}

} // namespace detail

// ============================================================
// constexpr_tuple 主模板
// ============================================================
template <typename... Ts>
struct constexpr_tuple
    : detail::tuple_storage<std::index_sequence_for<Ts...>, Ts...> {

    using base_type = detail::tuple_storage<std::index_sequence_for<Ts...>, Ts...>;
    using self_type = constexpr_tuple<Ts...>;

    static constexpr std::size_t size = sizeof...(Ts);
    static constexpr bool empty = (size == 0);

    // ==================== 构造函数 ====================

    constexpr constexpr_tuple() = default;

    // 从值构造
    template <typename... Args>
        requires (sizeof...(Args) == size) && (std::constructible_from<Ts, Args> && ...)
    constexpr explicit constexpr_tuple(Args&&... args)
        : base_type(std::forward<Args>(args)...) {}

    // 从 std::tuple 转换
    template <typename... Us>
        requires (sizeof...(Us) == size) && (std::convertible_to<Us, Ts> && ...)
    constexpr constexpr_tuple(std::tuple<Us...> const& t)
        : base_type(std::apply([](Us const&... us) {
            return base_type(static_cast<Ts>(us)...);
        }, t)) {}

    template <typename... Us>
        requires (sizeof...(Us) == size) && (std::convertible_to<Us, Ts> && ...)
    constexpr constexpr_tuple(std::tuple<Us...>&& t)
        : base_type(std::apply([](Us&&... us) {
            return base_type(static_cast<Ts>(std::move(us))...);
        }, std::move(t))) {}

    // 从另一个 constexpr_tuple 转换
    template <typename... Us>
        requires (sizeof...(Us) == size) && (std::convertible_to<Us, Ts> && ...)
    constexpr constexpr_tuple(constexpr_tuple<Us...> const& other)
        : base_type([&other]<std::size_t... Is>(std::index_sequence<Is...>) {
            return base_type(static_cast<Ts>(other.template get<Is>())...);
        }(std::index_sequence_for<Ts...>{})) {}

    template <typename... Us>
        requires (sizeof...(Us) == size) && (std::convertible_to<Us, Ts> && ...)
    constexpr constexpr_tuple(constexpr_tuple<Us...>&& other)
        : base_type([&other]<std::size_t... Is>(std::index_sequence<Is...>) {
            return base_type(static_cast<Ts>(std::move(other.template get<Is>()))...);
        }(std::index_sequence_for<Ts...>{})) {}

    // ==================== 元素访问 ====================

    // 按索引访问
    template <std::size_t I>
        requires (I < size)
    constexpr auto& get() & noexcept {
        return detail::get_element<I>(*this);
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto const& get() const& noexcept {
        return detail::get_element<I>(*this);
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto&& get() && noexcept {
        return detail::get_element<I>(std::move(*this));
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto const&& get() const&& noexcept {
        return detail::get_element<I>(std::move(*this));
    }

    // 运行时索引访问（返回指针）
    template <std::size_t I>
        requires (I < size)
    constexpr auto get_ptr() noexcept {
        return std::addressof(get<I>());
    }

    template <std::size_t I>
        requires (I < size)
    constexpr auto get_ptr() const noexcept {
        return std::addressof(get<I>());
    }

    // ==================== 元素修改 ====================

    // 设置元素值
    template <std::size_t I, typename U>
        requires (I < size) && std::assignable_from<Ts&, U>
    constexpr void set(U&& value) {
        get<I>() = std::forward<U>(value);
    }

    // 原地构造元素
    template <std::size_t I, typename... Args>
        requires (I < size) && std::constructible_from<Ts, Args...>
    constexpr void emplace(Args&&... args) {
        ::new (get_ptr<I>()) Ts(std::forward<Args>(args)...);
    }

    // ==================== 添加元素 ====================

    // 后置添加
    template <typename T>
    constexpr auto push_back(T&& value) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<Ts..., std::remove_cvref_t<T>>(
                get<Is>()..., std::forward<T>(value)
            );
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename T>
    constexpr auto push_back(T&& value) && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<Ts..., std::remove_cvref_t<T>>(
                std::move(get<Is>())..., std::forward<T>(value)
            );
        }(std::index_sequence_for<Ts...>{});
    }

    // 前置添加
    template <typename T>
    constexpr auto push_front(T&& value) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<std::remove_cvref_t<T>, Ts...>(
                std::forward<T>(value), get<Is>()...
            );
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename T>
    constexpr auto push_front(T&& value) && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<std::remove_cvref_t<T>, Ts...>(
                std::forward<T>(value), std::move(get<Is>())...
            );
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 移除元素 ====================

    // 移除指定索引元素
    template <std::size_t I>
        requires (I < size)
    constexpr auto erase() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return [&]<std::size_t... Js>(std::index_sequence<Js...>) {
                if constexpr (I == 0) {
                    return constexpr_tuple<Ts...>(get<Is + 1>()...);
                } else if constexpr (I == size - 1) {
                    return constexpr_tuple<Ts...>(get<Is>()...);
                } else {
                    return constexpr_tuple<Ts...>(
                        get<Is>()..., get<(Js + I + 1) % size>()...
                    );
                }
            }(std::make_index_sequence<(I < size - 1) ? (size - I - 1) : 0>{});
        }(std::make_index_sequence<I>{});
    }

    // 移除第一个元素
    constexpr auto pop_front() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<Ts...>(get<Is + 1>()...);
        }(std::make_index_sequence<size - 1>{});
    }

    // 移除最后一个元素
    constexpr auto pop_back() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<Ts...>(get<Is>()...);
        }(std::make_index_sequence<size - 1>{});
    }

    // ==================== 元组拼接 ====================

    // 连接另一个元组
    template <typename... Us>
    constexpr auto concat(constexpr_tuple<Us...> const& other) const& {
        return [&]<std::size_t... Is, std::size_t... Js>(
            std::index_sequence<Is...>, std::index_sequence<Js...>
        ) {
            return constexpr_tuple<Ts..., Us...>(
                get<Is>()..., other.template get<Js>()...
            );
        }(std::index_sequence_for<Ts...>{}, std::index_sequence_for<Us...>{});
    }

    template <typename... Us>
    constexpr auto concat(constexpr_tuple<Us...>&& other) && {
        return [&]<std::size_t... Is, std::size_t... Js>(
            std::index_sequence<Is...>, std::index_sequence<Js...>
        ) {
            return constexpr_tuple<Ts..., Us...>(
                std::move(get<Is>())..., std::move(other.template get<Js>())...
            );
        }(std::index_sequence_for<Ts...>{}, std::index_sequence_for<Us...>{});
    }

    // ==================== 元组切片 ====================

    // 获取子元组 [Start, End)
    template <std::size_t Start, std::size_t End = size>
        requires (Start <= End && End <= size)
    constexpr auto slice() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            using result_type = constexpr_tuple<std::tuple_element_t<Start + Is, std::tuple<Ts...>>...>;
            return result_type(get<Start + Is>()...);
        }(std::make_index_sequence<End - Start>{});
    }

    template <std::size_t Start, std::size_t End = size>
        requires (Start <= End && End <= size)
    constexpr auto slice() && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            using result_type = constexpr_tuple<std::tuple_element_t<Start + Is, std::tuple<Ts...>>...>;
            return result_type(std::move(get<Start + Is>())...);
        }(std::make_index_sequence<End - Start>{});
    }

    // ==================== 元组变换 ====================

    // 对每个元素应用函数
    template <typename F>
    constexpr auto transform(F&& f) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<std::invoke_result_t<F, Ts const&>...>(
                std::invoke(std::forward<F>(f), get<Is>())...
            );
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename F>
    constexpr auto transform(F&& f) && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<std::invoke_result_t<F, Ts>...>(
                std::invoke(std::forward<F>(f), std::move(get<Is>()))...
            );
        }(std::index_sequence_for<Ts...>{});
    }

    // 对每个元素应用带索引的函数
    template <typename F>
    constexpr auto transform_i(F&& f) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return constexpr_tuple<std::invoke_result_t<F, Ts const&, std::size_t>...>(
                std::invoke(std::forward<F>(f), get<Is>(), Is)...
            );
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 元组折叠 ====================

    // 左折叠
    template <typename Init, typename F>
    constexpr auto fold_left(Init init, F&& f) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((init = std::invoke(std::forward<F>(f), std::move(init), get<Is>())), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    // 右折叠
    template <typename Init, typename F>
    constexpr auto fold_right(Init init, F&& f) const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((init = std::invoke(std::forward<F>(f), get<Is>(), std::move(init))), ...);
        }(std::make_index_sequence<size>{});
    }

    // ==================== 元组遍历 ====================

    // 遍历每个元素
    template <typename F>
    constexpr void for_each(F&& f) const& {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (std::invoke(std::forward<F>(f), get<Is>()), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename F>
    constexpr void for_each(F&& f) & {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (std::invoke(std::forward<F>(f), get<Is>()), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    // 带索引遍历
    template <typename F>
    constexpr void for_each_i(F&& f) const& {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (std::invoke(std::forward<F>(f), get<Is>(), std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 元组查找 ====================

    // 查找满足条件的元素索引
    template <typename Pred>
    constexpr std::size_t find_if(Pred&& pred) const {
        std::size_t idx = 0;
        bool found = false;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((!found && std::invoke(std::forward<Pred>(pred), get<Is>())
                ? (found = true, idx = Is) : void()), ...);
        }(std::index_sequence_for<Ts...>{});
        return found ? idx : npos;
    }

    // 检查是否有元素满足条件
    template <typename Pred>
    constexpr bool any_of(Pred&& pred) const {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (std::invoke(std::forward<Pred>(pred), get<Is>()) || ...);
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename Pred>
    constexpr bool all_of(Pred&& pred) const {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return (std::invoke(std::forward<Pred>(pred), get<Is>()) && ...);
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 类型操作 ====================

    // 获取类型列表
    using type_list = constexpr_type_list<Ts...>;

    // 获取第 I 个类型
    template <std::size_t I>
    using type_at = std::tuple_element_t<I, std::tuple<Ts...>>;

    // ==================== 转换操作 ====================

    // 转换为 std::tuple
    constexpr auto to_tuple() const& {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(get<Is>()...);
        }(std::index_sequence_for<Ts...>{});
    }

    constexpr auto to_tuple() && {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::make_tuple(std::move(get<Is>())...);
        }(std::index_sequence_for<Ts...>{});
    }

    // 转换为 std::pair（仅当 size == 2）
    constexpr auto to_pair() const& -> std::pair<Ts...>
        requires (size == 2) {
        return std::make_pair(get<0>(), get<1>());
    }

    constexpr auto to_pair() && -> std::pair<Ts...>
        requires (size == 2) {
        return std::make_pair(std::move(get<0>()), std::move(get<1>()));
    }

    // ==================== 比较运算符 ====================

    template <typename... Us>
        requires (sizeof...(Us) == size)
    constexpr bool operator==(constexpr_tuple<Us...> const& other) const {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return ((get<Is>() == other.template get<Is>()) && ...);
        }(std::index_sequence_for<Ts...>{});
    }

    template <typename... Us>
        requires (sizeof...(Us) == size)
    constexpr auto operator<=>(constexpr_tuple<Us...> const& other) const {
        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
            -> std::common_comparison_category_t<decltype(get<Is>() <=> other.template get<Is>())...> {
            for (std::size_t i = 0; i < size; ++i) {
                // 运行时比较
                // 编译期版本需要递归展开
            }
            return std::strong_ordering::equal;
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 交换 ====================

    constexpr void swap(constexpr_tuple& other) noexcept(
        (std::is_nothrow_swappable_v<Ts> && ...)
    ) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            using std::swap;
            (swap(get<Is>(), other.template get<Is>()), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    // ==================== 静态常量 ====================

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);
};

// ============================================================
// 空元组特化
// ============================================================
template <>
struct constexpr_tuple<> {
    using self_type = constexpr_tuple<>;
    static constexpr std::size_t size = 0;
    static constexpr bool empty = true;
    using type_list = constexpr_type_list<>;

    constexpr constexpr_tuple() = default;

    template <typename T>
    constexpr auto push_back(T&& value) const {
        return constexpr_tuple<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    template <typename T>
    constexpr auto push_front(T&& value) const {
        return constexpr_tuple<std::remove_cvref_t<T>>(std::forward<T>(value));
    }

    constexpr constexpr_tuple<> to_tuple() const { return {}; }

    template <typename... Us>
    constexpr auto concat(constexpr_tuple<Us...> const& other) const {
        return other;
    }

    constexpr bool operator==(constexpr_tuple const&) const { return true; }
};

// ============================================================
// CTAD 推导指引
// ============================================================

template <typename... Ts>
constexpr_tuple(Ts...) -> constexpr_tuple<Ts...>;

template <typename... Ts>
constexpr_tuple(std::tuple<Ts...>) -> constexpr_tuple<Ts...>;

// ============================================================
// 非成员函数
// ============================================================

// get 函数（ADL 兼容）
template <std::size_t I, typename... Ts>
constexpr auto& get(constexpr_tuple<Ts...>& t) noexcept {
    return t.template get<I>();
}

template <std::size_t I, typename... Ts>
constexpr auto const& get(constexpr_tuple<Ts...> const& t) noexcept {
    return t.template get<I>();
}

template <std::size_t I, typename... Ts>
constexpr auto&& get(constexpr_tuple<Ts...>&& t) noexcept {
    return std::move(t.template get<I>());
}

template <std::size_t I, typename... Ts>
constexpr auto const&& get(constexpr_tuple<Ts...> const&& t) noexcept {
    return std::move(t.template get<I>());
}

// swap 重载
template <typename... Ts>
constexpr void swap(constexpr_tuple<Ts...>& lhs, constexpr_tuple<Ts...>& rhs) noexcept(
    (std::is_nothrow_swappable_v<Ts> && ...)
) {
    lhs.swap(rhs);
}

// make_constexpr_tuple
template <typename... Ts>
constexpr auto make_constexpr_tuple(Ts&&... args) {
    return constexpr_tuple<std::remove_cvref_t<Ts>...>(std::forward<Ts>(args)...);
}

// 从 std::tuple 创建
template <typename... Ts>
constexpr auto make_constexpr_tuple_from_std(std::tuple<Ts...> const& t) {
    return constexpr_tuple<Ts...>(t);
}

// tuple_cat 等价物
template <typename... Tuples>
constexpr auto constexpr_tuple_cat(Tuples&&... tuples) {
    return (... .concat(std::forward<Tuples>(tuples)));
}

// ============================================================
// 结构化绑定支持
// ============================================================

} // namespace constexpr_
} // namespace shine

// ============================================================
// 标准库特化
// ============================================================

// std::tuple_size
template <typename... Ts>
struct std::tuple_size<shine::constexpr_::constexpr_tuple<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

// std::tuple_element
template <std::size_t I, typename... Ts>
struct std::tuple_element<I, shine::constexpr_::constexpr_tuple<Ts...>> {
    using type = std::tuple_element_t<I, std::tuple<Ts...>>;
};

// std::tuple_size for empty tuple
template <>
struct std::tuple_size<shine::constexpr_::constexpr_tuple<>>
    : std::integral_constant<std::size_t, 0> {};