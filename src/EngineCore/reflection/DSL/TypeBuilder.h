#pragma once

// =============================================================================
// TypeBuilder.h — Runtime Type Builder for Reflection System
// =============================================================================
//
// Provides fluent API for building TypeInfo at runtime from DSL nodes.
// Used by REFLECTION_STRUCT and REFLECT_ENUM macros.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include "FieldDSL.h"
#include "MethodDSL.h"
#include <string_view>
#include <type_traits>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>

namespace shine::reflection {

// Traits detection
template<typename T> struct is_sequence_container : std::false_type {};
template<typename T> struct is_associative_container : std::false_type {};
template<typename T> struct container_trait_provider { static const void* get() { return nullptr; } };

template<typename T, typename A> struct is_sequence_container<std::vector<T, A>> : std::true_type {};
template<typename T, typename A> struct container_trait_provider<std::vector<T, A>> { static const void* get() { return &ListThunks<std::vector<T, A>>::GetTrait(); } };

template<typename K, typename V, typename C, typename A> struct is_associative_container<std::map<K, V, C, A>> : std::true_type {};
template<typename K, typename V, typename C, typename A> struct container_trait_provider<std::map<K, V, C, A>> { static const void* get() { return &MapThunks<std::map<K, V, C, A>>::GetTrait(); } };

template<typename K, typename V, typename H, typename E, typename A> struct is_associative_container<std::unordered_map<K, V, H, E, A>> : std::true_type {};
template<typename K, typename V, typename H, typename E, typename A> struct container_trait_provider<std::unordered_map<K, V, H, E, A>> { static const void* get() { return &MapThunks<std::unordered_map<K, V, H, E, A>>::GetTrait(); } };

template<typename T, typename C, typename A> struct is_associative_container<std::set<T, C, A>> : std::true_type {};
template<typename T, typename C, typename A> struct container_trait_provider<std::set<T, C, A>> { static const void* get() { return &SetThunks<std::set<T, C, A>>::GetTrait(); } };

template<typename T, typename H, typename E, typename A> struct is_associative_container<std::unordered_set<T, H, E, A>> : std::true_type {};
template<typename T, typename H, typename E, typename A> struct container_trait_provider<std::unordered_set<T, H, E, A>> { static const void* get() { return &SetThunks<std::unordered_set<T, H, E, A>>::GetTrait(); } };

// For std::array we need ArrayThunks
template <typename ArrayType>
struct ArrayThunks {
    static std::size_t GetSize(const void* p) { return std::tuple_size_v<ArrayType>; }
    static void* GetElement(void* p, std::size_t i) { return &(*static_cast<ArrayType*>(p))[i]; }
    static const void* GetElementConst(const void* p, std::size_t i) { return &(*static_cast<const ArrayType*>(p))[i]; }
    static void Resize(void* p, std::size_t) { /* no-op */ }
    static const SequenceTrait& GetTrait() {
        static const SequenceTrait trait{
            GetTypeId<typename ArrayType::value_type>(),
            &GetSize, &GetElement, &GetElementConst, &Resize
        };
        return trait;
    }
};

template<typename T, std::size_t N> struct is_sequence_container<std::array<T, N>> : std::true_type {};
template<typename T, std::size_t N> struct container_trait_provider<std::array<T, N>> { static const void* get() { return &ArrayThunks<std::array<T, N>>::GetTrait(); } };

// =============================================================================
// TypeBuilder — runtime DSL builder
// =============================================================================

template <typename T, typename Limits = void>
struct TypeBuilder {
    using ObjectType = T;

    TypeInfo& info;
    explicit TypeBuilder(TypeInfo& ti) : info(ti) {}

    // ---- FieldBuilder (fluent API, C++23: unified Meta via template) --------

    struct FieldBuilder {
        FieldInfo&    field;
        TypeBuilder&  builder;

        FieldBuilder& Range(float lo, float hi) {
            field.metadata.push_back({MetaKeys::Min, lo});
            field.metadata.push_back({MetaKeys::Max, hi});
            return *this;
        }

