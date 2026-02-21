#pragma once

// =============================================================================
// Reflection.h — Shine Runtime Reflection System
// =============================================================================
//
// Public API: TypeRegistry · Views · DSL · TypeBuilder · Macros
// Include this single header to use the reflection system.
//
// C++23 / MSVC
// =============================================================================

#include "ReflectionCore.h"
#include "ReflectionError.h"

#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <functional>

// =============================================================================
// Runtime Type Registry
// =============================================================================

namespace shine::reflection {

class TypeRegistry {
public:
    static TypeRegistry& Get() {
        static TypeRegistry instance;
        return instance;
    }

    void Register(TypeInfo info) {
        auto id = info.id;
        registry_[id] = std::make_shared<TypeInfo>(std::move(info));
    }

    const TypeInfo* Find(TypeId id) const {
        auto it = registry_.find(id);
        return (it != registry_.end()) ? it->second.get() : nullptr;
    }

    template <typename T>
    const TypeInfo* Find() const { return Find(GetTypeId<T>()); }

    std::size_t GetRegisteredTypeCount() const { return registry_.size(); }

private:
    TypeRegistry() = default;
    std::unordered_map<TypeId, std::shared_ptr<TypeInfo>> registry_;
};

// =============================================================================
// Script types (stubs for scripting integration)
// =============================================================================

struct ScriptValue {
    std::variant<std::monostate, bool, int, float, double, std::string, void*> data;

    ScriptValue() = default;

    template <typename T>
    explicit ScriptValue(T&& val) : data(std::forward<T>(val)) {}

    bool IsEmpty() const { return std::holds_alternative<std::monostate>(data); }
};

struct ScriptBridge {
    virtual ~ScriptBridge() = default;
    virtual ScriptValue ToScript(const void*, TypeId)                    const { return {}; }
    virtual void        FromScript(const ScriptValue&, void*, TypeId)    const {}
};

// =============================================================================
// Helper: type construction / destruction (placeholders)
// =============================================================================

inline void Construct(void*, TypeId) {}
inline void Destruct(void*, TypeId)  {}

// =============================================================================
// Views (Inspector / Script / ECS)
// =============================================================================

struct TypeView {
    const TypeInfo* typeInfo = nullptr;
    bool IsValid() const { return typeInfo != nullptr; }
};

// ---- InspectorView ----------------------------------------------------------

struct InspectorView : TypeView {
    struct FieldIterator {
        const TypeInfo* type;
        std::size_t     index;
        bool operator!=(const FieldIterator& o) const { return index != o.index; }
        void operator++() { ++index; }
        const FieldInfo& operator*() const { return type->fields[index]; }
    };

    FieldIterator begin() const { return {typeInfo, 0}; }
    FieldIterator end()   const { return {typeInfo, typeInfo->fields.size()}; }

    bool IsEditable(const FieldInfo& f) const {
        return HasFlag(f.flags, PropertyFlags::EditAnywhere)
            && !HasFlag(f.flags, PropertyFlags::ReadOnly);
    }

    const UI::Schema& GetUISchema(const FieldInfo& f) const { return f.uiSchema; }

    bool IsVisible(const FieldInfo& f, const void* instance) const {
        if (auto* m = f.GetMeta(MetaKeys::EditCondition);
            m && std::holds_alternative<std::string_view>(*m))
        {
            auto condField = std::get<std::string_view>(*m);
            if (auto* ci = typeInfo->FindField(condField);
                ci && ci->typeId == GetTypeId<bool>())
            {
                bool bVal = false;
                ci->Get(instance, &bVal);
                if (!bVal) return false;
            }
        }
        return true;
    }

    std::string_view GetCategory(const FieldInfo& f) const {
        if (auto* m = f.GetMeta(MetaKeys::Category);
            m && std::holds_alternative<std::string_view>(*m))
            return std::get<std::string_view>(*m);
        return "";
    }

    void SetValue(void* instance, const FieldInfo& f, const void* value) const {
        if (IsEditable(f)) f.Set(instance, value);
    }
};

// ---- ScriptView -------------------------------------------------------------

namespace detail {

/// Stack-or-heap scratch buffer for type-erased temporaries.
struct ScratchBuffer {
    static constexpr std::size_t kStack = 64;
    alignas(16) char stackBuf[kStack]{};
    std::unique_ptr<char[]> heap;
    void* ptr;

