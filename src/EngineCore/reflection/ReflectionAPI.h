#pragma once
#include "Registration/ReflectionRegistration.h"
#include "Registration/TypeRegistry.h"
#include "../Field/FieldAccess.h"
#include "../constexpr/constexpr_vector.h"
#include "../constexpr/constexpr_map.h"
#include "CompileTime/ReflectionModernHash.h"

namespace shine::reflection {

// 简单的反射访问API
template<typename T>
class ReflectedType {
public:
    static constexpr std::string_view name = TypeIdentity<T>::name;
    static constexpr uint32_t id = TypeIdentity<T>::id;
    static constexpr size_t size = TypeIdentity<T>::size;
    static constexpr size_t alignment = TypeIdentity<T>::alignment;
    static constexpr bool is_pod = TypeIdentity<T>::is_pod;
    
    // 获取类型信息
    static const TypeRegistrationInfo<T>& GetInfo() {
        return GlobalTypeRegistry::GetTypeInfo<T>();
    }
    
    // 字段访问
    template<auto MemberPtr>
    static auto& GetField(T& obj) {
        return obj.*MemberPtr;
    }
    
    template<auto MemberPtr>
    static const auto& GetField(const T& obj) {
        return obj.*MemberPtr;
    }
    
    // 字段设置
    template<auto MemberPtr>
    static void SetField(T& obj, const auto& value) {
        obj.*MemberPtr = value;
    }
    
    // 容器字段访问
    template<auto MemberPtr>
    requires VectorLike<std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>>
    static auto& GetContainer(T& obj) {
        return obj.*MemberPtr;
    }
    
    template<auto MemberPtr>
    requires MapLike<std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>>
    static auto& GetContainer(T& obj) {
        return obj.*MemberPtr;
    }
    
    // 编译期容器访问
    template<auto MemberPtr>
    requires container::ContainerReflectionInfo<
        std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>
    >::is_compile_time_container
    static constexpr auto GetCompileTimeContainer(const T& obj) {
        const auto& container = obj.*MemberPtr;
        return container::ContainerFieldAccessor<
            std::remove_reference_t<decltype(container)>
        >::GetCompileTimeView(container);
    }
    
    // 字段信息获取
    static consteval const FieldInfo* FindField(std::string_view field_name) {
        return GetInfo().FindField(field_name);
    }
    
    static consteval const FieldInfo* GetFieldByIndex(size_t index) {
        return GetInfo().GetField(index);
    }
    
    static consteval size_t GetFieldCount() {
        return GetInfo().field_count;
    }
    
    // 编译期字段遍历
    template<typename Func>
    static constexpr void ForEachField(Func&& func) {
        constexpr auto& info = GetInfo();
        for (size_t i = 0; i < info.field_count; ++i) {
            func(info.GetField(i));
        }
    }
    
    // 编译期方法遍历
    template<typename Func>
    static constexpr void ForEachMethod(Func&& func) {
        constexpr auto& info = GetInfo();
        for (size_t i = 0; i < info.method_count; ++i) {
            func(info.FindMethod(info.methods[i].name));
        }
    }
    
    // 类型验证
    static consteval bool IsValid() {
        return GlobalTypeRegistry::ValidateType<T>();
    }
    
    // 创建实例
    static T Create() requires std::is_default_constructible_v<T> {
        return T{};
    }
    
    // 复制实例
    static T Clone(const T& source) requires std::is_copy_constructible_v<T> {
        return T{source};
    }
};

// 便利的类型别名
template<typename T>
using reflected = ReflectedType<T>;

// 编译期反射助手
struct CompileTimeReflection {
    // 编译期类型检查
    template<typename T>
    static consteval bool IsReflected() {
        return GlobalTypeRegistry::ValidateType<T>();
    }
    
    // 编译期字段计数
    template<typename T>
    static consteval size_t FieldCount() {
        if constexpr (IsReflected<T>()) {
            return GlobalTypeRegistry::GetTypeInfo<T>().field_count;
        } else {
            return 0;
        }
    }
    
    // 编译期方法计数
    template<typename T>
    static consteval size_t MethodCount() {
        if constexpr (IsReflected<T>()) {
            return GlobalTypeRegistry::GetTypeInfo<T>().method_count;
        } else {
            return 0;
        }
    }
    
    // 编译期类型验证
    template<typename T>
    static consteval bool Validate() {
        static_assert(std::is_standard_layout_v<T>, "Type must be standard layout");
        static_assert(!std::is_union_v<T>, "Unions not supported");
        static_assert(sizeof(T) > 0, "Type must have non-zero size");
        return true;
    }
    
