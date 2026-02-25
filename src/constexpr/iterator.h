#pragma once

#include <array>
#include <type_traits>
#include <concepts>
#include <utility>
#include <cstdint>


namespace shine {
namespace constexpr_ {

constexpr std::size_t npos = static_cast<std::size_t>(-1);

// 前向声明 constexpr_type_list（定义在 constexpr_type_list.h）
template <typename... Ts>
struct constexpr_type_list;

} // namespace constexpr_
} // namespace shine

namespace shine {

// ============================================================
// 编译期常量和工具
// ============================================================

// 总是 false 的辅助模板（用于 static_assert）
template <typename...>
constexpr bool always_false_v = false;

// 总是 true 的辅助模板
template <typename...>
constexpr bool always_true_v = true;

// ============================================================
// 编译期容量特性
// ============================================================

// 默认容量失败提示
template <typename T>
struct ct_capacity_fail {
    static_assert(always_false_v<std::remove_cvref_t<T>>,
                  "Type does not support compile-time capacity");
};

// 默认容量值 - 在 constexpr_vector.h 和 constexpr_map.h 中特化
template <typename T>
inline constexpr std::size_t ct_capacity_v = 0;

// std::array 特化
template <typename T, std::size_t N>
inline constexpr std::size_t ct_capacity_v<std::array<T, N>> = N;

// 原始数组特化
template <typename T, std::size_t N>
inline constexpr std::size_t ct_capacity_v<T[N]> = N;

// 容量特性萃取
template <typename T>
struct ct_capacity : std::integral_constant<std::size_t, ct_capacity_v<T>> {};

// 检查类型是否有编译期容量（特化后非0）
template <typename T>
concept has_ct_capacity = requires {
    { ct_capacity_v<std::remove_cvref_t<T>> } -> std::convertible_to<std::size_t>;
} && (ct_capacity_v<std::remove_cvref_t<T>> > 0);

// ============================================================
// 编译期类型特性
// ============================================================

// 检查是否是编译期值类型
template <typename T>
constexpr auto is_ct_v = false;

template <typename T, T V>
constexpr auto is_ct_v<std::integral_constant<T, V>> = true;

template <typename T>
constexpr auto is_ct_v<std::type_identity<T>> = true;

template <typename T>
constexpr auto is_ct_v<T const> = is_ct_v<T>;

// 检查是否是编译期容器
template <typename T>
constexpr auto is_ct_container_v = false;

template <typename T, std::size_t N>
constexpr auto is_ct_container_v<std::array<T, N>> = true;

// ============================================================
// 编译期整型序列
// ============================================================

template <auto... Vs>
struct value_list {
    static constexpr std::size_t size = sizeof...(Vs);
    static constexpr bool empty = size == 0;

    // 获取第 I 个值
    template <std::size_t I>
    static constexpr auto at = std::get<I>(std::tuple{Vs...});

    // 求和
    static constexpr auto sum = (Vs + ...);

    // 求积
    static constexpr auto product = (Vs * ...);


    // 转换为数组
    static constexpr auto to_array() {
        using value_type = std::common_type_t<decltype(Vs)...>;
        return std::array<value_type, size>{static_cast<value_type>(Vs)...};
    }

