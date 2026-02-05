#pragma once


#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>



#include "Field/FieldInfo.h"
#include "ReflectionFlags.h"
#include "ReflectionHash.h"
#include "ReflectionUI.h"
#include "ReflectionError.h"

// -----------------------------------------------------------------------------
// Views (Inspector / Script / ECS)
// -----------------------------------------------------------------------------
namespace shine::reflection {

struct TypeView {
    const TypeInfo *typeInfo = nullptr;
    bool            IsValid() const { return typeInfo != nullptr; }
};

struct InspectorView : public TypeView {
    struct FieldIterator {
        const TypeInfo  *type;
        size_t           index;
        bool             operator!=(const FieldIterator &other) const { return index != other.index; }
        void             operator++() { ++index; }
        const FieldInfo &operator*() const { return type->fields[index]; }
    };
    FieldIterator begin() const { return {typeInfo, 0}; }
    FieldIterator end() const { return {typeInfo, typeInfo->fields.size()}; }

    bool IsEditable(const FieldInfo &field) const {
        return HasFlag(field.flags, ReflectionFlag::EditAnywhere) && !HasFlag(field.flags, ReflectionFlag::ReadOnly);
    }
    const UI::Schema &GetUISchema(const FieldInfo &field) const { return field.uiSchema; }

    bool IsVisible(const FieldInfo &field, const void *instance) const {
        const MetadataValue *condMeta = field.GetMeta(Hash("EditCondition"));
        if (condMeta && std::holds_alternative<std::string_view>(*condMeta)) {
            std::string_view condField = std::get<std::string_view>(*condMeta);

            const FieldInfo *condInfo = typeInfo->FindField(condField);
            if (condInfo && condInfo->typeId == GetTypeId<bool>()) {
                bool bVal = false;
                condInfo->Get(instance, &bVal);
                if (!bVal)
                    return false;
            }
        }
        return true;
    }

    std::string_view GetCategory(const FieldInfo &field) const {
        const MetadataValue *catMeta = field.GetMeta(Hash("Category"));
        if (catMeta && std::holds_alternative<std::string_view>(*catMeta)) {
            return std::get<std::string_view>(*catMeta);
        }
        return "";
    }

    void SetValue(void *instance, const FieldInfo &field, const void *value) const {
        if (IsEditable(field))
            field.Set(instance, value);
    }
};

struct ScriptView : public TypeView {
    static const TypeInfo *GetTypeInfo(TypeId id) { return TypeRegistry::Get().Find(id); }

    const FieldInfo *GetFieldInfo(std::string_view name) const { return typeInfo->FindField(name); }
    const FieldInfo *GetFieldInfo(size_t index) const {
        if (index < typeInfo->fields.size())
            return &typeInfo->fields[index];
        return nullptr;
    }

    ScriptValue GetField(void *instance, const FieldInfo *field, const ScriptBridge &bridge) const {
        if (!field || !HasFlag(field->flags, ReflectionFlag::ScriptRead))
            return ScriptValue();
            
        // 使用内存池优化分配
        constexpr size_t STACK_BUFFER_SIZE = 64;
        alignas(16) char stackBuffer[STACK_BUFFER_SIZE];
        
        const size_t align = field->alignment > 0 ? field->alignment : 8;
        const bool useStack = (field->size <= STACK_BUFFER_SIZE && align <= 16);
        
        void* storage = nullptr;
        std::unique_ptr<shine::reflection::PoolAllocator> heapAllocator;
        
        if (useStack) {
            storage = stackBuffer;
        } else {
            // 使用反射内存池分配
            heapAllocator = std::make_unique<shine::reflection::PoolAllocator>(field->size, align);
            storage = heapAllocator->Get();
            if (!storage) {
                return ScriptValue(); // 分配失败
            }
        }

        if (!field->isPod) {
            Construct(storage, field->typeId);
        }

        field->Get(instance, storage);
        ScriptValue result = bridge.ToScript(storage, field->typeId);

        if (!field->isPod) {
            Destruct(storage, field->typeId);
        }
        
        // heapAllocator会在析构时自动释放内存
        return result;
    }

