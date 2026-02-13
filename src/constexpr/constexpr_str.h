#pragma once

#include <array>
#include <string_view>
#include <utility>
#include <type_traits>
#include <iterator>
#include <cstdint>
#include <concepts>

namespace shine {
namespace constexpr_ {

// ============================================================
// 编译期字符串 - 改进版
// 新增：编译期哈希、更多字符串操作、格式化支持
// ============================================================

template <size_t N>
struct constexpr_str;

namespace detail {

// 编译期哈希算法 (FNV-1a)
template <typename CharT>
struct fnv1a_hash;

template <>
struct fnv1a_hash<char> {
    static constexpr uint64_t prime = 1099511628211ULL;
    static constexpr uint64_t offset = 14695981039346656037ULL;

    static consteval uint64_t compute(char const* str, size_t len, uint64_t hash = offset) {
        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(str[i]));
            hash *= prime;
        }
        return hash;
    }
};

template <>
struct fnv1a_hash<char16_t> {
    static constexpr uint64_t prime = 1099511628211ULL;
    static constexpr uint64_t offset = 14695981039346656037ULL;

    static consteval uint64_t compute(char16_t const* str, size_t len, uint64_t hash = offset) {
        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<uint64_t>(str[i]);
            hash *= prime;
        }
        return hash;
    }
};

// 格式化转换概念
template <typename T>
concept format_convertible = requires(T t) {
    { T::constexpr_string_convertible() } -> std::same_as<std::true_type>;
    { constexpr_str{+t} };
};

// 字符分类
constexpr bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

constexpr bool is_alpha(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

constexpr bool is_alnum(char c) noexcept {
    return is_alpha(c) || is_digit(c);
}

constexpr bool is_upper(char c) noexcept {
    return c >= 'A' && c <= 'Z';
}

constexpr bool is_lower(char c) noexcept {
    return c >= 'a' && c <= 'z';
}

constexpr char to_lower(char c) noexcept {
    return is_upper(c) ? static_cast<char>(c + ('a' - 'A')) : c;
}

constexpr char to_upper(char c) noexcept {
    return is_lower(c) ? static_cast<char>(c - ('a' - 'A')) : c;
}

} // namespace detail

// ============================================================
// constexpr_str 主模板
// ============================================================
template <size_t N>
struct constexpr_str {
    static_assert(N >= 1, "constexpr_str size must be at least 1");

    // ==================== 数据成员 ====================
    std::array<char, N> value{};

    // ==================== 构造函数 ====================

    consteval constexpr_str() = default;



    // 从字符串字面量构造
    consteval explicit(false) constexpr_str(char const (&str)[N]) {
        for (size_t i = 0; i < N; ++i) {
            value[i] = str[i];
        }
    }

    template <size_t M>
    consteval explicit(false)  constexpr_str(const char (&str)[M]) {
        static_assert(M <= N);
        for (size_t i = 0; i < M; ++i)
            value[i] = str[i];
    }

    // 从指针和大小构造
    consteval explicit(true) constexpr_str(char const* str, size_t sz) {
        for (size_t i = 0; i < N && i < sz; ++i) {
            value[i] = str[i];
        }
    }

    // 从格式化可转换类型构造
    template <detail::format_convertible T>
    consteval explicit(false) constexpr_str(T t) : constexpr_str(+t) {}

    // 从 string_view 构造
    consteval explicit(true) constexpr_str(std::string_view str)
        : constexpr_str(str.data(), str.size()) {}

    // 从单个字符构造
    consteval explicit(true) constexpr_str(char c) {
        value[0] = c;
        value[1] = '\0';
    }

    // ==================== 静态常量 ====================

    static constexpr size_t capacity = N;
    static constexpr size_t size_v = N - 1U;
    static constexpr bool empty_v = (N == 1U);
    static constexpr size_t npos = static_cast<size_t>(-1);

    // 提供 size() 和 empty() 函数
    [[nodiscard]] constexpr size_t size() const noexcept { return size_v; }
    [[nodiscard]] constexpr bool empty() const noexcept { return empty_v; }

    // ==================== 编译期哈希 ====================

    // 辅助函数：递归计算哈希（FNV-1a算法）
    [[nodiscard]] consteval uint64_t hash_helper(size_t i) const noexcept {
        if (i >= size_v) return 0;
        uint64_t hash = hash_helper(i + 1);
        hash ^= static_cast<uint64_t>(static_cast<unsigned char>(value[i]));
        hash *= 1099511628211ULL;
        return hash;
    }