    // 获取类型名称
    template<typename T>
    static consteval std::string_view GetTypeName() {
        return TypeIdentity<T>::name;
    }
    
    // 获取类型ID
    template<typename T>
    static consteval uint32_t GetTypeId() {
        return TypeIdentity<T>::id;
    }
};

// 运行时反射助手
struct RuntimeReflection {
    // 运行时类型检查
    template<typename T>
    static bool IsReflected() {
        return GlobalTypeRegistry::IsTypeRegistered(TypeIdentity<T>::id);
    }
    
    // 获取类型信息
    template<typename T>
    static const TypeRegistrationInfo<T>& GetTypeInfo() {
        return GlobalTypeRegistry::GetTypeInfo<T>();
    }
    
    // 查找字段（运行时）
    template<typename T>
    static const FieldInfo* FindField(const T& obj, std::string_view field_name) {
        const auto& info = GetTypeInfo<T>();
        return info.FindField(field_name);
    }
    
    // 获取字段值（运行时）
    template<typename T>
    static bool GetFieldValue(const T& obj, std::string_view field_name, void* out_value) {
        const FieldInfo* field = FindField<T>(obj, field_name);
        if (!field) return false;
        
        field->Get(&obj, out_value);
        return true;
    }
    
    // 设置字段值（运行时）
    template<typename T>
    static bool SetFieldValue(T& obj, std::string_view field_name, const void* value) {
        const FieldInfo* field = FindField<T>(obj, field_name);
        if (!field) return false;
        
        field->Set(&obj, value);
        return true;
    }
};

// 容器操作助手
struct ContainerHelpers {
    // Vector操作
    template<VectorLike Container>
    static constexpr size_t GetSize(const Container& container) {
        return container.size();
    }
    
    template<VectorLike Container>
    static constexpr bool IsEmpty(const Container& container) {
        return container.empty();
    }
    
    template<VectorLike Container>
    static constexpr void Clear(Container& container) {
        container.clear();
    }
    
    // Map操作
    template<MapLike Container>
    static constexpr size_t GetSize(const Container& container) {
        return container.size();
    }
    
    template<MapLike Container>
    static constexpr bool IsEmpty(const Container& container) {
        return container.empty();
    }
    
    template<MapLike Container>
    static constexpr bool Contains(const Container& container, const typename Container::key_type& key) {
        return container.find(key) != container.end();
    }
    
    template<MapLike Container>
    static constexpr void Clear(Container& container) {
        container.clear();
    }
    
    // 编译期容器操作
    template<typename Container>
    requires container::IsCompileTimeContainer<Container>::value
    static constexpr auto GetView(const Container& container) {
        return container.view();
    }
};

} // namespace shine::reflection

// 用户友好的宏接口
#define refl_type(Type) shine::reflection::reflected<Type>
#define refl_get(obj, Member) decltype(obj)::GetField<&std::remove_reference_t<decltype(obj)>::Member>(obj)
#define refl_set(obj, Member, value) decltype(obj)::SetField<&std::remove_reference_t<decltype(obj)>::Member>(obj, value)
#define refl_container(obj, Member) decltype(obj)::GetContainer<&std::remove_reference_t<decltype(obj)>::Member>(obj)
#define refl_ct_container(obj, Member) decltype(obj)::GetCompileTimeContainer<&std::remove_reference_t<decltype(obj)>::Member>(obj)

// 编译期检查宏
#define refl_validate(Type) static_assert(shine::reflection::CompileTimeReflection::Validate<Type>())
#define refl_is_reflected(Type) shine::reflection::CompileTimeReflection::IsReflected<Type>()
#define refl_field_count(Type) shine::reflection::CompileTimeReflection::FieldCount<Type>()
#define refl_method_count(Type) shine::reflection::CompileTimeReflection::MethodCount<Type>()
#define refl_type_name(Type) shine::reflection::CompileTimeReflection::GetTypeName<Type>()
#define refl_type_id(Type) shine::reflection::CompileTimeReflection::GetTypeId<Type>()

// 运行时检查宏
#define refl_runtime_is_reflected(Type) shine::reflection::RuntimeReflection::IsReflected<Type>()
#define refl_runtime_get_info(Type) shine::reflection::RuntimeReflection::GetTypeInfo<Type>()

// 容器操作宏
#define refl_container_size(container) shine::reflection::ContainerHelpers::GetSize(container)
#define refl_container_empty(container) shine::reflection::ContainerHelpers::IsEmpty(container)
#define refl_container_clear(container) shine::reflection::ContainerHelpers::Clear(container)
#define refl_map_contains(container, key) shine::reflection::ContainerHelpers::Contains(container, key)
#define refl_ct_view(container) shine::reflection::ContainerHelpers::GetView(container)