        FieldBuilder& UI(UI::Schema s)      { field.uiSchema = std::move(s); return *this; }
        FieldBuilder& EditAnywhere()         { field.flags |= PropertyFlags::EditAnywhere;    return *this; }
        FieldBuilder& ReadOnly()             { field.flags |= PropertyFlags::ReadOnly;        return *this; }
        FieldBuilder& ScriptRead()           { field.flags |= PropertyFlags::ScriptRead;      return *this; }
        FieldBuilder& ScriptWrite()          { field.flags |= PropertyFlags::ScriptWrite;     return *this; }
        FieldBuilder& ScriptReadWrite()      { field.flags |= PropertyFlags::ScriptReadWrite; return *this; }
        FieldBuilder& Transient()            { field.flags |= PropertyFlags::Transient;       return *this; }
        FieldBuilder& SaveGame()             { field.flags |= PropertyFlags::SaveGame;        return *this; }
        FieldBuilder& FunctionSelect()       { field.uiSchema = UI::FunctionSelector{};       return *this; }
        FieldBuilder& DisplayName(std::string_view dn) {
            field.metadata.push_back({MetaKeys::DisplayName, MetadataValue{dn}});
            return *this;
        }

        /// Single template replaces five per-type Meta overloads (C++23).
        template <typename V>
        FieldBuilder& Meta(std::string_view key, V&& value) {
            field.metadata.push_back({Hash(key), MetadataValue{std::forward<V>(value)}});
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        FieldBuilder& Meta(MetadataKey key, V&& value) {
            field.metadata.push_back({key, MetadataValue{std::forward<V>(value)}});
            return *this;
        }

        template <auto Cb>
        FieldBuilder& OnChange() {
            using Param = typename FnParamExtractor<decltype(Cb)>::type;
            field.onChangeFn = [](void* inst, const void* old) {
                auto* obj = static_cast<T*>(inst);
                if constexpr (std::is_same_v<Param, void>) {
                    (obj->*Cb)();
                } else {
                    if (old) (obj->*Cb)(*static_cast<const Param*>(old));
                }
            };
            return *this;
        }
    };

    template <auto MemberPtr>
    FieldBuilder RegisterFieldFromDSL(DSL::FieldDSLNode<MemberPtr> node) {
        using MType = typename DSL::FieldDSLNode<MemberPtr>::MemberType;
        using CType = typename DSL::FieldDSLNode<MemberPtr>::ClassType;

        FieldInfo f{};
        f.name      = node.name;
        f.nameHash  = Hash(node.name);
        f.typeId    = GetTypeId<MType>();
        f.offset    = ComputeOffset<CType, MType>(MemberPtr);
        f.size      = sizeof(MType);
        f.alignment = alignof(MType);
        f.isPod     = std::is_trivially_copyable_v<MType>;

        if constexpr (is_sequence_container<MType>::value) {
            f.containerType = ContainerType::Sequence;
            f.containerTrait = container_trait_provider<MType>::get();
        } else if constexpr (is_associative_container<MType>::value) {
            f.containerType = ContainerType::Associative;
            f.containerTrait = container_trait_provider<MType>::get();
        }

        // Optimized getter/setter - 零间接调用
        f.getterFn = [](const void* inst, void* out) {
            const auto& val = static_cast<const CType*>(inst)->*MemberPtr;
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(out, &val, sizeof(MType));
            else
                *static_cast<MType*>(out) = val;
        };

        f.setterFn = [](void* inst, const void* in) {
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(&(static_cast<CType*>(inst)->*MemberPtr), in, sizeof(MType));
            else
                static_cast<CType*>(inst)->*MemberPtr = *static_cast<const MType*>(in);
        };

        f.equalsFn = [](const void* a, const void* b, std::size_t) -> bool {
            if constexpr (requires(const MType& x, const MType& y) { x == y; })
                return *static_cast<const MType*>(a) == *static_cast<const MType*>(b);
            else
                return std::memcmp(a, b, sizeof(MType)) == 0;
        };

        f.copyFn = [](void* dst, const void* src, std::size_t) {
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(dst, src, sizeof(MType));
            else
                *static_cast<MType*>(dst) = *static_cast<const MType*>(src);
        };

        info.fields.push_back(std::move(f));
        return FieldBuilder{info.fields.back(), *this};
    }

    // ---- MethodBuilder (fluent API, C++23: unified Meta) --------------------

    struct MethodBuilder {
        MethodInfo&  method;
        TypeBuilder& builder;

        MethodBuilder& ScriptCallable() { method.flags |= FunctionFlags::ScriptCallable; return *this; }
        MethodBuilder& EditorCallable() { method.flags |= FunctionFlags::EditorCallable; return *this; }

        template <typename V>
        MethodBuilder& Meta(std::string_view key, V&& value) {
            method.metadata.push_back({Hash(key), MetadataValue{std::forward<V>(value)}});
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        MethodBuilder& Meta(MetadataKey key, V&& value) {
            method.metadata.push_back({key, MetadataValue{std::forward<V>(value)}});
            return *this;
        }
    };

