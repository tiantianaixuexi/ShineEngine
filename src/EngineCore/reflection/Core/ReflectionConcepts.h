#pragma once

#include <concepts>
#include <type_traits>
#include <vector>
#include <list>
#include <map>
#include <deque>
#include <unordered_map>

#include "ReflectionModernTypes.h"

namespace shine::reflection {

    // 类型别名
    template<typename T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

    // 枚举类型概念
    template<typename T>
    concept EnumType = std::is_enum_v<T>;

    // =============================================================================
    // 基础概念约束
    // =============================================================================

    // 反射注册概念 - 类型必须满足的基本要求
    template<typename T>
    concept ReflectionRegistrable = 
        std::is_class_v<T> &&
        std::is_standard_layout_v<T> &&
        !std::is_union_v<T>;

    // 可复制概念 - 支持反射的类型必须可复制
    template<typename T>
    concept ReflectionCopyable = 
        std::is_copy_constructible_v<T> &&
        std::is_copy_assignable_v<T>;

    // 可默认构造概念
    template<typename T>
    concept DefaultConstructible = std::is_default_constructible_v<T>;

    // 可销毁概念
    template<typename T>
    concept Destructible = std::is_destructible_v<T>;

    // =============================================================================
    // 容器类型概念
    // =============================================================================

    // 序列容器概念
    template<typename T>
    concept SequenceContainer = 
        requires(T container) {
            typename T::value_type;
            typename T::size_type;
            typename T::iterator;
            typename T::const_iterator;
            { container.size() } -> std::convertible_to<typename T::size_type>;
            { container.begin() } -> std::convertible_to<typename T::iterator>;
            { container.end() } -> std::convertible_to<typename T::iterator>;
        } &&
        (std::same_as<T, std::vector<typename T::value_type>> ||
         std::same_as<T, std::list<typename T::value_type>> ||
         std::same_as<T, std::deque<typename T::value_type>>);

    // 关联容器概念
    template<typename T>
    concept AssociativeContainer = 
        requires(T container, const typename T::key_type& key) {
            typename T::key_type;
            typename T::mapped_type;
            typename T::value_type;
            typename T::size_type;
            typename T::iterator;
            { container.size() } -> std::convertible_to<typename T::size_type>;
            { container.find(key) } -> std::convertible_to<typename T::iterator>;
            { container.end() } -> std::convertible_to<typename T::iterator>;
        } &&
        (std::same_as<T, std::map<typename T::key_type, typename T::mapped_type>> ||
         std::same_as<T, std::unordered_map<typename T::key_type, typename T::mapped_type>>);

    // 数组概念
    template<typename T>
    concept ArrayType = std::is_array_v<T>;

    // =============================================================================
    // 函数和成员指针概念
    // =============================================================================

    // 函数指针概念
    template<typename T>
    concept FunctionPointerType = 
        std::is_function_v<std::remove_pointer_t<std::remove_cv_t<T>>> &&
        std::is_pointer_v<T>;

    // 成员函数指针概念
    template<typename T>
    concept MemberFunctionPointerType = std::is_member_function_pointer_v<T>;

    // 成员对象指针概念
    template<typename T>
    concept MemberObjectPointerType = std::is_member_object_pointer_v<T>;

    // 可调用概念
    template<typename T, typename... Args>
    concept InvocableWith = std::invocable<T, Args...>;

    // =============================================================================
    // 数值类型概念
    // =============================================================================

    // 整数类型概念
    template<typename T>
    concept IntegerType = std::integral<T> && !std::same_as<T, bool>;

    // 浮点类型概念
    template<typename T>
    concept FloatingPointType = std::floating_point<T>;

    // 算术类型概念
    template<typename T>
    concept ArithmeticType = std::is_arithmetic_v<T>;

    // 数值类型概念（包括枚举）
    template<typename T>
    concept NumericType = ArithmeticType<T> || std::is_enum_v<T>;

    // =============================================================================
    // 字符串和文本概念
    // =============================================================================