    explicit ScratchBuffer(std::size_t sz, std::size_t align = 16)
        : ptr(nullptr)
    {
        if (sz <= kStack && align <= 16) { ptr = stackBuf; }
        else { heap = std::make_unique<char[]>(sz); ptr = heap.get(); }
    }
};

} // namespace detail

struct ScriptView : TypeView {
    static const TypeInfo* GetTypeInfo(TypeId id) {
        return TypeRegistry::Get().Find(id);
    }

    const FieldInfo*  GetFieldInfo(std::string_view n) const { return typeInfo->FindField(n); }
    const FieldInfo*  GetFieldInfo(std::size_t i)      const {
        return i < typeInfo->fields.size() ? &typeInfo->fields[i] : nullptr;
    }
    const MethodInfo* GetMethodInfo(std::string_view n) const { return typeInfo->FindMethod(n); }
    const MethodInfo* GetMethodInfo(std::size_t i)      const {
        return i < typeInfo->methods.size() ? &typeInfo->methods[i] : nullptr;
    }

    // --- Field get / set (with stack-buffer optimisation) --------------------

    ScriptValue GetField(void* instance, const FieldInfo* field, const ScriptBridge& bridge) const {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptRead)) return {};

        detail::ScratchBuffer buf(field->size, field->alignment);
        if (!field->isPod) Construct(buf.ptr, field->typeId);
        field->Get(instance, buf.ptr);
        auto result = bridge.ToScript(buf.ptr, field->typeId);
        if (!field->isPod) Destruct(buf.ptr, field->typeId);
        return result;
    }

    void SetField(void* instance, const FieldInfo* field,
                  const ScriptValue& value, const ScriptBridge& bridge) const
    {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptWrite)) return;

        detail::ScratchBuffer buf(field->size, field->alignment);
        if (!field->isPod) Construct(buf.ptr, field->typeId);
        bridge.FromScript(value, buf.ptr, field->typeId);
        field->Set(instance, buf.ptr);
        if (!field->isPod) Destruct(buf.ptr, field->typeId);
    }

    ScriptValue GetField(void* inst, std::string_view n, const ScriptBridge& br) const {
        return GetField(inst, GetFieldInfo(n), br);
    }
    void SetField(void* inst, std::string_view n, const ScriptValue& v, const ScriptBridge& br) const {
        SetField(inst, GetFieldInfo(n), v, br);
    }

    // --- Method invocation ---------------------------------------------------

    ScriptValue CallMethod(void* instance, const MethodInfo* method,
                           const std::vector<ScriptValue>& args,
                           const ScriptBridge& bridge) const
    {
        if (!method || !HasFlag(method->flags, FunctionFlags::ScriptCallable))
            return {};
        if (args.size() != method->paramTypes.size())
            return {};

        std::vector<std::unique_ptr<char[]>> argBufs;
        std::vector<void*> rawArgs(args.size());
        argBufs.reserve(args.size());

        for (std::size_t i = 0; i < args.size(); ++i) {
            const TypeInfo* pt = GetTypeInfo(method->paramTypes[i]);
            if (!pt) return {};
            auto buf = std::make_unique<char[]>(pt->size);
            bridge.FromScript(args[i], buf.get(), method->paramTypes[i]);
            rawArgs[i] = buf.get();
            argBufs.push_back(std::move(buf));
        }

        const TypeInfo* rt = (method->returnType != GetTypeId<void>())
                                 ? GetTypeInfo(method->returnType) : nullptr;
        std::unique_ptr<char[]> retBuf;
        void* retPtr = nullptr;
        if (rt) { retBuf = std::make_unique<char[]>(rt->size); retPtr = retBuf.get(); }

        method->Invoke(instance, rawArgs.data(), retPtr);

        return (rt && retPtr) ? bridge.ToScript(retPtr, method->returnType) : ScriptValue{};
    }

    ScriptValue CallMethod(void* inst, std::string_view n,
                           const std::vector<ScriptValue>& a, const ScriptBridge& br) const {
        return CallMethod(inst, GetMethodInfo(n), a, br);
    }
};

// ---- ECSView ----------------------------------------------------------------

struct ECSView {
    struct ComponentLayout {
        std::size_t     size;
        std::size_t     alignment;
        const TypeInfo* layoutSource;
    };
    ComponentLayout layout;
    std::size_t GetSize()      const { return layout.size; }
    std::size_t GetAlignment() const { return layout.alignment; }
};

} // namespace shine::reflection

