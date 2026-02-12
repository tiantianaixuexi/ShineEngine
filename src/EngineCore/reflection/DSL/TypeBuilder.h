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

namespace shine::reflection {

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
        f.typeId    = GetTypeId<MType>();
        f.offset    = ComputeOffset<CType, MType>(MemberPtr);
        f.size      = sizeof(MType);
        f.alignment = alignof(MType);
        f.isPod     = std::is_trivially_copyable_v<MType>;

        f.getterFn = [](const void* inst, void* out, std::size_t, std::size_t) {
            const auto& val = static_cast<const CType*>(inst)->*MemberPtr;
            if constexpr (std::is_trivially_copyable_v<MType>)
                std::memcpy(out, &val, sizeof(MType));
            else
                *static_cast<MType*>(out) = val;
        };

        f.setterFn = [](void* inst, const void* in, std::size_t, std::size_t) {
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
        m.returnType = GetTypeId<typename Traits::ReturnType>();
        m.owner      = nullptr;   // patched after TypeInfo is complete

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
                        *static_cast<std::tuple_element_t<I, typename Traits::ParamTuple>*>(
                            args ? args[I] : nullptr)...);
                } else {
                    auto result = (obj->*MethodPtr)(
                        *static_cast<std::tuple_element_t<I, typename Traits::ParamTuple>*>(
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