    void SetField(void *instance, const FieldInfo *field, const ScriptValue &value, const ScriptBridge &bridge) const {
        if (!field || !HasFlag(field->flags, ReflectionFlag::ScriptWrite))
            return;
            
        // 使用内存池优化分配
        constexpr size_t STACK_BUFFER_SIZE = 64;
        alignas(16) char stackBuffer[STACK_BUFFER_SIZE];
        
        const size_t align = field->alignment > 0 ? field->alignment : 8;
        const bool useStack = (field->size <= STACK_BUFFER_SIZE && align <= 16);
        
        void* storage = nullptr;
        std::unique_ptr<shine::reflection::PoolAllocator> heapAllocator;
        
        if (useStack) {
            storage = stackBuffer;
        } else {
            // 使用反射内存池分配
            heapAllocator = std::make_unique<shine::reflection::PoolAllocator>(field->size, align);
            storage = heapAllocator->Get();
            if (!storage) {
                return; // 分配失败
            }
        }

        if (!field->isPod) {
            Construct(storage, field->typeId);
        }

        bridge.FromScript(value, storage, field->typeId);
        field->Set(instance, storage);

        if (!field->isPod) {
            Destruct(storage, field->typeId);
        }
        
        // heapAllocator会在析构时自动释放内存
    }

    ScriptValue GetField(void *instance, std::string_view name, const ScriptBridge &bridge) const { return GetField(instance, GetFieldInfo(name), bridge); }
    void        SetField(void *instance, std::string_view name, const ScriptValue &val, const ScriptBridge &bridge) const { SetField(instance, GetFieldInfo(name), val, bridge); }

    const MethodInfo *GetMethodInfo(std::string_view name) const { return typeInfo->FindMethod(name); }
    const MethodInfo *GetMethodInfo(size_t index) const {
        if (index < typeInfo->methods.size())
            return &typeInfo->methods[index];
        return nullptr;
    }

    ScriptValue CallMethod(void *instance, const MethodInfo *method, const std::vector<ScriptValue> &args, const ScriptBridge &bridge) const {
        if (!method || !HasFlag(method->flags, FunctionFlags::ScriptCallable))
            return ScriptValue();
        if (args.size() != method->paramTypes.size())
            return ScriptValue();

        // 使用作用域内存分配器管理所有临时分配
        shine::reflection::ScopedMemoryAllocator scopedAllocator;
        
        std::vector<void*> rawArgs(args.size());
        std::vector<std::unique_ptr<shine::reflection::PoolAllocator>> argAllocators;
        argAllocators.reserve(args.size());

        // 分配参数内存
        for (size_t i = 0; i < args.size(); ++i) {
            const TypeInfo *pType = GetTypeInfo(method->paramTypes[i]);
            if (!pType)
                return ScriptValue();

            auto allocator = std::make_unique<shine::reflection::PoolAllocator>(pType->size, pType->alignment);
            void* argPtr = allocator->Get();
            if (!argPtr) {
                return ScriptValue(); // 分配失败
            }
            
            argAllocators.push_back(std::move(allocator));
            rawArgs[i] = argPtr;
            bridge.FromScript(args[i], argPtr, method->paramTypes[i]);
        }

        // 分配返回值内存
        const TypeInfo *rType = (method->returnType != GetTypeId<void>()) ? GetTypeInfo(method->returnType) : nullptr;
        std::unique_ptr<shine::reflection::PoolAllocator> returnAllocator;
        void* retPtr = nullptr;
        
        if (rType) {
            returnAllocator = std::make_unique<shine::reflection::PoolAllocator>(rType->size, rType->alignment);
            retPtr = returnAllocator->Get();
            if (!retPtr) {
                return ScriptValue(); // 分配失败
            }
        }

        method->Invoke(instance, rawArgs.data(), retPtr);

        ScriptValue result;
        if (rType && retPtr) {
            result = bridge.ToScript(retPtr, method->returnType);
        }
        
        // 所有分配器会在作用域结束时自动清理
        return result;
    }

