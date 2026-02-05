#pragma once
#include <array>
#include "../Core/ReflectionModernTypes.h"
#include "../../../constexpr/constexpr_map.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../CompileTime/ReflectionModernHash.h"
#include "../Registration/ReflectionRegistration.h"

namespace shine::reflection {

// 类型注册信息存储 - 使用constexpr容器
template<typename T, size_t MaxFields = 32>
struct TypeRegistrationInfo {
    static constexpr std::string_view name = TypeIdentity<T>::name;
    static constexpr uint32_t id = TypeIdentity<T>::id;
    static constexpr size_t size = sizeof(T);
    static constexpr size_t alignment = alignof(T);
    static constexpr bool is_pod = TypeIdentity<T>::is_pod;
    static constexpr bool is_empty = TypeIdentity<T>::is_empty;
    
    // 使用constexpr_vector存储字段信息
    using FieldVector = shine::constexpr_vector<FieldInfo, MaxFields>;
    FieldVector fields{};
    size_t field_count = 0;
    
    // 方法信息（简化版）
    using MethodVector = shine::constexpr_vector<MethodInfo, 16>;
    MethodVector methods{};
    size_t method_count = 0;
    
    // 编译期字段查找
    consteval const FieldInfo* FindField(std::string_view field_name) const {
        constexpr auto field_hash = compile_time::FNV1aHash(field_name);
        for (size_t i = 0; i < field_count; ++i) {
            if (compile_time::FNV1aHash(fields[i].name) == field_hash) {
                return &fields[i];
            }
        }
        return nullptr;
    }
    
    // 编译期字段按索引访问
    consteval const FieldInfo* GetField(size_t index) const {
        if (index < field_count) {
            return &fields[index];
        }
        return nullptr;
    }
    
    // 编译期方法查找
    consteval const MethodInfo* FindMethod(std::string_view method_name) const {
        constexpr auto method_hash = compile_time::FNV1aHash(method_name);
        for (size_t i = 0; i < method_count; ++i) {
            if (compile_time::FNV1aHash(methods[i].name) == method_hash) {
                return &methods[i];
            }
        }
        return nullptr;
    }
    
    // 添加字段
    consteval void AddField(const FieldInfo& field) {
        if (field_count < MaxFields) {
            fields[field_count++] = field;
        }
    }
    
    // 添加方法
    consteval void AddMethod(const MethodInfo& method) {
        if (method_count < 16) {
            methods[method_count++] = method;
        }
    }
};

// 全局类型注册表 - 使用constexpr_map
class GlobalTypeRegistry {
private:
    // 使用constexpr_map存储所有注册的类型信息
    template<size_t Capacity = 256>
    using TypeMap = shine::constexpr_map<uint32_t, std::string_view, Capacity>;
    
    static consteval auto BuildTypeMap() {
        TypeMap<> map{};
        // 预注册基本类型
        map.insert(TypeIdentity<int>::id, TypeIdentity<int>::name);
        map.insert(TypeIdentity<unsigned int>::id, TypeIdentity<unsigned int>::name);
        map.insert(TypeIdentity<long>::id, TypeIdentity<long>::name);
        map.insert(TypeIdentity<unsigned long>::id, TypeIdentity<unsigned long>::name);
        map.insert(TypeIdentity<long long>::id, TypeIdentity<long long>::name);
        map.insert(TypeIdentity<unsigned long long>::id, TypeIdentity<unsigned long long>::name);
        map.insert(TypeIdentity<float>::id, TypeIdentity<float>::name);
        map.insert(TypeIdentity<double>::id, TypeIdentity<double>::name);
        map.insert(TypeIdentity<long double>::id, TypeIdentity<long double>::name);
        map.insert(TypeIdentity<bool>::id, TypeIdentity<bool>::name);
        map.insert(TypeIdentity<char>::id, TypeIdentity<char>::name);
        map.insert(TypeIdentity<unsigned char>::id, TypeIdentity<unsigned char>::name);
        map.insert(TypeIdentity<short>::id, TypeIdentity<short>::name);
        map.insert(TypeIdentity<unsigned short>::id, TypeIdentity<unsigned short>::name);
        map.insert(TypeIdentity<std::string>::id, TypeIdentity<std::string>::name);
        map.insert(TypeIdentity<std::string_view>::id, TypeIdentity<std::string_view>::name);
        return map;
    }
    
    static constexpr auto type_map = BuildTypeMap();

public:
    template<typename T>
    static consteval const TypeRegistrationInfo<T>& Register() {
        static_assert(std::is_standard_layout_v<T>, "Only standard layout types supported");
        static_assert(!std::is_union_v<T>, "Unions not supported");
        return GetInstance<T>();
    }
    