    // 字符类型概念
    template<typename T>
    concept CharacterType = 
        std::same_as<T, char> ||
        std::same_as<T, wchar_t> ||
        std::same_as<T, char8_t> ||
        std::same_as<T, char16_t> ||
        std::same_as<T, char32_t>;

    // 字符串视图概念
    template<typename T>
    concept StringViewType = 
        std::same_as<T, std::string_view> ||
        std::same_as<T, std::wstring_view> ||
        requires(T t) {
            { t.data() } -> std::convertible_to<const char*>;
            { t.size() } -> std::convertible_to<size_t>;
        };

    // 字符串类型概念
    template<typename T>
    concept StringType = 
        std::same_as<T, std::string> ||
        std::same_as<T, std::wstring> ||
        StringViewType<T> ||
        requires(T t) {
            { t.c_str() } -> std::convertible_to<const char*>;
            { t.length() } -> std::convertible_to<size_t>;
        };

    // =============================================================================
    // 智能指针概念
    // =============================================================================

    // 智能指针概念
    template<typename T>
    concept SmartPointerType = 
        requires(T ptr) {
            typename T::element_type;
            { ptr.get() } -> std::convertible_to<typename T::element_type*>;
            { *ptr } -> std::convertible_to<typename T::element_type&>;
        } &&
        (std::same_as<T, std::unique_ptr<typename T::element_type>> ||
         std::same_as<T, std::shared_ptr<typename T::element_type>> ||
         std::same_as<T, std::weak_ptr<typename T::element_type>>);

    // 可空指针概念
    template<typename T>
    concept NullablePointer = 
        std::is_pointer_v<T> || SmartPointerType<T>;

    // =============================================================================
    // 反射特定概念
    // =============================================================================

    // 可反射字段概念
    template<typename T>
    concept ReflectableField = 
        ReflectionRegistrable<remove_cvref_t<T>> ||
        ArithmeticType<T> ||
        StringType<T> ||
        SequenceContainer<T> ||
        AssociativeContainer<T> ||
        std::is_enum_v<T>;

    // 可反射方法概念
    template<typename T>
    concept ReflectableMethod = 
        FunctionPointerType<T> ||
        MemberFunctionPointerType<T>;

    // 可序列化概念
    template<typename T>
    concept Serializable = 
        ReflectionCopyable<T> &&
        (ArithmeticType<T> || 
         StringType<T> || 
         SequenceContainer<T> || 
         AssociativeContainer<T> ||
         ReflectionRegistrable<T>);
    
    // POD类型概念
    template<typename T>
    concept PodType = std::is_trivially_copyable_v<T>;

    // =============================================================================
    // 编译期辅助概念
    // =============================================================================

    // 编译期已知大小概念
    template<typename T>
    concept KnownSizeAtCompileTime = 
        sizeof(T) > 0 && 
        alignof(T) > 0;

    // 编译期可构造概念
    template<typename T>
    concept CompileTimeConstructible = 
        std::is_constructible_v<T> &&
        !std::is_abstract_v<T>;

    // 编译期可比较概念
    template<typename T>
    concept CompileTimeComparable = 
        requires(const T& a, const T& b) {
            { a == b } -> std::convertible_to<bool>;
            { a != b } -> std::convertible_to<bool>;
        };

    // =============================================================================
    // 组合概念
    // =============================================================================

    // 完整的反射支持概念
    template<typename T>
    concept FullyReflectable = 
        ReflectionRegistrable<T> &&
        ReflectionCopyable<T> &&
        Destructible<T> &&
        KnownSizeAtCompileTime<T>;

    // 高性能反射概念（POD类型）
    template<typename T>
    concept HighPerformanceReflectable = 
        FullyReflectable<T> &&
        PodType<T> &&
        sizeof(T) <= REFLECTION_SMALL_OBJECT_THRESHOLD;

    // 容器反射概念
    template<typename T>
    concept ContainerReflectable = 
        SequenceContainer<T> || 
        AssociativeContainer<T> ||
        ArrayType<T>;

} // namespace shine::reflection