    ScriptValue CallMethod(void *instance, std::string_view name, const std::vector<ScriptValue> &args, const ScriptBridge &bridge) const {
        return CallMethod(instance, GetMethodInfo(name), args, bridge);
    }
};

struct ECSView {
    struct ComponentLayout {
        size_t          size;
        size_t          alignment;
        const TypeInfo *layoutSource;
    };
    ComponentLayout layout;
    size_t          GetSize() const { return layout.size; }
    size_t          GetAlignment() const { return layout.alignment; }
};

} // namespace shine::reflection

// -----------------------------------------------------------------------------
// Macros (compile-time registration only)
// -----------------------------------------------------------------------------

template <typename Builder>
using BuilderType = typename std::remove_reference_t<Builder>::ObjectType;

#define REFLECTION_REGISTER(Type)                                      \
    inline constexpr auto Type##_CT =                                   \
        shine::reflection::BuildTypeInfoCT<Type>(#Type);                \
    inline constinit auto Type##_Reg = []() {                            \
        shine::reflection::TypeRegistry::Get().Register(Type##_CT);     \
        return true;                                                    \
    }();

#define REFLECTION_REGISTER_LIMITS(Type, LimitsType)                    \
    inline constexpr auto Type##_CT =                                   \
        shine::reflection::BuildTypeInfoCT<Type, LimitsType>(#Type);    \
    inline constinit auto Type##_Reg = []() {                            \
        shine::reflection::TypeRegistry::Get().Register(Type##_CT);     \
        return true;                                                    \
    }();

#define REFLECTION_STRUCT(Type)                                                     \
    template<auto F>  struct Type##Register {                                       \
        static constexpr decltype(auto) call(auto &&args) {                         \
            return F(std::forward<decltype(args)>(args));                           \
        }                                                                           \
    };                                                                              \
    constexpr void Type##RegisterReflection(shine::reflection::TypeBuilder<Type> &builder);  \
    template <>                                                                     \
    struct shine::reflection::detail::ReflectionRegister<Type> {                    \
        static constexpr bool Exists = true;                                        \
        template <typename Limits>                                                  \
        static constexpr void Call(shine::reflection::TypeBuilder<Type, Limits> &builder) {\
            Type##RegisterReflection(builder);                                      \
        }                                                                           \
    };                                                                              \
    constexpr void Type##RegisterReflection(shine::reflection::TypeBuilder<Type> &builder)

#define REFLECT_FIELD(Member)                 \
    builder.RegisterFieldFromDSL(             \
        shine::reflection::DSL::FieldDSLNode< \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member>(#Member))

#define REFLECT_METHOD(Member)                 \
    builder.RegisterMethodFromDSL(             \
        shine::reflection::DSL::MakeMethodDSL< \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member>(#Member))

#define REFLECT_ENUM(Type)                                                           \
    constexpr void Type##_Reflect(shine::reflection::TypeBuilder<Type> &builder);    \
    template <>                                                                      \
    struct shine::reflection::detail::ReflectionRegister<Type> {                     \
        static constexpr bool Exists = true;                                         \
        template <typename Limits>                                                   \
        static constexpr void Call(shine::reflection::TypeBuilder<Type, Limits> &builder) { \
            Type##_Reflect(builder);                                                 \
        }                                                                            \
    };                                                                               \
    inline constexpr auto Type##_CT =                                                 \
        shine::reflection::BuildTypeInfoCT<Type>(#Type);                              \
    inline constinit auto Type##_Reg = []() {                                        \
        shine::reflection::TypeRegistry::Get().Register(Type##_CT);                  \
        return true;                                                                 \
    }();                                                                             \
    constexpr void Type##_Reflect(shine::reflection::TypeBuilder<Type> &builder)