    // 连接
    template <auto... Us>
    using concat = value_list<Vs..., Us...>;
};

// ============================================================
// 编译期类型哈希
// ============================================================

namespace detail {

template <typename T>
consteval std::uint64_t type_hash_impl() {
    // 使用类型名生成哈希
    std::string_view name =
#if defined(_MSC_VER)
        __FUNCSIG__;
#else
        __PRETTY_FUNCTION__;
#endif
    // FNV-1a 哈希
    std::uint64_t hash = 14695981039346656037ULL;
    for (char c : name) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace detail

// 类型哈希
template <typename T>
inline constexpr std::uint64_t type_hash_v = detail::type_hash_impl<T>();

// 类型哈希萃取
template <typename T>
struct type_hash : std::integral_constant<std::uint64_t, type_hash_v<T>> {};

// ============================================================
// 类型 ID（用于反射系统）
// ============================================================

using type_id_t = std::uint64_t;

namespace detail {

inline constexpr type_id_t next_type_id() {
    // 使用原子计数器模拟（编译期）
    static type_id_t counter = 0;
    return ++counter;
}

} // namespace detail

// 编译期类型 ID
template <typename T>
inline constexpr type_id_t type_id_v = type_hash_v<T>;

// 检查两个类型是否相同（使用 ID）
template <typename T, typename U>
constexpr bool same_type_v = type_id_v<T> == type_id_v<U>;

// ============================================================
// 类型特性检测
// ============================================================

// 检测类型是否可序列化（概念）
template <typename T>
concept serializable = requires(T const& t) {
    { std::size(t) } -> std::convertible_to<std::size_t>;
    { std::data(t) } -> std::convertible_to<void const*>;
} || requires(T const& t) {
    { t.serialize() };
};

// 检测类型是否可反射
template <typename T>
concept reflectable = std::is_class_v<T> || std::is_enum_v<T>;

// 检测类型是否有默认构造函数
template <typename T>
concept default_constructible = std::is_default_constructible_v<T>;

// 检测类型是否可拷贝
template <typename T>
concept copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

// 检测类型是否可移动
template <typename T>
concept movable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

// 检测类型是否是指针
template <typename T>
constexpr bool is_pointer_v = std::is_pointer_v<T>;

template <typename T>
constexpr bool is_pointer_v<T*> = true;

template <typename T>
constexpr bool is_pointer_v<T* const> = true;

template <typename T>
constexpr bool is_pointer_v<T* volatile> = true;

template <typename T>
constexpr bool is_pointer_v<T* const volatile> = true;

// 检测类型是否是引用
template <typename T>
constexpr bool is_reference_v = std::is_reference_v<T>;

// 检测类型是否是智能指针
template <typename T>
constexpr bool is_unique_ptr_v = false;

template <typename T, typename Deleter>
constexpr bool is_unique_ptr_v<std::unique_ptr<T, Deleter>> = true;

template <typename T>
constexpr bool is_shared_ptr_v = false;

template <typename T>
constexpr bool is_shared_ptr_v<std::shared_ptr<T>> = true;

template <typename T>
constexpr bool is_smart_ptr_v = is_unique_ptr_v<T> || is_shared_ptr_v<T>;

// ============================================================
// 成员检测
// ============================================================

// 检测是否有指定成员
#define SHINE_HAS_MEMBER(NAME)                                                 \
    template <typename T, typename = void>                                     \
    struct has_##NAME : std::false_type {};                                    \
                                                                               \
    template <typename T>                                                      \
    struct has_##NAME<T, std::void_t<decltype(std::declval<T>().NAME)>>        \
        : std::true_type {};                                                   \
                                                                               \
    template <typename T>                                                      \
    inline constexpr bool has_##NAME##_v = has_##NAME<T>::value;

SHINE_HAS_MEMBER(size)
SHINE_HAS_MEMBER(data)
SHINE_HAS_MEMBER(begin)
SHINE_HAS_MEMBER(end)
SHINE_HAS_MEMBER(push_back)
SHINE_HAS_MEMBER(emplace_back)
SHINE_HAS_MEMBER(clear)
SHINE_HAS_MEMBER(resize)
SHINE_HAS_MEMBER(reserve)
SHINE_HAS_MEMBER(insert)
SHINE_HAS_MEMBER(erase)
SHINE_HAS_MEMBER(find)
SHINE_HAS_MEMBER(contains)

#undef SHINE_HAS_MEMBER

// ============================================================
// 函数特性检测
// ============================================================

// 检测是否是函数指针
template <typename T>
constexpr bool is_function_ptr_v = false;

template <typename R, typename... Args>
constexpr bool is_function_ptr_v<R(*)(Args...)> = true;

template <typename R, typename... Args>
constexpr bool is_function_ptr_v<R(*)(Args...) noexcept> = true;

// 检测是否是成员函数指针
template <typename T>
constexpr bool is_member_function_ptr_v = std::is_member_function_pointer_v<T>;

// 获取函数返回类型
template <typename T>
struct function_result;

template <typename R, typename... Args>
struct function_result<R(Args...)> { using type = R; };

template <typename R, typename... Args>
struct function_result<R(*)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct function_result<R(C::*)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct function_result<R(C::*)(Args...) const> { using type = R; };

template <typename T>
using function_result_t = typename function_result<T>::type;

// 获取函数参数类型列表
template <typename T>
struct function_args;

template <typename R, typename... Args>
struct function_args<R(Args...)> { using type = constexpr_::constexpr_type_list<Args...>; };

template <typename R, typename... Args>
struct function_args<R(*)(Args...)> { using type = constexpr_::constexpr_type_list<Args...>; };

template <typename R, typename C, typename... Args>
struct function_args<R(C::*)(Args...)> { using type = constexpr_::constexpr_type_list<Args...>; };

template <typename R, typename C, typename... Args>
struct function_args<R(C::*)(Args...) const> { using type = constexpr_::constexpr_type_list<Args...>; };

template <typename T>
using function_args_t = typename function_args<T>::type;

// 获取函数参数数量
template <typename T>
constexpr std::size_t function_arity_v = function_args_t<T>::size;

// ============================================================
// 类特性检测
// ============================================================

// 检测是否是模板实例
template <typename T>
constexpr bool is_template_v = false;

template <template <typename...> typename Template, typename... Args>
constexpr bool is_template_v<Template<Args...>> = true;

// 获取模板参数
template <typename T>
struct template_args;

template <template <typename...> typename Template, typename... Args>
struct template_args<Template<Args...>> {
    using type = constexpr_::constexpr_type_list<Args...>;
};

template <typename T>
using template_args_t = typename template_args<T>::type;

// 检测基类
template <typename Base, typename Derived>
constexpr bool is_base_of_v = std::is_base_of_v<Base, Derived>;

// 检测派生类
template <typename Derived, typename Base>
constexpr bool is_derived_from_v = std::is_base_of_v<Base, Derived>;

// 获取所有基类
template <typename T>
struct base_classes {
    using type = constexpr_::constexpr_type_list<>;
};

// ============================================================
// 编译期条件选择
// ============================================================

// 类似 std::conditional，但支持多个条件
template <auto Condition, typename TrueType, typename FalseType>
using conditional_t = std::conditional_t<static_cast<bool>(Condition), TrueType, FalseType>;

// 编译期 switch - 简化版
template <auto Value, typename Default, typename... Cases>
struct ct_switch;

// 递归终止：没有更多 case
template <auto Value, typename Default>
struct ct_switch<Value, Default> {
    using type = Default;
};

// 通用递归：检查当前 case 是否匹配
template <auto Value, typename Default, typename CaseValue, typename CaseType, typename... Rest>
struct ct_switch<Value, Default, CaseValue, CaseType, Rest...> {
    using type = CaseType;  // 简化：总是返回第一个匹配的类型
};

template <auto Value, typename Default, typename... Cases>
using ct_switch_t = typename ct_switch<Value, Default, Cases...>::type;

// ============================================================
// 编译期循环
// ============================================================

// 编译期 for 循环
template <std::size_t Start, std::size_t End, std::size_t Step = 1, typename F>
constexpr void ct_for(F&& func) {
    if constexpr (Start < End) {
        func(std::integral_constant<std::size_t, Start>{});
        ct_for<Start + Step, End, Step>(std::forward<F>(func));
    }
}

// 编译期 while 循环
template <typename Condition, typename Body>
constexpr void ct_while(Condition, Body) {
    if constexpr (Condition::value) {
        Body{};
        ct_while(Condition{}, Body{});
    }
}

// ============================================================
// 常用类型别名
// ============================================================

// 移除 cv 限定符和引用
template <typename T>
using clean_type = std::remove_cvref_t<T>;

// 检查类型是否是 const
template <typename T>
constexpr bool is_const_v = std::is_const_v<T>;

// 检查类型是否是 volatile
template <typename T>
constexpr bool is_volatile_v = std::is_volatile_v<T>;

// ============================================================
// 类型包装器
// ============================================================

// 用于传递类型而非值
template <typename T>
struct type_wrapper {
    using type = T;
    constexpr type_wrapper() = default;
};

// 类型推导辅助
template <typename T>
constexpr type_wrapper<T> type_w{};

// ============================================================
// 值包装器
// ============================================================

// 用于将值作为类型传递
template <auto V>
struct value_wrapper {
    static constexpr auto value = V;
    using value_type = decltype(V);

    constexpr operator value_type() const noexcept { return V; }
    constexpr auto operator()() const noexcept { return V; }
};

// ============================================================
// 编译期断言
// ============================================================

template <bool Condition, typename Message = void>
struct ct_assert {
    static_assert(Condition, "Compile-time assertion failed");
};

// ============================================================
// sizeof 辅助
// ============================================================

template <typename T>
inline constexpr std::size_t sizeof_v = sizeof(T);

template <typename T>
inline constexpr std::size_t alignof_v = alignof(T);

// 检查类型大小是否在指定范围内
template <typename T, std::size_t Min, std::size_t Max>
constexpr bool size_in_range_v = (sizeof(T) >= Min && sizeof(T) <= Max);

// ============================================================
// 类型标签
// ============================================================

// 用于标记类型的优先级、类别等
template <typename T, auto Tag>
struct tagged_type {
    using type = T;
    static constexpr auto tag = Tag;
};

// 提取类型标签
template <typename T>
struct get_tag;

template <typename T, auto Tag>
struct get_tag<tagged_type<T, Tag>> {
    static constexpr auto value = Tag;
};

template <typename T>
inline constexpr auto get_tag_v = get_tag<T>::value;

} // namespace shine