    template <auto MethodPtr>
    MethodBuilder RegisterMethodFromDSL(DSL::MethodDSLNode<MethodPtr> node) {
        using Traits = MethodTraits<decltype(MethodPtr)>;

        MethodInfo m{};
        m.name       = node.name;
        m.nameHash   = Hash(node.name);
        m.returnType = GetTypeId<typename Traits::ReturnType>();
        m.owner      = nullptr;   // patched after TypeInfo is complete
        m.paramTypes.reserve(Traits::Arity);

        // Record parameter types
        [&]<std::size_t... I>(std::index_sequence<I...>) {
            ((m.paramTypes.push_back(
                GetTypeId<std::tuple_element_t<I, typename Traits::ParamTuple>>())), ...);
        }(std::make_index_sequence<Traits::Arity>{});

        // Generate type-safe invoke thunk that unpacks void** args
        m.invokeFn = [](void* inst, void** args, void* ret) {
            auto* obj = static_cast<T*>(inst);

            // Unpack args into correctly-typed values and call
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                if constexpr (std::is_void_v<typename Traits::ReturnType>) {
                    (obj->*MethodPtr)(
                        *static_cast<std::remove_reference_t<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                            args ? args[I] : nullptr)...);
                } else {
                    auto result = (obj->*MethodPtr)(
                        *static_cast<std::remove_reference_t<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                            args ? args[I] : nullptr)...);
                    if (ret) *static_cast<typename Traits::ReturnType*>(ret) = result;
                }
            }(std::make_index_sequence<Traits::Arity>{});
        };

        info.methods.push_back(std::move(m));
        return MethodBuilder{info.methods.back(), *this};
    }

    // ---- Enum registration --------------------------------------------------

    struct EnumPair { T value; std::string_view name; };

    void Enums(std::initializer_list<EnumPair> entries) {
        info.isEnum = true;
        for (const auto& e : entries)
            info.enumEntries.push_back({static_cast<int64_t>(e.value), e.name});
    }
};

// =============================================================================
// FastMethodCall — Optimized method invocation with C++ index sequences
// =============================================================================

namespace detail {

// 辅助模板：移除引用
template<typename T>
using RemoveRef = std::remove_reference_t<T>;

// 便捷函数模板 - 使用模板参数传递方法指针，支持任意参数
template<auto MethodPtr>
[[gnu::always_inline]] inline void FastMethodCall(void* inst, void** args, void* ret) {
    using MP = decltype(MethodPtr);
    using Traits = MethodTraits<MP>;
    using ClassType = typename Traits::ClassType;
    using ReturnType = typename Traits::ReturnType;
    
    auto* obj = static_cast<ClassType*>(inst);
    
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        if constexpr (std::is_void_v<ReturnType>) {
            (obj->*MethodPtr)(
                *static_cast<RemoveRef<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                    args ? args[I] : nullptr)...);
        } else {
            auto result = (obj->*MethodPtr)(
                *static_cast<RemoveRef<std::tuple_element_t<I, typename Traits::ParamTuple>>*>(
                    args ? args[I] : nullptr)...);
            if (ret) *static_cast<ReturnType*>(ret) = result;
        }
    }(std::make_index_sequence<Traits::Arity>{});
}

} // namespace detail

// =============================================================================
// 注册方法时使用优化的调用器
// =============================================================================

// 使用示例:
// builder.RegisterMethodFast< &MyClass::MyMethod >("MyMethod")

template <auto MethodPtr>
struct FastMethodRegistration {
    template<typename T>
    static auto Register(T& builder, std::string_view name) {
        //using MP = decltype(MethodPtr);
        
        // 使用 DSL 创建方法节点
        auto mb = builder.RegisterMethodFromDSL(
            DSL::MakeMethodDSL<MethodPtr>(name));
        
        // 替换为优化的调用器
        mb.method.invokeFn = &detail::FastMethodCall<MethodPtr>;
        
        return mb;  // 返回 MethodBuilder 支持链式调用
    }
};

// =============================================================================
// BuildTypeInfo — constructs a basic TypeInfo at runtime init
// =============================================================================

template <typename T>
constexpr TypeInfo BuildTypeInfo(std::string_view name) {
    TypeInfo i{};
    i.id        = GetTypeId<T>();
    i.name      = name;
    i.size      = sizeof(T);
    i.alignment = alignof(T);
    i.isPod     = std::is_trivially_copyable_v<T>;
    i.isEnum    = std::is_enum_v<T>;
    return i;
}

// Legacy alias
template <typename T>
constexpr TypeInfo BuildTypeInfoCT(std::string_view name) { return BuildTypeInfo<T>(name); }

} // namespace shine::reflection