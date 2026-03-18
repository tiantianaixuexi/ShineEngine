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
#include <type_traits>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>

namespace shine::reflection {

struct TypeRegistrationPlan {
    size_t fieldCount = 0;
    size_t methodCount = 0;
    size_t enumCount = 0;
};

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

    TypeBuilder(const TypeBuilder&) = delete;
    TypeBuilder& operator=(const TypeBuilder&) = delete;
    TypeBuilder(TypeBuilder&&) = delete;
    TypeBuilder& operator=(TypeBuilder&&) = delete;

    explicit TypeBuilder(TypeInfo& ti, TypeRegistrationPlan plan = {})
        : info(ti)
        , fieldBatch_(ReflectionColdPool<FieldColdData>::Get().BeginContiguousBatch(plan.fieldCount))
        , methodBatch_(ReflectionColdPool<MethodColdData>::Get().BeginContiguousBatch(plan.methodCount)) {
        if (plan.fieldCount != 0) {
            if (info.fields.empty()) {
                info.fields.resize(plan.fieldCount);
                fieldsPreSized_ = true;
            } else {
                info.fields.reserve(info.fields.size() + plan.fieldCount);
                fieldWriteIndex_ = info.fields.size();
            }
        }
        if (plan.methodCount != 0) {
            if (info.methods.empty()) {
                info.methods.resize(plan.methodCount);
                methodsPreSized_ = true;
            } else {
                info.methods.reserve(info.methods.size() + plan.methodCount);
                methodWriteIndex_ = info.methods.size();
            }
        }
        if (plan.enumCount != 0) {
            auto& enumEntries = info.MutableEnumEntries();
            if (enumEntries.empty()) {
                enumEntries.resize(plan.enumCount);
                enumsPreSized_ = true;
            } else {
                enumEntries.reserve(enumEntries.size() + plan.enumCount);
                enumWriteIndex_ = enumEntries.size();
            }
        }
    }

    ~TypeBuilder() {
        if (fieldsPreSized_) {
            info.fields.resize(fieldWriteIndex_);
        }
        if (methodsPreSized_) {
            info.methods.resize(methodWriteIndex_);
        }
        if (enumsPreSized_) {
            info.MutableEnumEntries().resize(enumWriteIndex_);
        }
    }

    // ---- FieldBuilder (fluent API, C++23: unified Meta via template) --------

    struct FieldBuilder {
        FieldInfo&    field;
        TypeBuilder&  builder;
        FieldColdData* coldData = nullptr;

        FieldBuilder& Range(float lo, float hi) {
            if (coldData) {
                coldData->builtinMetadata.minValue = lo;
                coldData->builtinMetadata.maxValue = hi;
                coldData->builtinMetadata.hasRange = true;
            } else {
                field.SetRange(lo, hi);
            }
            return *this;
        }

        FieldBuilder& UI(UI::Schema s) {
            if (coldData) {
                coldData->uiSchema = std::move(s);
            } else {
                field.SetUISchema(std::move(s));
            }
            return *this;
        }
        FieldBuilder& EditAnywhere()         { field.flags |= PropertyFlags::EditAnywhere;    return *this; }
        FieldBuilder& ReadOnly()             { field.flags |= PropertyFlags::ReadOnly;        return *this; }
        FieldBuilder& ScriptRead()           { field.flags |= PropertyFlags::ScriptRead;      return *this; }
        FieldBuilder& ScriptWrite()          { field.flags |= PropertyFlags::ScriptWrite;     return *this; }
        FieldBuilder& ScriptReadWrite()      { field.flags |= PropertyFlags::ScriptReadWrite; return *this; }
        FieldBuilder& Transient()            { field.flags |= PropertyFlags::Transient;       return *this; }
        FieldBuilder& SaveGame()             { field.flags |= PropertyFlags::SaveGame;        return *this; }
        FieldBuilder& FunctionSelect() {
            if (coldData) {
                coldData->uiSchema = UI::FunctionSelector{};
            } else {
                field.SetUISchema(UI::FunctionSelector{});
            }
            return *this;
        }
        FieldBuilder& DisplayName(shine::STextView dn) {
            if (coldData) {
                coldData->builtinMetadata.displayName = dn;
            } else {
                field.SetDisplayName(dn);
            }
            return *this;
        }

