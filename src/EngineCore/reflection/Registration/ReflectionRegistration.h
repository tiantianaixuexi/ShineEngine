#pragma once
#include "../Core/ReflectionCore.h"
#include "../Container/ContainerRegistry.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../../../constexpr/constexpr_map.h"
#include "../CompileTime/ReflectionModernHash.h"

namespace shine::reflection {

// 前向声明
template<typename T>
struct TypeRegistration;

// 现代化类型构建器
template<typename T>
struct TypeBuilder {
    using type = T;
    static constexpr std::string_view name = TypeIdentity<T>::name;
    static constexpr uint32_t id = TypeIdentity<T>::id;
    
    // 使用constexpr_vector存储字段信息
    template<size_t FieldCount = 16>
    using FieldVector = shine::constexpr_vector<FieldInfo, FieldCount>;
    
    // 字段注册
    template<auto MemberPtr>
    static consteval auto Field(std::string_view field_name) {
        return FieldDescriptor<MemberPtr>{field_name};
    }
    
    // 容器字段注册
    template<auto MemberPtr>
    requires (VectorLike<std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>> ||
              MapLike<std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>>)
    static consteval auto ContainerField(std::string_view field_name) {
        using MemberType = std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>;
        return ContainerFieldDescriptor<MemberPtr, MemberType>{field_name};
    }
    
    // 编译期容器字段注册
    template<auto MemberPtr>
    requires container::ContainerReflectionInfo<
        std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>
    >::is_compile_time_container
    static consteval auto CompileTimeContainerField(std::string_view field_name) {
        using MemberType = std::remove_reference_t<decltype(std::declval<T>().*MemberPtr)>;
        return CompileTimeContainerFieldDescriptor<MemberPtr, MemberType>{field_name};
    }
    
    // 方法注册（简化版本）
    template<auto MethodPtr>
    static consteval auto Method(std::string_view method_name) {
        return MethodDescriptor<MethodPtr>{method_name};
    }
};

// 字段描述符
template<auto MemberPtr>
struct FieldDescriptor {
    std::string_view name;
    
    template<typename... Attrs>
    consteval auto operator()(Attrs... attrs) const {
        return FieldRegistration<MemberPtr, Attrs...>{name, {attrs...}};
    }
};

// 容器字段描述符
template<auto MemberPtr, typename ContainerType>
struct ContainerFieldDescriptor {
    std::string_view name;
    
    template<typename... Attrs>
    consteval auto operator()(Attrs... attrs) const {
        return ContainerFieldRegistration<MemberPtr, ContainerType, Attrs...>{name, {attrs...}};
    }
};

// 编译期容器字段描述符
template<auto MemberPtr, typename ContainerType>
struct CompileTimeContainerFieldDescriptor {
    std::string_view name;
    
    template<typename... Attrs>
    consteval auto operator()(Attrs... attrs) const {
        return CompileTimeContainerFieldRegistration<MemberPtr, ContainerType, Attrs...>{name, {attrs...}};
    }
};

// 方法描述符
template<auto MethodPtr>
struct MethodDescriptor {
    std::string_view name;
    
    template<typename... Attrs>
    consteval auto operator()(Attrs... attrs) const {
        return MethodRegistration<MethodPtr, Attrs...>{name, {attrs...}};
    }
};

// 属性标记
struct Editable {};
struct ReadOnly {};
struct ScriptReadable {};
struct ScriptWritable {};
struct Transient {};
struct Required {};

// 字段注册器
template<auto MemberPtr, typename... Attrs>
struct FieldRegistration {
    std::string_view name;
    std::tuple<Attrs...> attributes;
    
