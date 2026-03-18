// =============================================================================
// ConstexprOffset.h — Compile-time member name and runtime offset
// =============================================================================
//
// 提供成员名编译期获取、类型名编译期获取
// 偏移量因 MSVC 限制使用运行时计算
// =============================================================================

#pragma once

#include <cstddef>
#include <tuple>
#include "string/shine_text_view.h"

namespace shine::reflection {

// =============================================================================
// Type Utilities
// =============================================================================

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// =============================================================================
// Compile-Time Type Name (MSVC)
// =============================================================================

namespace detail {

template <typename T>
constexpr shine::STextView get_raw_name_impl() {
#if defined(_MSC_VER)
    return shine::STextView::from_cstring(__FUNCSIG__);
#else
    return shine::STextView::from_cstring(__PRETTY_FUNCTION__);
#endif
}

} // namespace detail

// 获取类型名（编译期）
template <typename T>
inline constexpr shine::STextView type_string() {
    constexpr shine::STextView sample = detail::get_raw_name_impl<int>();
    constexpr size_t prefix_length = sample.find("int");
    constexpr shine::STextView str = detail::get_raw_name_impl<T>();
    constexpr size_t suffix_length = sample.size() - prefix_length - 3;
    constexpr auto name = 
        str.substr(prefix_length, str.size() - prefix_length - suffix_length);
    return name;
}

// =============================================================================
// Compile-time member name extraction
// =============================================================================

namespace detail {

// 从成员指针提取类型
template <typename Member, typename Base>
std::tuple<Member, Base> get_types(Member Base::*);

} // namespace detail

// =============================================================================
// Public API
// =============================================================================

// 编译期获取成员名 - 使用 __FUNCSIG__
template <auto member_ptr>
inline constexpr shine::STextView member_name() {
#if defined(_MSC_VER)
    constexpr shine::STextView sig = shine::STextView::from_cstring(__FUNCSIG__);
    
    // 查找 "member_name<" 之后的内容
    constexpr shine::STextView prefix = shine::STextView::from_literal("member_name<");
    constexpr auto prefix_pos = sig.find(prefix);
    if (prefix_pos == shine::STextView::npos) return {};
    
    constexpr auto start = prefix_pos + prefix.size();
    constexpr auto end = sig.find('>', start);
    if (end == shine::STextView::npos) return {};
    
    // 提取如 "&Vec3::x>" 的部分
    constexpr auto raw = sig.substr(start, end - start);
    
    // 找到 "::" 后的部分
    constexpr auto colon_pos = raw.find("::");
    if (colon_pos == shine::STextView::npos) return {};
    
    // 返回成员名 (去掉末尾的 >)
    constexpr auto name = raw.substr(colon_pos + 2);
    return name;
#else
    return {};
#endif
}

// 运行时计算偏移量 - 使用参数而非模板参数
template <typename C, typename M>
inline std::size_t compute_offset(M C::* ptr) {
    // 零开销计算，避免栈分配
    return static_cast<std::size_t>(
        reinterpret_cast<const char*>(&(reinterpret_cast<C*>(0)->*ptr)) - reinterpret_cast<const char*>(0));
}



} // namespace shine::reflection