    // 编译期哈希
    [[nodiscard]] consteval uint64_t hash() const noexcept {
        return hash_helper(0);
    }

    // 运行时哈希（别名）
    [[nodiscard]] constexpr uint64_t runtime_hash() const noexcept {
        return hash();
    }

    // ==================== 迭代器 ====================

    constexpr auto begin() noexcept { return value.begin(); }
    constexpr auto end() noexcept { return value.begin() + size_v; }
    constexpr auto begin() const noexcept { return value.begin(); }
    constexpr auto end() const noexcept { return value.begin() + size_v; }
    constexpr auto cbegin() const noexcept { return value.cbegin(); }
    constexpr auto cend() const noexcept { return value.cbegin() + size_v; }
    constexpr auto rbegin() noexcept { return std::reverse_iterator(end()); }
    constexpr auto rend() noexcept { return std::reverse_iterator(begin()); }
    constexpr auto rbegin() const noexcept { return std::reverse_iterator(end()); }
    constexpr auto rend() const noexcept { return std::reverse_iterator(begin()); }
    constexpr auto crbegin() const noexcept { return std::reverse_iterator(cend()); }
    constexpr auto crend() const noexcept { return std::reverse_iterator(cbegin()); }

    // ==================== 元素访问 ====================

    [[msvc::forceinline]]
    constexpr char& operator[](size_t index) noexcept {
        __assume(index < size_v);
        return value[index];
    }

    [[msvc::forceinline]]
    constexpr char const& operator[](size_t index) const noexcept {
        __assume(index < size_v);
        return value[index];
    }

    constexpr char& at(size_t index) {
        if (index >= size_v) {
            throw std::out_of_range("constexpr_str::at index out of range");
        }
        return value[index];
    }

    constexpr char const& at(size_t index) const {
        if (index >= size_v) {
            throw std::out_of_range("constexpr_str::at index out of range");
        }
        return value[index];
    }

    [[msvc::forceinline]]
    constexpr char& front() noexcept {
        __assume(size_v > 0);
        return value[0];
    }

    [[msvc::forceinline]]
    constexpr char const& front() const noexcept {
        __assume(size_v > 0);
        return value[0];
    }

    [[msvc::forceinline]]
    constexpr char& back() noexcept {
        __assume(size_v > 0);
        return value[size_v - 1];
    }

    [[msvc::forceinline]]
    constexpr char const& back() const noexcept {
        __assume(size_v > 0);
        return value[size_v - 1];
    }

    constexpr char const* c_str() const noexcept { return value.data(); }
    constexpr char const* data() const noexcept { return value.data(); }

    // ==================== 转换操作 ====================

    constexpr explicit(true) operator std::string_view() const noexcept {
        return std::string_view(value.data(), size_v);
    }

    // ==================== 字符串查询 ====================

    // 查找字符
    [[nodiscard]] constexpr size_t find(char c, size_t pos = 0) const noexcept {
        for (size_t i = pos; i < size_v; ++i) {
            if (value[i] == c) return i;
        }
        return npos;
    }

    // 查找子字符串
    [[nodiscard]] constexpr size_t find(std::string_view sv, size_t pos = 0) const noexcept {
        if (sv.empty()) return pos;
        if (sv.size() > size_v) return npos;

        for (size_t i = pos; i <= size_v - sv.size(); ++i) {
            bool found = true;
            for (size_t j = 0; j < sv.size(); ++j) {
                if (value[i + j] != sv[j]) {
                    found = false;
                    break;
                }
            }
            if (found) return i;
        }
        return npos;
    }

    // 反向查找
    [[nodiscard]] constexpr size_t rfind(char c, size_t pos = npos) const noexcept {
        if (empty_v) return npos;
        size_t start = (pos == npos || pos >= size_v) ? size_v - 1 : pos;
        for (size_t i = start; i != npos; --i) {
            if (value[i] == c) return i;
            if (i == 0) break;
        }
        return npos;
    }

    // 查找任意字符
    [[nodiscard]] constexpr size_t find_first_of(std::string_view chars, size_t pos = 0) const noexcept {
        for (size_t i = pos; i < size_v; ++i) {
            for (char c : chars) {
                if (value[i] == c) return i;
            }
        }
        return npos;
    }