    template<size_t FieldCount>
    static consteval auto ApplyTo(FieldBuilder<FieldCount>& builder) {
        constexpr size_t offset = offsetof(typename TypeBuilder<typename TypeOfMemberPtr<MemberPtr>::type>::type, 
                                          TypeOfMemberPtr<MemberPtr>::member_name);
        
        auto field_info = BuildFieldInfo<offset>();
        builder.AddField(field_info);
        return builder;
    }
    
private:
    template<size_t Offset>
    static consteval FieldInfo BuildFieldInfo() {
        using ClassType = typename TypeOfMemberPtr<MemberPtr>::type;
        using MemberType = typename MemberTypeOfPtr<MemberPtr>::type;
        
        FieldInfo field{};
        field.typeId = TypeIdentity<MemberType>::id;
        field.containerType = ContainerType::None;
        field.offset = Offset;
        field.size = sizeof(MemberType);
        field.alignment = alignof(MemberType);
        field.isPod = TypeIdentity<MemberType>::is_pod;
        field.name = name;
        field.flags = BuildFlags<Attrs...>();
        
        // 设置优化的函数指针
        field.getterFn = [](const void* instance, void* out_value, size_t offset, size_t size) {
            const auto* obj = static_cast<const ClassType*>(instance);
            *static_cast<MemberType*>(out_value) = obj->*MemberPtr;
        };
        
        field.setterFn = [](void* instance, const void* in_value, size_t offset, size_t size) {
            auto* obj = static_cast<ClassType*>(instance);
            obj->*MemberPtr = *static_cast<const MemberType*>(in_value);
        };
        
        field.equalsFn = [](const void* a, const void* b, size_t size) {
            return *static_cast<const MemberType*>(a) == *static_cast<const MemberType*>(b);
        };
        
        field.copyFn = [](void* dst, const void* src, size_t size) {
            *static_cast<MemberType*>(dst) = *static_cast<const MemberType*>(src);
        };
        
        return field;
    }
    
    template<typename... Flags>
    static consteval PropertyFlags BuildFlags() {
        PropertyFlags flags = PropertyFlags::None;
        ((flags |= ConvertAttribute<Flags>()), ...);
        return flags;
    }
    
    template<typename Attr>
    static consteval PropertyFlags ConvertAttribute() {
        if constexpr (std::is_same_v<Attr, Editable>) {
            return PropertyFlags::Editable;
        } else if constexpr (std::is_same_v<Attr, ReadOnly>) {
            return PropertyFlags::ReadOnly;
        } else if constexpr (std::is_same_v<Attr, ScriptReadable>) {
            return PropertyFlags::ScriptReadable;
        } else if constexpr (std::is_same_v<Attr, ScriptWritable>) {
            return PropertyFlags::ScriptWritable;
        } else if constexpr (std::is_same_v<Attr, Transient>) {
            return PropertyFlags::Transient;
        } else if constexpr (std::is_same_v<Attr, Required>) {
            return PropertyFlags::Required;
        } else {
            return PropertyFlags::None;
        }
    }
};

// 辅助类型特征
template<auto MemberPtr>
struct TypeOfMemberPtr;

template<typename Class, typename Member, Member Class::*Ptr>
struct TypeOfMemberPtr<Ptr> {
    using type = Class;
    static constexpr auto member_name = Ptr;
};

template<auto MemberPtr>
struct MemberTypeOfPtr;

template<typename Class, typename Member, Member Class::*Ptr>
struct MemberTypeOfPtr<Ptr> {
    using type = Member;
};

// 字段构建器
template<size_t MaxFields>
struct FieldBuilder {
    shine::constexpr_vector<FieldInfo, MaxFields> fields{};
    size_t count = 0;
    
    consteval void AddField(const FieldInfo& field) {
        if (count < MaxFields) {
            fields[count++] = field;
        }
    }
    
    consteval auto GetFields() const {
        return fields;
    }
    
    consteval size_t GetCount() const {
        return count;
    }
};

} // namespace shine::reflection

// 主要注册宏
#define REFLECTION_TYPE(Type) \
    template<> \
    struct shine::reflection::TypeRegistration<Type> { \
        static constexpr auto Build() { \
            return shine::reflection::TypeBuilder<Type>{}; \
        } \
    };

#define REFLECT_FIELD(Member, ...) \
    .Field<&Type::Member>(#Member)(__VA_ARGS__)

#define REFLECT_CONTAINER(Member, ...) \
    .ContainerField<&Type::Member>(#Member)(__VA_ARGS__)

#define REFLECT_COMPILETIME_CONTAINER(Member, ...) \
    .CompileTimeContainerField<&Type::Member>(#Member)(__VA_ARGS__)

#define REFLECT_METHOD(Member, ...) \
    .Method<&Type::Member>(#Member)(__VA_ARGS__)

#define END_REFLECTION ;