// =============================================================================
// DSL (Domain-Specific Language for field / method registration)
// =============================================================================

namespace shine::reflection {

// Helper: extract first parameter type from member-function pointer
template <typename T> struct FnParamExtractor;
template <typename C, typename R, typename P>
struct FnParamExtractor<R(C::*)(P)> { using type = P; };
template <typename C, typename R>
struct FnParamExtractor<R(C::*)()>  { using type = void; };

// Helper: extract class + member type from member-pointer
template <typename T> struct MemberPtrInfo;
template <typename C, typename M>
struct MemberPtrInfo<M C::*> { using ClassType = C; using MemberType = M; };

// Helper: decompose a member-function pointer into return type, class, and parameter pack
template <typename T> struct MethodTraits;

template <typename C, typename R, typename... Args>
struct MethodTraits<R(C::*)(Args...)> {
    using ClassType  = C;
    using ReturnType = R;
    using ParamTuple = std::tuple<Args...>;
    static constexpr std::size_t Arity = sizeof...(Args);
};

template <typename C, typename R, typename... Args>
struct MethodTraits<R(C::*)(Args...) const> {
    using ClassType  = C;
    using ReturnType = R;
    using ParamTuple = std::tuple<Args...>;
    static constexpr std::size_t Arity = sizeof...(Args);
};

// Compute member offset from member pointer (MSVC-compatible)
template <typename C, typename M>
inline std::size_t ComputeOffset(M C::* ptr) {
    alignas(alignof(C)) char buf[sizeof(C)]{};
    auto* fake = reinterpret_cast<C*>(buf);
    auto* addr = &(fake->*ptr);
    return static_cast<std::size_t>(
        reinterpret_cast<const char*>(addr) - reinterpret_cast<const char*>(fake));
}

namespace DSL {

// ---- FieldDescriptor --------------------------------------------------------

struct FieldDescriptorBase {
    std::string_view  name;
    UI::Schema        uiSchema = UI::None{};
    MetadataContainer metadata;
    OnChangeFn        onChange  = nullptr;
};

template <std::size_t = 0>
struct FieldDescriptor : FieldDescriptorBase {};

// ---- FieldDSLNode (created by REFLECT_FIELD macro) --------------------------

template <auto MemberPtr>
struct FieldDSLNode {
    using Info       = MemberPtrInfo<decltype(MemberPtr)>;
    using ClassType  = typename Info::ClassType;
    using MemberType = typename Info::MemberType;

    static constexpr auto MemberPtrValue = MemberPtr;

    std::string_view   name;
    FieldDescriptor<0> desc;

    explicit FieldDSLNode(std::string_view n) : name(n) { desc.name = n; }

    // Chaining (returns copy — enables StaticInspector DSL compatibility)
    auto EditAnywhere()    const { return *this; }
    auto ReadOnly()        const { return *this; }
    auto ScriptReadWrite() const { return *this; }

    template <typename S>
    auto UI(S&& schema) const {
        auto c = *this;
        c.desc.uiSchema = std::forward<S>(schema);
        return c;
    }

    template <typename V>
    auto Meta(std::string_view key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({Hash(key), MetadataValue{std::forward<V>(val)}});
        return c;
    }

    /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
    template <typename V>
    auto Meta(MetadataKey key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({key, MetadataValue{std::forward<V>(val)}});
        return c;
    }

    template <typename V>
    auto Range(V lo, V hi) const {
        auto c = *this;
        c.desc.metadata.push_back({MetaKeys::Min, MetadataValue{static_cast<float>(lo)}});
        c.desc.metadata.push_back({MetaKeys::Max, MetadataValue{static_cast<float>(hi)}});
        return c;
    }

    auto FunctionSelect(bool onlyScript = true) const {
        auto c = *this;
        c.desc.uiSchema = shine::reflection::UI::FunctionSelector{onlyScript};
        return c;
    }

    auto DisplayName(std::string_view dn) const {
        auto c = *this;
        c.desc.metadata.push_back({MetaKeys::DisplayName, MetadataValue{dn}});
        return c;
    }

