#pragma once

// =============================================================================
// ReflectionMacros.h — Macro Definitions for Reflection System
// =============================================================================
//
// Contains all reflection macros for type, field, and method registration.
// These macros provide the declarative interface for the reflection system.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include "../TypeRegistry.h"
#include "../DSL/TypeBuilder.h"

namespace shine::reflection {

// Forward declarations for macro expansion
template <typename T>
constexpr TypeInfo BuildTypeInfo(shine::STextView name);

} // namespace shine::reflection

// =============================================================================
// Main Registration Macros
// =============================================================================

#define REFLECTION_STRUCT(Type)                                                 \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder);                             \
    template<typename _RB>                                                      \
    inline void _ReflectStaticBuild(_RB& builder, Type*) {                      \
        _ReflectRegFn_##Type(builder);                                          \
    }                                                                           \
    inline auto _ReflectInit_##Type = []() {                                    \
        auto _reflectGraphFn = []<typename _RB>(_RB& builder) {                 \
            _ReflectRegFn_##Type(builder);                                      \
        };                                                                      \
        auto _graph = shine::reflection::BuildTypeRegistrationGraph<Type>(      \
            shine::STextView::from_literal(#Type),                              \
            _reflectGraphFn,                                                    \
            std::is_enum_v<Type>);                                              \
        shine::reflection::TypeInfo _info = _graph.BuildTypeInfo(_reflectGraphFn); \
        (void)shine::reflection::TypeRegistry::Get().Register(std::move(_info));\
        return true;                                                            \
    }();                                                                        \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder)

// REFLECTION_REGISTER is now integrated into REFLECTION_STRUCT
// Kept for backward compatibility
#define REFLECTION_REGISTER(Type) /* Deprecated: Use REFLECTION_STRUCT only */

#define REFLECTION_STRUCT_AUTO(Type)                                            \
    REFLECTION_STRUCT(Type)

// =============================================================================
// Field and Method Registration Macros
// =============================================================================

#define REFLECT_FIELD(Member)                                                   \
    builder.RegisterFieldFromDSL(                                               \
        shine::reflection::DSL::FieldDSLNode<                                   \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member      \
        >(#Member))


// 优化的方法注册 - 使用模板特化减少虚函数调用开销 (支持链式调用)
#define REFLECT_METHOD_FAST(Member)                                             \
    shine::reflection::FastMethodRegistration<                                  \
        &std::remove_reference_t<decltype(builder)>::ObjectType::Member        \
    >::Register(builder, #Member)

// 普通方法注册 (支持链式调用)
#define REFLECT_METHOD(Member)                                                  \
    builder.RegisterMethodFromDSL(                                              \
        shine::reflection::DSL::MethodDSLNode<                                   \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member      \
        >(#Member))

// =============================================================================
// Enum Registration Macros
// =============================================================================

#define REFLECT_ENUM(Type)                                                      \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder);                             \
    inline auto _ReflectInit_##Type = []() {                                    \
        auto _reflectGraphFn = []<typename _RB>(_RB& builder) {                 \
            _ReflectRegFn_##Type(builder);                                      \
        };                                                                      \
        auto _graph = shine::reflection::BuildTypeRegistrationGraph<Type>(      \
            shine::STextView::from_literal(#Type),                              \
            _reflectGraphFn,                                                    \
            true);                                                              \
        shine::reflection::TypeInfo _info = _graph.BuildTypeInfo(_reflectGraphFn); \
        shine::reflection::TypeRegistry::Get().Register(std::move(_info));      \
        return true;                                                            \
    }();                                                                        \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder)