        /// Single template replaces five per-type Meta overloads (C++23).
        template <typename V>
        FieldBuilder& Meta(shine::STextView key, V&& value) {
            const auto metadataKey = Hash(key);
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (!field.TrySetBuiltinMetadata(metadataKey, metadataValue)) {
                if (coldData) {
                    coldData->metadata.push_back({metadataKey, std::move(metadataValue)});
                } else {
                    field.MutableMetadata().push_back({metadataKey, std::move(metadataValue)});
                }
            }
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        FieldBuilder& Meta(MetadataKey key, V&& value) {
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (!field.TrySetBuiltinMetadata(key, metadataValue)) {
                if (coldData) {
                    coldData->metadata.push_back({key, std::move(metadataValue)});
                } else {
                    field.MutableMetadata().push_back({key, std::move(metadataValue)});
                }
            }
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

        auto& f = AcquireFieldSlot();
        f = FieldInfo{};
        auto fieldColdData = MakeReflectionColdData<FieldColdData>();
        fieldColdData->name = node.name;
        f.nameHash  = Hash(node.name);
        f.typeId    = GetTypeId<MType>();
        f.owner     = ReflectionOwnerHandle{};
        f.offset    = ComputeOffset<CType, MType>(MemberPtr);
        f.size      = sizeof(MType);
        f.alignment = alignof(MType);
        f.isPod     = std::is_trivially_copyable_v<MType>;
        f.coldData  = std::move(fieldColdData);

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

        return FieldBuilder{f, *this, f.coldData.get()};
    }

    // ---- MethodBuilder (fluent API, C++23: unified Meta) --------------------

    struct MethodBuilder {
        MethodInfo&  method;
        TypeBuilder& builder;
        MethodColdData* coldData = nullptr;

        MethodBuilder& ScriptCallable() { method.flags |= FunctionFlags::ScriptCallable; return *this; }
        MethodBuilder& EditorCallable() { method.flags |= FunctionFlags::EditorCallable; return *this; }

        template <typename V>
        MethodBuilder& Meta(shine::STextView key, V&& value) {
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (coldData) {
                coldData->metadata.push_back({Hash(key), std::move(metadataValue)});
            } else {
                method.MutableMetadata().push_back({Hash(key), std::move(metadataValue)});
            }
            return *this;
        }

        /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
        template <typename V>
        MethodBuilder& Meta(MetadataKey key, V&& value) {
            auto metadataValue = MakeMetadataValue(std::forward<V>(value));
            if (coldData) {
                coldData->metadata.push_back({key, std::move(metadataValue)});
            } else {
                method.MutableMetadata().push_back({key, std::move(metadataValue)});
            }
            return *this;
        }
    };

    template <auto MethodPtr>
    MethodBuilder RegisterMethodFromDSL(DSL::MethodDSLNode<MethodPtr> node) {
        using Traits = MethodTraits<decltype(MethodPtr)>;

        auto& m = AcquireMethodSlot();
        m = MethodInfo{};
        auto methodColdData = MakeReflectionColdData<MethodColdData>();
        methodColdData->name = node.name;
        m.nameHash   = Hash(node.name);
        m.returnType = GetTypeId<typename Traits::ReturnType>();
        m.owner      = ReflectionOwnerHandle{};   // patched after TypeInfo is complete
        m.paramTypes.reserve(Traits::Arity);
        m.coldData   = std::move(methodColdData);

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

        return MethodBuilder{m, *this, m.coldData.get()};
    }

    // ---- Enum registration --------------------------------------------------

    struct EnumPair { T value; shine::STextView name; };

    void Enums(std::initializer_list<EnumPair> entries) {
        info.isEnum = true;
        for (const auto& e : entries) {
            auto& entry = AcquireEnumSlot();
            entry = EnumEntry{static_cast<int64_t>(e.value), e.name};
        }
    }

private:
    FieldInfo& AcquireFieldSlot() {
        if (fieldWriteIndex_ < info.fields.size()) {
            return info.fields[fieldWriteIndex_++];
        }
        info.fields.emplace_back();
        ++fieldWriteIndex_;
        return info.fields.back();
    }

    MethodInfo& AcquireMethodSlot() {
        if (methodWriteIndex_ < info.methods.size()) {
            return info.methods[methodWriteIndex_++];
        }
        info.methods.emplace_back();
        ++methodWriteIndex_;
        return info.methods.back();
    }

    EnumEntry& AcquireEnumSlot() {
        auto& enumEntries = info.MutableEnumEntries();
        if (enumWriteIndex_ < enumEntries.size()) {
            return enumEntries[enumWriteIndex_++];
        }
        enumEntries.emplace_back();
        ++enumWriteIndex_;
        return enumEntries.back();
    }

    size_t fieldWriteIndex_ = 0;
    size_t methodWriteIndex_ = 0;
    size_t enumWriteIndex_ = 0;
    bool fieldsPreSized_ = false;
    bool methodsPreSized_ = false;
    bool enumsPreSized_ = false;
    typename ReflectionColdPool<FieldColdData>::ContiguousBatch fieldBatch_;
    typename ReflectionColdPool<MethodColdData>::ContiguousBatch methodBatch_;
};

template <typename T>
struct TypeBuilderPlanCounter {
    using ObjectType = T;