    template <auto Cb>
    auto OnChange() const {
        using Param = typename FnParamExtractor<decltype(Cb)>::type;
        auto c = *this;
        c.desc.onChange = [](void* inst, const void* old) {
            auto* obj = static_cast<ClassType*>(inst);
            if constexpr (std::is_same_v<Param, void>) {
                (obj->*Cb)();
            } else {
                if (old) (obj->*Cb)(*static_cast<const Param*>(old));
            }
        };
        return c;
    }

};

// ---- MethodDSLNode (created by REFLECT_METHOD macro) ------------------------

struct MethodDescriptorBase {
    std::string_view  name;
    MetadataContainer metadata;
};

template <std::size_t = 0>
struct MethodDescriptor : MethodDescriptorBase {};

template <auto MethodPtr>
struct MethodDSLNode {
    static constexpr auto MethodPtrValue = MethodPtr;

    std::string_view    name;
    MethodDescriptor<0> desc;

    explicit MethodDSLNode(std::string_view n) : name(n) { desc.name = n; }

    auto ScriptCallable() const { return *this; }
    auto EditorCallable() const { return *this; }

    template <typename V>
    auto Meta(std::string_view key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({Hash(key), MetadataValue{std::forward<V>(val)}});
        return c;
    }

    /// Overload accepting a pre-computed MetadataKey (consteval-friendly).
    template <typename V>
    auto Meta(MetadataKey key, V&& val) const {
        auto c = *this;
        c.desc.metadata.push_back({key, MetadataValue{std::forward<V>(val)}});
        return c;
    }
};

template <auto M>
MethodDSLNode<M> MakeMethodDSL(std::string_view n) { return MethodDSLNode<M>{n}; }

} // namespace DSL

// =============================================================================
// TypeBuilder — runtime DSL builder (used by REFLECTION_STRUCT / REFLECT_ENUM)
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

// =============================================================================
// Macros (runtime registration — namespace-safe, no template specialization)
// =============================================================================

#define REFLECTION_STRUCT(Type)                                                 \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder);                             \
    template<typename _RB>                                                      \
    inline void _ReflectRegFn_##Type(_RB& builder)

#define REFLECTION_REGISTER(Type)                                               \
    template<typename _RB>                                                      \
    inline void _ReflectStaticBuild(_RB& builder, Type*) {                      \
        _ReflectRegFn_##Type(builder);                                          \
    }                                                                           \
    inline auto _ReflectInit_##Type = []() {                                    \
        shine::reflection::TypeInfo _info{};                                    \
        _info.id        = shine::reflection::GetTypeId<Type>();                 \
        _info.name      = #Type;                                                \
        _info.size      = sizeof(Type);                                         \
        _info.alignment = alignof(Type);                                        \
        _info.isPod     = std::is_trivially_copyable_v<Type>;                   \
        _info.isEnum    = std::is_enum_v<Type>;                                 \
        shine::reflection::TypeBuilder<Type> _builder(_info);                   \
        _ReflectRegFn_##Type(_builder);                                         \
        for (auto& _m : _info.methods) _m.owner = &_info;                      \
        shine::reflection::TypeRegistry::Get().Register(std::move(_info));      \
        return true;                                                            \
    }();

#define REFLECTION_REGISTER_LIMITS(Type, LimitsType) REFLECTION_REGISTER(Type)

#define REFLECT_FIELD(Member)                                                   \
    builder.RegisterFieldFromDSL(                                               \
        shine::reflection::DSL::FieldDSLNode<                                   \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member      \
        >(#Member))

#define REFLECT_METHOD(Member)                                                  \
    builder.RegisterMethodFromDSL(                                              \
        shine::reflection::DSL::MakeMethodDSL<                                  \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member      \
        >(#Member))

#define REFLECT_ENUM(Type)                                                      \
    inline void _ReflectRegFn_##Type(                                           \
        shine::reflection::TypeBuilder<Type>& builder);                         \
    inline auto _ReflectInit_##Type = []() {                                    \
        shine::reflection::TypeInfo _info{};                                    \
        _info.id        = shine::reflection::GetTypeId<Type>();                 \
        _info.name      = #Type;                                                \
        _info.size      = sizeof(Type);                                         \
        _info.alignment = alignof(Type);                                        \
        _info.isPod     = false;                                                \
        _info.isEnum    = true;                                                 \
        shine::reflection::TypeBuilder<Type> _builder(_info);                   \
        _ReflectRegFn_##Type(_builder);                                         \
        shine::reflection::TypeRegistry::Get().Register(std::move(_info));      \
        return true;                                                            \
    }();                                                                        \
    inline void _ReflectRegFn_##Type(                                           \
        shine::reflection::TypeBuilder<Type>& builder)