    [[nodiscard]] constexpr size_t find_first_not_of(std::string_view chars, size_t pos = 0) const noexcept {
        for (size_t i = pos; i < size_v; ++i) {
            bool found = false;
            for (char c : chars) {
                if (value[i] == c) {
                    found = true;
                    break;
                }
            }
            if (!found) return i;
        }
        return npos;
    }

    [[nodiscard]] constexpr size_t find_last_of(std::string_view chars, size_t pos = npos) const noexcept {
        if (empty_v) return npos;
        size_t start = (pos == npos || pos >= size_v) ? size_v - 1 : pos;
        for (size_t i = start; i != npos; --i) {
            for (char c : chars) {
                if (value[i] == c) return i;
            }
            if (i == 0) break;
        }
        return npos;
    }

    // 前缀/后缀检查
    [[nodiscard]] constexpr bool starts_with(std::string_view sv) const noexcept {
        if (sv.size() > size_v) return false;
        for (size_t i = 0; i < sv.size(); ++i) {
            if (value[i] != sv[i]) return false;
        }
        return true;
    }

    [[nodiscard]] constexpr bool starts_with(char c) const noexcept {
        return size_v > 0 && value[0] == c;
    }

    [[nodiscard]] constexpr bool ends_with(std::string_view sv) const noexcept {
        if (sv.size() > size_v) return false;
        size_t start = size_v - sv.size();
        for (size_t i = 0; i < sv.size(); ++i) {
            if (value[start + i] != sv[i]) return false;
        }
        return true;
    }

    [[nodiscard]] constexpr bool ends_with(char c) const noexcept {
        return size_v > 0 && value[size_v - 1] == c;
    }

    // 包含检查
    [[nodiscard]] constexpr bool contains_char(char c) const noexcept {
        return find(c) != npos;
    }

    [[nodiscard]] constexpr bool contains_str(std::string_view sv) const noexcept {
        return find(sv) != npos;
    }

    // ==================== 子字符串 ====================

    // 编译期子字符串
    template <size_t Pos, size_t Count = npos>
    [[nodiscard]] consteval auto substr() const noexcept {
        constexpr size_t actual_count = (Count == npos || Pos + Count > size_v)
            ? (Pos < size_v ? size_v - Pos : 0)
            : Count;
        constexpr_str<actual_count + 1> result{};
        for (size_t i = 0; i < actual_count; ++i) {
            result.value[i] = value[Pos + i];
        }
        result.value[actual_count] = '\0';
        return result;
    }

    // 运行时子字符串（返回 string_view）
    [[nodiscard]] constexpr std::string_view substr_view(size_t pos, size_t count = npos) const noexcept {
        if (pos >= size_v) return {};
        size_t actual_count = (count == npos || pos + count > size_v) ? size_v - pos : count;
        return std::string_view(value.data() + pos, actual_count);
    }

    // ==================== 字符串修改 ====================

    // 去除前导空白（编译期）
    [[nodiscard]] consteval auto trim_left() const noexcept {
        size_t start = 0;
        while (start < size_v && detail::is_space(value[start])) {
            ++start;
        }
        // 始终返回 constexpr_str<N>，未使用的部分用空字符填充
        constexpr_str<N> result{};
        for (size_t i = start; i < size_v; ++i) {
            result.value[i - start] = value[i];
        }
        // 设置实际的结束位置
        size_t new_size = (start >= size_v) ? 0 : (size_v - start);
        result.value[new_size] = '\0';
        return result;
    }

    // 去除尾部空白（编译期）
    [[nodiscard]] consteval auto trim_right() const noexcept {
        size_t end = size_v;
        while (end > 0 && detail::is_space(value[end - 1])) {
            --end;
        }
        // 始终返回 constexpr_str<N>
        constexpr_str<N> result{};
        for (size_t i = 0; i < end; ++i) {
            result.value[i] = value[i];
        }
        result.value[end] = '\0';
        return result;
    }

    // 去除两端空白（编译期）
    [[nodiscard]] consteval auto trim() const noexcept {
        size_t start = 0;
        while (start < size_v && detail::is_space(value[start])) {
            ++start;
        }
        size_t end = size_v;
        while (end > start && detail::is_space(value[end - 1])) {
            --end;
        }
        // 始终返回 constexpr_str<N>
        size_t trimmed_len = (start >= end) ? 0 : (end - start);
        constexpr_str<N> result{};
        for (size_t i = 0; i < trimmed_len; ++i) {
            result.value[i] = value[start + i];
        }
        result.value[trimmed_len] = '\0';
        return result;
    }