    TypeRegistrationPlan& plan;

    explicit TypeBuilderPlanCounter(TypeRegistrationPlan& registrationPlan)
        : plan(registrationPlan) {}

    struct FieldBuilder {
        FieldBuilder& Range(float, float) { return *this; }
        FieldBuilder& UI(UI::Schema) { return *this; }
        FieldBuilder& EditAnywhere() { return *this; }
        FieldBuilder& ReadOnly() { return *this; }
        FieldBuilder& ScriptRead() { return *this; }
        FieldBuilder& ScriptWrite() { return *this; }
        FieldBuilder& ScriptReadWrite() { return *this; }
        FieldBuilder& Transient() { return *this; }
        FieldBuilder& SaveGame() { return *this; }
        FieldBuilder& FunctionSelect() { return *this; }
        FieldBuilder& DisplayName(shine::STextView) { return *this; }

        template <typename V>
        FieldBuilder& Meta(shine::STextView, V&&) { return *this; }

        template <typename V>
        FieldBuilder& Meta(MetadataKey, V&&) { return *this; }

        template <auto Cb>
        FieldBuilder& OnChange() { return *this; }
    };

    struct MethodBuilder {
        MethodInfo method{};

        MethodBuilder& ScriptCallable() { return *this; }
        MethodBuilder& EditorCallable() { return *this; }

        template <typename V>
        MethodBuilder& Meta(shine::STextView, V&&) { return *this; }

        template <typename V>
        MethodBuilder& Meta(MetadataKey, V&&) { return *this; }
    };

    template <auto MemberPtr>
    FieldBuilder RegisterFieldFromDSL(const DSL::FieldDSLNode<MemberPtr>&) {
        ++plan.fieldCount;
        return {};
    }

    template <auto MethodPtr>
    MethodBuilder RegisterMethodFromDSL(const DSL::MethodDSLNode<MethodPtr>&) {
        ++plan.methodCount;
        return {};
    }

    struct EnumPair { T value; shine::STextView name; };

    void Enums(std::initializer_list<EnumPair> entries) {
        plan.enumCount += entries.size();
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
    static auto Register(T& builder, shine::STextView name) {
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
constexpr TypeInfo BuildTypeInfo(shine::STextView name) {
    TypeInfo i{};
    i.id        = GetTypeId<T>();
    i.SetName(name);
    i.size      = sizeof(T);
    i.alignment = alignof(T);
    i.isPod     = std::is_trivially_copyable_v<T>;
    i.isEnum    = std::is_enum_v<T>;
    return i;
}

// Legacy alias
template <typename T>
constexpr TypeInfo BuildTypeInfoCT(shine::STextView name) { return BuildTypeInfo<T>(name); }

} // namespace shine::reflection