    template<typename T>
    static consteval const TypeRegistrationInfo<T>& GetTypeInfo() {
        return GetInstance<T>();
    }
    
    static consteval bool IsTypeRegistered(uint32_t type_id) {
        return type_map.contains(type_id);
    }
    
    static consteval std::string_view GetTypeName(uint32_t type_id) {
        if (auto it = type_map.find(type_id); it != type_map.end()) {
            return it->second;
        }
        return "";
    }
    
    // 获取所有注册的类型数量
    static consteval size_t GetRegisteredTypeCount() {
        return type_map.size();
    }
    
    // 编译期类型验证
    template<typename T>
    static consteval bool ValidateType() {
        if constexpr (requires { TypeRegistration<T>::Build(); }) {
            return true;
        } else {
            return false;
        }
    }

private:
    template<typename T>
    static consteval TypeRegistrationInfo<T>& GetInstance() {
        static TypeRegistrationInfo<T> instance{};
        return instance;
    }
};

// 编译期注册触发器
template<typename T>
consteval bool TriggerRegistration() {
    [[maybe_unused]] const auto& info = GlobalTypeRegistry::Register<T>();
    return true;
}

// 类型注册助手
template<typename T>
struct TypeRegistrar {
    template<typename Builder>
    static consteval auto RegisterFields(Builder& builder) {
        if constexpr (requires { TypeRegistration<T>::Build(); }) {
            return TypeRegistration<T>::Build()(builder);
        } else {
            return builder; // 没有注册信息时返回原始构建器
        }
    }
    
    static consteval const TypeRegistrationInfo<T>& GetRegistrationInfo() {
        return GlobalTypeRegistry::GetTypeInfo<T>();
    }
};

// 编译期类型构建器
template<typename T, size_t MaxFields = 32>
struct CompileTimeTypeBuilder {
    using RegistrationInfo = TypeRegistrationInfo<T, MaxFields>;
    RegistrationInfo info{};
    
    consteval CompileTimeTypeBuilder() {
        info.field_count = 0;
        info.method_count = 0;
    }
    
    template<auto MemberPtr>
    consteval auto& Field(std::string_view name) {
        constexpr size_t offset = GetMemberOffset<MemberPtr>();
        constexpr auto field_info = BuildFieldInfo<MemberPtr, offset>(name);
        AddField(field_info);
        return *this;
    }
    
    template<auto MemberPtr>
    requires container::ContainerReflectionInfo<
        std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>
    >::is_compile_time_container
    consteval auto& ContainerField(std::string_view name) {
        constexpr size_t offset = GetMemberOffset<MemberPtr>();
        auto field_info = container::ContainerFieldInfoBuilder<
            std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>
        >::BuildFieldInfo(name, offset);
        AddField(field_info);
        return *this;
    }
    
    consteval const RegistrationInfo& Build() const {
        return info;
    }

private:
    template<auto MemberPtr>
    static consteval size_t GetMemberOffset() {
        return offsetof(T, TypeOfMemberPtr<MemberPtr>::member_name);
    }
    
    template<auto MemberPtr, size_t Offset>
    static consteval FieldInfo BuildFieldInfo(std::string_view name) {
        using MemberType = typename MemberTypeOfPtr<MemberPtr>::type;
        
        FieldInfo field{};
        field.typeId = TypeIdentity<MemberType>::id;
        field.containerType = ContainerType::None;
        field.offset = Offset;
        field.size = sizeof(MemberType);
        field.alignment = alignof(MemberType);
        field.isPod = TypeIdentity<MemberType>::is_pod;
        field.name = name;
        field.flags = PropertyFlags::None;
        
        // 设置优化的函数指针
        field.getterFn = [](const void* instance, void* out_value, size_t offset, size_t size) {
            const auto* obj = static_cast<const T*>(instance);
            *static_cast<MemberType*>(out_value) = obj->*MemberPtr;
        };
        
        field.setterFn = [](void* instance, const void* in_value, size_t offset, size_t size) {
            auto* obj = static_cast<T*>(instance);
            obj->*MemberPtr = *static_cast<const MemberType*>(in_value);
        };
        
        return field;
    }
    
    consteval void AddField(const FieldInfo& field) {
        if (info.field_count < MaxFields) {
            info.fields[info.field_count++] = field;
        }
    }
};

// 便利的类型注册函数
template<typename T>
consteval const TypeRegistrationInfo<T>& RegisterType() {
    return GlobalTypeRegistry::Register<T>();
}

template<typename T>
consteval const TypeRegistrationInfo<T>& GetTypeInfo() {
    return GlobalTypeRegistry::GetTypeInfo<T>();
}

} // namespace shine::reflection