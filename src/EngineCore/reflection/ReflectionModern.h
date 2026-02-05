#pragma once

// =============================================================================
// Modern Reflection System - 现代反射系统
// =============================================================================
//
// 这是一个基于C++23/26标准的现代化反射系统，具有以下特点：
// - 纯编译期计算，零运行时开销
// - 支持STL容器(vector, map, pair等)和自定义容器的反射
// - 简洁直观的注册API
// - 高性能的类型擦除和访问机制
// - 完全利用现有的编译期容器基础设施
//
// 使用示例：
// ```
// struct MyStruct {
//     int id;
//     std::string name;
//     std::vector<float> values;
//     shine::constexpr_map<std::string, int, 8> config;
// };
//
// REFLECTION_TYPE(MyStruct)
//     REFLECT_FIELD(id, Editable, ScriptReadable)
//     REFLECT_FIELD(name, Editable, ScriptReadable)
//     REFLECT_CONTAINER(values, Editable)
//     REFLECT_COMPILETIME_CONTAINER(config, Editable)
// END_REFLECTION
//
// // 使用反射API
// MyStruct obj{42, "test", {1.0f, 2.0f}, {{"key", 100}}};
// auto& id = refl_get(obj, id);  // 获取字段
// refl_set(obj, name, "new_name");  // 设置字段
// auto size = refl_container_size(obj.values);  // 容器大小
// ```

#include "Core/ReflectionModernTypes.h"
#include "CompileTime/ReflectionModernHash.h"
#include "Container/ContainerTraits.h"
#include "Container/ContainerRegistry.h"
#include "Registration/ReflectionRegistration.h"
#include "Registration/TypeRegistry.h"
#include "ReflectionAPI.h"

// 便利的命名空间别名
namespace rf = shine::reflection;
namespace rfc = shine::reflection::compile_time;
namespace rfc_container = shine::reflection::container;

// 核心类型别名
template<typename T>
using ReflectionType = shine::reflection::ReflectedType<T>;

template<typename T>
constexpr auto reflected = shine::reflection::reflected<T>{};

// 编译期常量
constexpr auto kMaxFieldsPerType = 32;
constexpr auto kMaxMethodsPerType = 16;
constexpr auto kMaxRegisteredTypes = 256;

// 版本信息
constexpr auto kReflectionVersion = "2.0.0";
constexpr auto kReflectionVersionMajor = 2;
constexpr auto kReflectionVersionMinor = 0;
constexpr auto kReflectionVersionPatch = 0;

#endif // SHINE_REFLECTION_MODERN_H