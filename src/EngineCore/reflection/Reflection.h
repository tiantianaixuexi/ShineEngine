#pragma once

// =============================================================================
// Reflection.h — Shine Runtime Reflection System (Aggregated Header)
// =============================================================================
//
// This is the main public API header that includes all reflection components.
// Users should only need to include this single header to use the reflection system.
//
// Component Structure:
// ├── Views/           - View interfaces (Inspector, Script, ECS)
// ├── DSL/             - Domain Specific Language (Field/Method/Type builders)
// ├── Script/          - Script integration (values and bridges)
// ├── Core/            - Core components (macros and helpers)
// ├── ReflectionCore.h - Core type definitions
// ├── TypeRegistry.h   - Runtime type registration
// └── ReflectionError.h - Error handling
//
// C++23 / MSVC
// =============================================================================

// Core components (must be included first)
#include "ReflectionCore.h"
#include "ReflectionError.h"
#include "TypeRegistry.h"

// Core shared components
#include "Core/TypeView.h"

// View system
#include "Views/InspectorView.h"
#include "Views/ScriptView.h"
#include "Views/ECSView.h"

// DSL system
#include "DSL/FieldDSL.h"
#include "DSL/MethodDSL.h"
#include "DSL/TypeBuilder.h"

// Script integration
#include "Script/ScriptValue.h"
#include "Script/ScriptBridge.h"

// Core components
#include "Core/ReflectionHelpers.h"
#include "Core/ReflectionMacros.h"
#include "Core/ConstexprOffset.h"

// =============================================================================
// Compile-Time Registration Support
// =============================================================================

// 编译期反射注册宏（集成到现有系统）
#define SHINE_REFLECT_CT_BEGIN(type_name) \
    namespace shine { namespace reflection { \
    template<> struct ConstexprRegistration<type_name> { \
        static consteval auto Register() { \
            auto info = ::shine::reflection::ConstexprTypeInfo<type_name>::Create(#type_name);

#define SHINE_REFLECT_CT_FIELD_IMPL(type_name, field_name) \
            info.AddField<decltype(type_name::field_name)>(#field_name);

#define SHINE_REFLECT_CT_FIELD(field_name) \
            SHINE_REFLECT_CT_FIELD_IMPL(CompileTimeTestStruct, field_name)

#define SHINE_REFLECT_CT_END(type_name) \
            return info; \
        } \
    }; }}

// 编译期类型信息构建器
template<typename T>
struct ConstexprRegistration {
    static consteval auto Register() {
        return ::shine::reflection::ConstexprTypeInfo<T>{};
    }
};

// 编译期字段创建助手 - 简化版本
consteval auto MakeConstexprField(const char* name, uint32_t type_id) {
    return ::shine::reflection::ConstexprFieldInfo{
        name,
        type_id,
        0,  // offset - 需要在运行时确定
        0,  // size - 需要在运行时确定
        0,  // alignment - 需要在运行时确定
        false  // isPod - 需要在运行时确定
    };
}

// 编译期类型信息构建
template<typename T>
consteval auto BuildConstexprTypeInfo(const char* name) {
    return ::shine::reflection::ConstexprTypeInfo<T>{
        ::shine::reflection::GetTypeId<T>(),
        name,
        sizeof(T),
        alignof(T),
        std::is_enum_v<T>,
        std::is_trivially_copyable_v<T>
    };
}
