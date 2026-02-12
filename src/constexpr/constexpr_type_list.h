#pragma once

#include <array>
#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

namespace shine::constexpr_ {

// ============================================================
// constexpr_type_list
// ============================================================

template <typename... Ts>
struct constexpr_type_list {

    using self = constexpr_type_list;

    static constexpr std::size_t size  = sizeof...(Ts);
    static constexpr bool        empty = size == 0;
    static constexpr std::size_t npos  = static_cast<std::size_t>(-1);

    using as_tuple = std::tuple<Ts...>;

    // ========================================================
    // 访问
    // ========================================================

    template <std::size_t I>
    using at = std::tuple_element_t<I, as_tuple>;

private:
    // 修复: 使用辅助结构体实现惰性求值，避免空列表时实例化 at<0>
    template <bool IsEmpty>
    struct first_impl {
        using type = void;
    };

    template <>
    struct first_impl<false> {
        using type = at<0>;
    };

    template <bool IsEmpty>
    struct last_impl {
        using type = void;
    };

    template <>
    struct last_impl<false> {
        using type = at<size - 1>;
    };

public:
    using first = typename first_impl<empty>::type;
    using last  = typename last_impl<empty>::type;

    // ========================================================
    // 查询
    // ========================================================

    template <typename T>
    static constexpr bool contains = (std::same_as<T, Ts> || ...);

    template <typename T>
    static constexpr std::size_t count =
        (static_cast<std::size_t>(std::same_as<T, Ts>) + ...);

private:
    template <typename T, std::size_t Index = 0>
    static consteval std::size_t find_impl() {
        if constexpr (Index >= size)
            return npos;
        else if constexpr (std::same_as<T, at<Index>>)
            return Index;
        else
            return find_impl<T, Index + 1>();
    }

public:
    template <typename T>
    static constexpr std::size_t find = find_impl<T>();

    // ========================================================
    // 添加
    // ========================================================

    template <typename T>
    using push_front = constexpr_type_list<T, Ts...>;

    template <typename T>
    using push_back = constexpr_type_list<Ts..., T>;

    template <typename... Us>
    using push_front_n = constexpr_type_list<Us..., Ts...>;

    template <typename... Us>
    using push_back_n = constexpr_type_list<Ts..., Us...>;

    // ========================================================
    // 删除
    // ========================================================

private:
    template <typename T, typename...>
    struct remove_impl;

    template <typename T>
    struct remove_impl<T> {
        using type = constexpr_type_list<>;
    };

    template <typename T, typename Head, typename... Tail>
    struct remove_impl<T, Head, Tail...> {

        using tail = typename remove_impl<T, Tail...>::type;

        using type = std::conditional_t<
            std::same_as<T, Head>,
            tail,
            typename tail::template push_front<Head>>;
    };

public:
    template <typename T>
    using remove = typename remove_impl<T, Ts...>::type;

    // ========================================================
    // 变换
    // ========================================================

    template <template <typename> typename Transform>
    using transform = constexpr_type_list<Transform<Ts>...>;

private:
    template <template <typename> typename Pred, typename...>
    struct filter_impl;

    template <template <typename> typename Pred>
    struct filter_impl<Pred> {
        using type = constexpr_type_list<>;
    };

    template <template <typename> typename Pred,
              typename Head, typename... Tail>
    struct filter_impl<Pred, Head, Tail...> {

        using tail = typename filter_impl<Pred, Tail...>::type;

        using type = std::conditional_t<
            Pred<Head>::value,
            typename tail::template push_front<Head>,
            tail>;
    };

public:
    template <template <typename> typename Pred>
    using filter = typename filter_impl<Pred, Ts...>::type;

    // ========================================================
    // concat
    // ========================================================

    template <typename Other>
    struct concat_impl;

    template <typename... Us>
    struct concat_impl<constexpr_type_list<Us...>> {
        using type = constexpr_type_list<Ts..., Us...>;
    };

    template <typename Other>
    using concat = typename concat_impl<Other>::type;

    // ========================================================
    // repeat
    // ========================================================

private:
    template <std::size_t N, typename Acc>
    struct repeat_impl;

    template <typename Acc>
    struct repeat_impl<0, Acc> {
        using type = Acc;
    };

    template <std::size_t N, typename Acc>
    struct repeat_impl {

        using next =
            typename Acc::template concat<constexpr_type_list<Ts...>>;

        using type =
            typename repeat_impl<N - 1, next>::type;
    };

public:
    template <std::size_t N>
    using repeat = typename repeat_impl<N, constexpr_type_list<>>::type;

    // ========================================================
    // reverse
    // ========================================================

private:
    template <typename...>
    struct reverse_impl;

    template <>
    struct reverse_impl<> {
        using type = constexpr_type_list<>;
    };

    template <typename Head, typename... Tail>
    struct reverse_impl<Head, Tail...> {
        using type =
            typename reverse_impl<Tail...>::type ::template push_back<Head>;
    };

public:
    using reverse = typename reverse_impl<Ts...>::type;

    // ========================================================
    // unique
    // ========================================================

private:
    template <typename Result, typename...>
    struct unique_impl;

    template <typename Result>
    struct unique_impl<Result> {
        using type = Result;
    };

    template <typename Result, typename Head, typename... Tail>
    struct unique_impl<Result, Head, Tail...> {

        using next =
            std::conditional_t<
                Result::template contains<Head>,
                Result,
                typename Result::template push_back<Head>>;

        using type =
            typename unique_impl<next, Tail...>::type;
    };

public:
    using unique =
        typename unique_impl<constexpr_type_list<>, Ts...>::type;

    // ========================================================
    // 反射辅助
    // ========================================================

    static constexpr auto type_sizes() {
        return std::array<std::size_t, size>{sizeof(Ts)...};
    }

    static constexpr auto type_aligns() {
        return std::array<std::size_t, size>{alignof(Ts)...};
    }

    // ========================================================
    // 遍历
    // ========================================================

    template <typename F>
    static constexpr void for_each(F &&f) {
        (f(std::type_identity<Ts>{}), ...);
    }

    template <typename F>
    static constexpr void for_each_i(F &&f) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (f(std::type_identity<at<Is>>{},
               std::integral_constant<std::size_t, Is>{}),
             ...);
        }(std::make_index_sequence<size>{});
    }
};

// ============================================================
// 多列表 concat
// ============================================================

template <typename... Lists>
struct type_list_concat_all;

template <>
struct type_list_concat_all<> {
    using type = constexpr_type_list<>;
};

template <typename List>
struct type_list_concat_all<List> {
    using type = List;
};

template <typename List1, typename List2, typename... Rest>
struct type_list_concat_all<List1, List2, Rest...> {

    using merged =
        List1::template concat<List2>;

    using type =
        type_list_concat_all<
            merged,
            Rest...>::type;
};

template <typename... Lists>
using type_list_concat_all_t =
    type_list_concat_all<Lists...>::type;

// ============================================================
// 概念
// ============================================================

template <typename T>
concept is_type_list = requires {
    T::size;
};

} // namespace shine::constexpr_