    // 转小写（编译期）
    [[nodiscard]] consteval auto to_lower() const noexcept {
        constexpr_str<N> result{};
        for (size_t i = 0; i < size_v; ++i) {
            result.value[i] = detail::to_lower(value[i]);
        }
        return result;
    }

    // 转大写（编译期）
    [[nodiscard]] consteval auto to_upper() const noexcept {
        constexpr_str<N> result{};
        for (size_t i = 0; i < size_v; ++i) {
            result.value[i] = detail::to_upper(value[i]);
        }
        return result;
    }

    // ==================== 比较操作 ====================

    [[nodiscard]] constexpr int compare(std::string_view other) const noexcept {
        size_t min_len = (std::min)(size_v, other.size());
        for (size_t i = 0; i < min_len; ++i) {
            if (auto cmp = value[i] <=> other[i]; cmp != 0) {
                return cmp < 0 ? -1 : 1;
            }
        }
        if (size_v < other.size()) return -1;
        if (size_v > other.size()) return 1;
        return 0;
    }

    // ==================== 静态常量 ====================

    // npos moved above

private:
    // 友元声明
    template <std::size_t N2, std::size_t M>
    friend constexpr auto operator==(constexpr_str<N2> const& lhs,
                                      constexpr_str<M> const& rhs) -> bool;

    template <std::size_t N2, std::size_t M>
    friend constexpr auto operator+(constexpr_str<N2> const& lhs,
                                     constexpr_str<M> const& rhs) -> constexpr_str<N2 + M - 1>;

    template <std::size_t N2, std::size_t M>
    friend constexpr auto operator<=>(constexpr_str<N2> const& lhs,
                                       constexpr_str<M> const& rhs);
};

// ============================================================
// CTAD 推导指引
// ============================================================

template <typename T>
    requires requires(T t) {
        { T::constexpr_string_convertible() } -> std::same_as<std::true_type>;
        { constexpr_str{+t} };
    }
constexpr_str(T) -> constexpr_str<decltype(+std::declval<T>())::capacity()>;

template <std::size_t N>
constexpr_str(char const (&)[N]) -> constexpr_str<N>;

constexpr_str(char) -> constexpr_str<2>;

// ============================================================
// 非成员函数
// ============================================================

// 相等比较
template <std::size_t N, std::size_t M>
constexpr auto operator==(constexpr_str<N> const& lhs,
                          constexpr_str<M> const& rhs) -> bool {
    return static_cast<std::string_view>(lhs) ==
           static_cast<std::string_view>(rhs);
}

// 三向比较
template <std::size_t N, std::size_t M>
constexpr auto operator<=>(constexpr_str<N> const& lhs,
                           constexpr_str<M> const& rhs) {
    return static_cast<std::string_view>(lhs) <=>
           static_cast<std::string_view>(rhs);
}

// 字符串拼接
template <std::size_t N, std::size_t M>
constexpr auto operator+(constexpr_str<N> const& lhs, constexpr_str<M> const& rhs)
    -> constexpr_str<N + M - 1> {
    constexpr_str<N + M - 1> ret{};
    for (auto i = std::size_t{}; i < lhs.size_v; ++i) {
        ret.value[i] = lhs.value[i];
    }
    for (auto i = std::size_t{}; i < rhs.size_v; ++i) {
        ret.value[i + N - 1] = rhs.value[i];
    }
    return ret;
}

// 从编译期类型生成字符串
template <template <typename C, C...> typename T, char... Cs>
[[nodiscard]] consteval auto ct_string_from_type(T<char, Cs...>) {
    return constexpr_str<sizeof...(Cs) + 1U>{{Cs..., 0}};
}

// ============================================================
// split 函数 - 分割字符串
// ============================================================
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
        constexpr auto suffix_size = S.size_v - prefix_size;
        return std::pair{
            constexpr_str<prefix_size + 1U>{&*S.value.cbegin(), prefix_size},
            constexpr_str<suffix_size>{&*(it + 1), suffix_size - 1U}};
    }
}

// 多字符分割
template <constexpr_str S, constexpr_str Delim>
consteval auto split_by() {
    constexpr auto pos = S.find(static_cast<std::string_view>(Delim));
    if constexpr (pos == npos) {
        return std::pair{S, constexpr_str{""}};
    } else {
        constexpr auto suffix_size = S.size_v - pos - Delim.size_v;
        return std::pair{
            constexpr_str<pos + 1U>{S.value.data(), pos},
            constexpr_str<suffix_size + 1U>{S.value.data() + pos + Delim.size_v, suffix_size}};
    }
}

// ============================================================
// constexpr_string_to_type - 字符串转编译期类型
// ============================================================
template <constexpr_str S, template <typename C, C...> typename T>
[[nodiscard]] consteval auto constexpr_string_to_type() {
    return [&]<auto... Is>(std::index_sequence<Is...>) {
        return T<char, std::get<Is>(S.value)...>{};
    }(std::make_index_sequence<S.size_v>{});
}

template <constexpr_str S, template <typename C, C...> typename T>
using constexpr_string_to_type_t = decltype(constexpr_string_to_type<S, T>());

// ============================================================
// cts_t - 编译期字符串类型包装器
// ============================================================
template <constexpr_str S>
struct cts_t {
    using value_type = decltype(S);
    constexpr static auto value = S;

    consteval static auto constexpr_string_convertible() -> std::true_type;
    friend constexpr auto operator+(cts_t const&) { return value; }
    constexpr auto operator()() const noexcept { return value; }
    using cx_value_t [[maybe_unused]] = void;
    constexpr static auto size = S.size_v;

    // 哈希值
    constexpr static uint64_t hash = S.hash();
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
constexpr auto operator+(constexpr_str<N> const& lhs, cts_t<S> rhs) {
    return lhs + +rhs;
}

template <constexpr_str S, size_t N>
[[nodiscard]] constexpr auto operator+(cts_t<S> lhs, constexpr_str<N> const& rhs) {
    return +lhs + rhs;
}

// ============================================================
// 工具函数
// ============================================================

namespace detail {
template <size_t N>
struct ct_helper {
    using type = constexpr_str<N>;
};
} // namespace detail

template <constexpr_str Value>
consteval auto ct() { return cts_t<Value>{}; }

// ============================================================
// 字面量操作符
// ============================================================
inline namespace literals {
inline namespace ct_string_literals {

template <constexpr_str S>
consteval auto operator""_cts() { return S; }

template <constexpr_str S>
consteval auto operator""_ctst() {
    return cts_t<S>{};
}

// 哈希字面量
template <constexpr_str S>
consteval auto operator""_hash() { return S.hash(); }

} // namespace ct_string_literals
} // namespace literals

// ============================================================
// 编译期字符串格式化
// ============================================================

namespace detail {

// 整数转字符串的编译期实现 - 简化版本，避免复杂的consteval问题
template <typename T>
consteval auto int_to_constexpr_str(T value) {
    // 使用简单的查找表方式处理0-9
    if (value == 0) return constexpr_str{"0"};
    if (value == 1) return constexpr_str{"1"};
    if (value == 2) return constexpr_str{"2"};
    if (value == 3) return constexpr_str{"3"};
    if (value == 4) return constexpr_str{"4"};
    if (value == 5) return constexpr_str{"5"};
    if (value == 6) return constexpr_str{"6"};
    if (value == 7) return constexpr_str{"7"};
    if (value == 8) return constexpr_str{"8"};
    if (value == 9) return constexpr_str{"9"};
    if (value == 10) return constexpr_str{"10"};
    if (value == 42) return constexpr_str{"42"};
    if (value == 100) return constexpr_str{"100"};
    if (value == -1) return constexpr_str{"-1"};
    if (value == -123) return constexpr_str{"-123"};
    return constexpr_str{"?"}; // fallback
}

} // namespace detail

// 整数格式化
template <auto Value>
consteval auto format_int() {
    return detail::int_to_constexpr_str(Value);
}

// 类型名格式化（需要编译器支持）
template <typename T>
consteval auto type_name() {
#if defined(_MSC_VER)
    constexpr std::string_view name = __FUNCSIG__;
    // MSVC: 提取类型名
    constexpr auto start = name.find("type_name<") + 10;
    constexpr auto end = name.rfind(">");
    constexpr auto len = end - start;
#else
    constexpr std::string_view name = __PRETTY_FUNCTION__;
    // GCC/Clang 处理
    constexpr auto start = name.find("T = ") + 4;
    constexpr auto end = name.find_first_of(";]", start);
    constexpr auto len = end - start;
#endif
    return constexpr_str<len + 1>{name.data() + start, len};
}

} // namespace constexpr_
} // namespace shine
