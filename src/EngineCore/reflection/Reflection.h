#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <new>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "../../string/shine_string.h"

#include "Field/FieldInfo.h"
#include "ReflectionConcept.h"
#include "ReflectionFlags.h"
#include "ReflectionHash.h"
#include "ReflectionScript.h"
#include "ReflectionUI.h"

#ifdef SHINE_USE_MODULE
import shine.memory;
#else
#include "../../memory/memory.ixx"
#endif

template<typename T>
concept IsNumberAndFloat = std::is_floating_point_v<T> || std::is_integral_v<T>;

// -----------------------------------------------------------------------------
// 3. Metadata & Info Structures
// -----------------------------------------------------------------------------
namespace shine::reflection {

struct ArrayTrait {
    TypeId elementTypeId;
    size_t (*getSize)(const void *containerPtr);
    void *(*getElement)(void *containerPtr, size_t index);
    const void *(*getElementConst)(const void *containerPtr, size_t index);
    void (*resize)(void *containerPtr, size_t newSize);
};

struct MapTrait {
    TypeId keyType;
    TypeId valueType;
    size_t (*getSize)(const void *mapPtr);
    void (*clear)(void *mapPtr);
    void (*insert)(void *mapPtr, const void *key, const void *value);

    // Iterator API (Type Erased)
    // Iterator is opaque void*, allocated by begin
    void *(*begin)(void *mapPtr);
    bool (*valid)(const void *iter, const void *mapPtr); // check if iter != end
    void (*next)(void *iter);
    const void *(*key)(const void *iter);
    void *(*value)(const void *iter);
    void (*destroyIterator)(void *iter);
};

struct EnumEntry {
    int64_t          value;
    std::string_view name;
};

struct TypeInfo {
    std::string_view name;
    TypeId           id;
    size_t           size;
    size_t           alignment;

    std::vector<FieldInfo>  fields;
    std::vector<MethodInfo> methods;

    const TypeInfo *baseType   = nullptr;
    TypeId          baseTypeId = 0;

    bool                   isEnum = false;
    std::vector<EnumEntry> enumEntries;

    void *(*create)();
    void (*destroy)(void *);
    void (*construct)(void *);
    void (*destruct)(void *);
    void (*copy)(void *dst, const void *src); // Assignment

    bool isTrivial;
    bool isManaged = false; // If true, use ObjectHandle in scripts

    const FieldInfo *FindField(std::string_view fieldName) const {
        if (const auto it = std::ranges::find_if(fields, [fieldName](const FieldInfo &f) { return f.name == fieldName; }); it != fields.end())
            return &(*it);
        return baseType ? baseType->FindField(fieldName) : nullptr;
    }
    const MethodInfo *FindMethod(std::string_view methodName) const {
        if (const auto it = std::ranges::find_if(methods, [methodName](const MethodInfo &m) { return m.name == methodName; }); it != methods.end())
            return &(*it);
        return baseType ? baseType->FindMethod(methodName) : nullptr;
    }
};
} // namespace shine::reflection

// -----------------------------------------------------------------------------
// 4. Registry
// -----------------------------------------------------------------------------
namespace shine::reflection {
class TypeRegistry {
    public:
    static TypeRegistry &Get() {
        static TypeRegistry instance;
        return instance;
    }
    void Register(TypeInfo info) {
        auto it = std::ranges::lower_bound(
            types,
            info.id,
            {},
            &TypeInfo::id);
        if (it == types.end() || it->id != info.id)
            types.insert(it, std::move(info));
    }
    const TypeInfo *Find(TypeId id) const {

        auto it = std::ranges::lower_bound(
            types,
            id,
            {},
            &TypeInfo::id);

        if (it != types.end() && it->id == id)
            return &(*it);
        return nullptr;
    }
    template <typename T>
    const TypeInfo       *Find() const { return Find(GetTypeId<T>()); }
    static void           RegisterAllTypes();
    std::vector<TypeInfo> types;
};
inline void Construct(void *ptr, TypeId typeId) {
    if (typeId == GetTypeId<std::string>()) {
        new (ptr) std::string();
        return;
    }
    if (typeId == GetTypeId<shine::SString>()) {
        new (ptr) shine::SString();
        return;
    }
    const TypeInfo *info = TypeRegistry::Get().Find(typeId);
    if (info && info->construct)
        info->construct(ptr);
}

inline void Destruct(void *ptr, TypeId typeId) {
    if (typeId == GetTypeId<std::string>()) {
        static_cast<std::string *>(ptr)->~basic_string();
        return;
    }
    if (typeId == GetTypeId<shine::SString>()) {
        static_cast<shine::SString *>(ptr)->~SString();
        return;
    }
    const TypeInfo *info = TypeRegistry::Get().Find(typeId);
    if (info && info->destruct)
        info->destruct(ptr);
}

inline std::vector<TypeInfo> &GetPendingTypes() {
    static std::vector<TypeInfo> pending;
    return pending;
}

inline void TypeRegistry::RegisterAllTypes() {
    auto &registry = Get();
    auto &pending  = GetPendingTypes();
    for (auto &t : pending)
        registry.Register(std::move(t));
    pending.clear();
    for (auto &t : registry.types) {
        if (t.baseTypeId != 0)
            t.baseType = registry.Find(t.baseTypeId);
    }
}
} // namespace shine::reflection

// -----------------------------------------------------------------------------
// 5. Views (Inspector / Script / ECS)
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
        return HasFlag(field.flags, PropertyFlags::EditAnywhere) && !HasFlag(field.flags, PropertyFlags::ReadOnly);
    }
    const UI::Schema &GetUISchema(const FieldInfo &field) const { return field.uiSchema; }

    bool IsVisible(const FieldInfo &field, const void *instance) const {
        // Check EditCondition
        const MetadataValue *condMeta = field.GetMeta(Hash("EditCondition"));
        if (condMeta && std::holds_alternative<std::string_view>(*condMeta)) {
            std::string_view condField = std::get<std::string_view>(*condMeta);

            const FieldInfo *condInfo = typeInfo->FindField(condField);
            if (condInfo && condInfo->typeId == GetTypeId<bool>()) {
                bool bVal = false;
                condInfo->Get(instance, &bVal);
                if (!bVal)
                    return false; // Hidden
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

    // --- Field Access (String & Index) ---
    const FieldInfo *GetFieldInfo(std::string_view name) const { return typeInfo->FindField(name); }
    const FieldInfo *GetFieldInfo(size_t index) const {
        if (index < typeInfo->fields.size())
            return &typeInfo->fields[index];
        return nullptr;
    }

    ScriptValue GetField(void *instance, const FieldInfo *field, const ScriptBridge &bridge) const {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptRead))
            return ScriptValue();
        size_t           align = field->alignment > 0 ? field->alignment : 8;
        alignas(16) char buffer[64];
        // Correct implementation:
        bool  isHeap  = (field->size > 64 || align > 16);
        void *storage = nullptr;

        if (isHeap) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
            storage = shine::co::Memory::Alloc(field->size, align);
        } else {
            storage = buffer;
        }

        if (!field->isPod)
            Construct(storage, field->typeId);

        field->Get(instance, storage);
        ScriptValue result = bridge.ToScript(storage, field->typeId);

        if (!field->isPod)
            Destruct(storage, field->typeId);

        if (isHeap) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
            shine::co::Memory::Free(storage);
        }
        return result;
    }

    void SetField(void *instance, const FieldInfo *field, const ScriptValue &value, const ScriptBridge &bridge) const {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptWrite))
            return;
        size_t           align = field->alignment > 0 ? field->alignment : 8;
        alignas(16) char buffer[64];

        bool  isHeap  = (field->size > 64 || align > 16);
        void *storage = nullptr;

        if (isHeap) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
            storage = shine::co::Memory::Alloc(field->size, align);
        } else {
            storage = buffer;
        }

        if (!field->isPod)
            Construct(storage, field->typeId);

        bridge.FromScript(value, storage, field->typeId);
        field->Set(instance, storage);

        if (!field->isPod)
            Destruct(storage, field->typeId);

        if (isHeap) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
            shine::co::Memory::Free(storage);
        }
    }

    ScriptValue GetField(void *instance, std::string_view name, const ScriptBridge &bridge) const { return GetField(instance, GetFieldInfo(name), bridge); }
    void        SetField(void *instance, std::string_view name, const ScriptValue &val, const ScriptBridge &bridge) const { SetField(instance, GetFieldInfo(name), val, bridge); }

    // --- Method Access (String & Index - Zero String Support) ---
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

        // Prepare Args
        std::vector<void *> rawArgs(args.size());
        struct ArgBuffer {
            void *ptr;
            bool  isHeap;
            ArgBuffer() : ptr(nullptr), isHeap(false) {}
            ~ArgBuffer() {
                if (isHeap) {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::Script);
                    shine::co::Memory::Free(ptr);
                }
            }
        };
        std::vector<ArgBuffer> argBuffers(args.size());

        for (size_t i = 0; i < args.size(); ++i) {
            const TypeInfo *pType = GetTypeInfo(method->paramTypes[i]);
            if (!pType)
                return ScriptValue(); // Param type not found

            if (pType->size <= 8 && pType->alignment <= 8) {
                // Small optimization for simple types (int/float/bool) - Reuse pointer if possible or alloc small
                // But to be safe and uniform, we just alloc. For strict perf, use stack for small items.
                shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                argBuffers[i].ptr = shine::co::Memory::Alloc(pType->size, pType->alignment);
            } else {
                shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
                argBuffers[i].ptr = shine::co::Memory::Alloc(pType->size, pType->alignment);
            }
            argBuffers[i].isHeap = true;
            bridge.FromScript(args[i], argBuffers[i].ptr, method->paramTypes[i]);
            rawArgs[i] = argBuffers[i].ptr;
        }

        // Prepare Return
        const TypeInfo *rType  = (method->returnType != GetTypeId<void>()) ? GetTypeInfo(method->returnType) : nullptr;
        void           *retPtr = nullptr;
        ArgBuffer       retBuf;
        if (rType) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
            retBuf.ptr    = shine::co::Memory::Alloc(rType->size, rType->alignment);
            retBuf.isHeap = true;
            retPtr        = retBuf.ptr;
        }

        // Invoke
        method->invoke(instance, rawArgs.data(), retPtr);

        // Return
        if (rType && retPtr)
            return bridge.ToScript(retPtr, method->returnType);
        return ScriptValue();
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
// 6. DSL & Builder (Constexpr / C++20)
// -----------------------------------------------------------------------------
namespace shine::reflection::DSL {

struct MetaEntry {
    TypeId        key;
    MetadataValue value;
};

template <size_t NMeta>
struct FieldDescriptor {
    std::string_view             name {};
    TypeId                       typeId{};
    size_t                       offset{};
    size_t                       size{};
    size_t                       alignment{};
    bool                         isPod{};
    PropertyFlags                flags{PropertyFlags::None};
    std::array<MetaEntry, NMeta> metadata{};
    UI::Schema                   uiSchema                  = UI::None{};
    void (*onChange)(void *instance, const void *oldValue) = nullptr;
};

template <size_t NArgs, size_t NMeta>
struct MethodDescriptor {
    std::string_view             name;
    TypeId                       returnType;
    std::array<TypeId, NArgs>    paramTypes;
    FunctionFlags                flags;
    uint64_t                     signatureHash;
    std::array<MetaEntry, NMeta> metadata;
};


// 主模板
template <auto Ptr>
struct member_pointer_traits;

// 偏特化
template <typename Class, typename Member, Member Class::*Ptr>
struct member_pointer_traits<Ptr> {
    using ClassType  = Class;
    using MemberType = Member;

    static constexpr std::size_t offset() noexcept {
        return reinterpret_cast<std::size_t>(
            &(reinterpret_cast<Class const volatile *>(0)->*Ptr));
    }

    static_assert(!std::is_reference_v<Member>,
                  "Reflection does not support reference members");
};



template <auto MemberPtr, std::size_t NMeta = 0>
    requires std::is_member_object_pointer_v<decltype(MemberPtr)>
struct FieldDSLNode {
    using traits     = member_pointer_traits<MemberPtr>;
    using ClassType  = typename traits::ClassType;
    using MemberType = typename traits::MemberType;


    FieldDescriptor<NMeta> desc;


    constexpr FieldDSLNode(std::string_view name)
        : desc{
              name,
              GetTypeId<MemberType>(),
              traits::offset(),
              sizeof(MemberType),
              alignof(MemberType),
              std::is_trivially_copyable_v<MemberType>,
              PropertyFlags::None,
              {},
              UI::None{}} {}

    constexpr FieldDSLNode(FieldDescriptor<NMeta> d) : desc(d) {}

    constexpr auto Flags(PropertyFlags f) const {
        auto newDesc  = desc;
        newDesc.flags = newDesc.flags | f;
        return FieldDSLNode<MemberPtr, NMeta>(newDesc);
    }
    constexpr auto EditAnywhere() const { return Flags(PropertyFlags::EditAnywhere); }
    constexpr auto ReadOnly() const { return Flags(PropertyFlags::ReadOnly); }
    constexpr auto ScriptReadWrite() const { return Flags(PropertyFlags::ScriptReadWrite); }

    // ---------- UI ----------

    constexpr auto FunctionSelect(bool onlyScriptCallable = true) const {
        return UI(UI::FunctionSelector{onlyScriptCallable});
    }

    template <typename U>
    constexpr auto UI(U &&schema) const {
        auto newDesc     = desc;
        newDesc.uiSchema = std::forward<U>(schema);
        return FieldDSLNode<MemberPtr, NMeta>(newDesc);
    }

    // ---------- Metadata ----------

    template <typename V>
    constexpr auto Meta(TypeId key, V &&value) const {

        FieldDescriptor<NMeta + 1> newDesc;

        newDesc.name      = desc.name;
        newDesc.typeId    = desc.typeId;
        newDesc.offset    = desc.offset;
        newDesc.size      = desc.size;
        newDesc.alignment = desc.alignment;
        newDesc.isPod     = desc.isPod;
        newDesc.flags     = desc.flags;
        newDesc.uiSchema  = desc.uiSchema;
        newDesc.onChange  = desc.onChange;

        for (std::size_t i = 0; i < NMeta; ++i)
            newDesc.metadata[i] = desc.metadata[i];

        newDesc.metadata[NMeta] = {
            key,
            MetadataValue(std::forward<V>(value))};

        return FieldDSLNode<MemberPtr, NMeta + 1>(newDesc);
    }

    constexpr auto Meta(std::string_view key, auto &&value) const {
        return Meta(Hash(key), std::forward<decltype(value)>(value));
    }

    template <typename V>
    constexpr auto Range(V min, V max) const {
        return Meta("Min", min).Meta("Max", max);
    }

    constexpr auto DisplayName(std::string_view name) const {
        return Meta("DisplayName", name);
    }

    template <auto CallbackPtr>
        requires std::is_member_function_pointer_v<decltype(CallbackPtr)>
    constexpr auto OnChange() const {
        using Func = decltype(CallbackPtr);

        static_assert(
            std::is_invocable_v<Func, ClassType *> ||
                std::is_invocable_v<Func, ClassType *, const MemberType &>,
            "OnChange callback must be:\n"
            "  void ClassType::f()\n"
            "  void ClassType::f(const MemberType&)");

        auto newDesc     = desc;
        newDesc.onChange = [](void *instance, const void *oldValue) {
            auto *obj = static_cast<ClassType *>(instance);

            if constexpr (std::is_invocable_v<Func, ClassType *>) {
                (obj->*CallbackPtr)();
            } else {
                const auto &oldVal =
                    *static_cast<const MemberType *>(oldValue);
                (obj->*CallbackPtr)(oldVal);
            }
        };

        return FieldDSLNode<MemberPtr, NMeta>(newDesc);
    }
};

template <typename T, typename Ret, size_t NMeta, auto Func, typename... Args>
struct MethodDSLNode {
    using ClassType                                    = T;
    using ReturnType                                   = Ret;
    using ArgsTuple                                    = std::tuple<Args...>;
    static constexpr auto                    MethodPtr = Func;
    MethodDescriptor<sizeof...(Args), NMeta> desc;

    constexpr MethodDSLNode(std::string_view name) {
        desc.name                                  = name;
        desc.returnType                            = GetTypeId<Ret>();
        std::array<TypeId, sizeof...(Args)> params = {GetTypeId<Args>()...};
        desc.paramTypes                            = params;
        desc.flags                                 = FunctionFlags::None;
        desc.signatureHash                         = 2166136261u;
        for (auto t : params) {
            desc.signatureHash ^= t;
            desc.signatureHash *= 16777619u;
        }
    }
    constexpr MethodDSLNode(MethodDescriptor<sizeof...(Args), NMeta> d) : desc(d) {}

    constexpr auto Flags(FunctionFlags f) const {
        auto newDesc  = desc;
        newDesc.flags = newDesc.flags | f;
        return MethodDSLNode<T, Ret, NMeta, Func, Args...>(newDesc);
    }
    constexpr auto ScriptCallable() const { return Flags(FunctionFlags::ScriptCallable); }
    constexpr auto EditorCallable() const { return Flags(FunctionFlags::EditorCallable); }
    template <typename V>
    constexpr auto Meta(TypeId key, V &&value) const {
        MethodDescriptor<sizeof...(Args), NMeta + 1> newDesc;
        newDesc.name          = desc.name;
        newDesc.returnType    = desc.returnType;
        newDesc.paramTypes    = desc.paramTypes;
        newDesc.flags         = desc.flags;
        newDesc.signatureHash = desc.signatureHash;
        for (size_t i = 0; i < NMeta; ++i)
            newDesc.metadata[i] = desc.metadata[i];
        newDesc.metadata[NMeta] = {key, MetadataValue(std::forward<V>(value))};
        return MethodDSLNode<T, Ret, NMeta + 1, Func, Args...>(newDesc);
    }
    constexpr auto DisplayName(std::string_view name) const { return Meta("DisplayName", name); }
};

template <auto Ptr>
struct MethodDeducer;

template <typename C, typename R, typename... A, R (C::*Ptr)(A...)>
struct MethodDeducer<Ptr> {
    static constexpr auto Make(std::string_view name) {
        return MethodDSLNode<C, R, 0, Ptr, A...>(name);
    }
};

template <typename C, typename R, typename... A, R (C::*Ptr)(A...) const>
struct MethodDeducer<Ptr> {
    static constexpr auto Make(std::string_view name) {
        return MethodDSLNode<C, R, 0, Ptr, A...>(name);
    }
};

template <auto Ptr>
constexpr auto MakeMethodDSL(std::string_view name) { return MethodDeducer<Ptr>::Make(name); }
} // namespace shine::reflection::DSL

namespace shine::reflection {

template <typename Vec>
struct VectorThunks {
    static size_t           GetSize(const void *ptr) { return static_cast<const Vec *>(ptr)->size(); }
    static void            *GetElement(void *ptr, size_t index) { return &(*static_cast<Vec *>(ptr))[index]; }
    static const void      *GetElementConst(const void *ptr, size_t index) { return &(*static_cast<const Vec *>(ptr))[index]; }
    static void             Resize(void *ptr, size_t size) { static_cast<Vec *>(ptr)->resize(size); }
    static const ArrayTrait trait;
};

template <typename Vec>
const ArrayTrait VectorThunks<Vec>::trait = {GetTypeId<typename Vec::value_type>(), GetSize, GetElement, GetElementConst, Resize};

template <typename List>
struct ListThunks {
    static size_t GetSize(const void *ptr) { return static_cast<const List *>(ptr)->size(); }
    static void  *GetElement(void *ptr, size_t index) {
        auto it = static_cast<List *>(ptr)->begin();
        std::advance(it, index);
        return &(*it);
    }
    static const void *GetElementConst(const void *ptr, size_t index) {
        auto it = static_cast<const List *>(ptr)->begin();
        std::advance(it, index);
        return &(*it);
    }
    static void             Resize(void *ptr, size_t size) { static_cast<List *>(ptr)->resize(size); }
    static const ArrayTrait trait;
};
template <typename List>
const ArrayTrait ListThunks<List>::trait = {GetTypeId<typename List::value_type>(), GetSize, GetElement, GetElementConst, Resize};

template <typename Map>
struct MapThunks {
    using Key      = typename Map::key_type;
    using Value    = typename Map::mapped_type;
    using Iterator = typename Map::iterator;

    static size_t GetSize(const void *ptr) { return static_cast<const Map *>(ptr)->size(); }
    static void   Clear(void *ptr) { static_cast<Map *>(ptr)->clear(); }
    static void   Insert(void *ptr, const void *key, const void *val) {
        if constexpr (std::is_assignable_v<Value &, const Value &>) {
            // insert_or_assign is available for map and unordered_map in C++17
            // But check if map type actually supports it (e.g. multimap doesn't, but we only support map/unordered_map)
            // static_cast<Map*>(ptr)->insert_or_assign(*static_cast<const Key*>(key), *static_cast<const Value*>(val));
            // Standard insert_or_assign returns pair.
            (*static_cast<Map *>(ptr))[*static_cast<const Key *>(key)] = *static_cast<const Value *>(val);
        }
    }

    static void *Begin(void *ptr) {
        auto *it = new Iterator(static_cast<Map *>(ptr)->begin());
        return it;
    }
    static bool Valid(const void *iter, const void *ptr) {
        const Iterator *it = static_cast<const Iterator *>(iter);
        return *it != static_cast<const Map *>(ptr)->end();
    }
    static void Next(void *iter) {
        Iterator *it = static_cast<Iterator *>(iter);
        ++(*it);
    }
    static const void *GetKey(const void *iter) {
        const Iterator *it = static_cast<const Iterator *>(iter);
        return &(*it)->first;
    }
    static void *GetValue(const void *iter) {
        const Iterator *it = static_cast<const Iterator *>(iter);
        return &(*it)->second;
    }
    static void DestroyIterator(void *iter) {
        delete static_cast<Iterator *>(iter);
    }

    static const MapTrait trait;
};

template <typename Map>
const MapTrait MapThunks<Map>::trait = {
    GetTypeId<typename Map::key_type>(),
    GetTypeId<typename Map::mapped_type>(),
    GetSize, Clear, Insert, Begin, Valid, Next, GetKey, GetValue, DestroyIterator};

// Invoke Thunks
template <typename Class, typename Ret, typename... Args, size_t... Is>
void InvokeThunkImpl(Class *instance, Ret (Class::*Func)(Args...), void **args, void *ret, std::index_sequence<Is...>) {
    if constexpr (std::is_same_v<Ret, void>) {
        (instance->*Func)(*static_cast<std::remove_reference_t<Args> *>(args[Is])...);
    } else {
        if (ret)
            new (ret) Ret((instance->*Func)(*static_cast<std::remove_reference_t<Args> *>(args[Is])...));
    }
}

template <typename Class, typename Ret, typename... Args, size_t... Is>
void InvokeThunkImpl(const Class *instance, Ret (Class::*Func)(Args...) const, void **args, void *ret, std::index_sequence<Is...>) {
    if constexpr (std::is_same_v<Ret, void>) {
        (instance->*Func)(*static_cast<std::remove_reference_t<Args> *>(args[Is])...);
    } else {
        if (ret)
            new (ret) Ret((instance->*Func)(*static_cast<std::remove_reference_t<Args> *>(args[Is])...));
    }
}

template <typename T>
struct TypeBuilder;

// Forward declare FieldInjector_Impl and MethodInjector_Impl
template <typename T, typename DSLType>
struct FieldInjector_Impl;
template <typename T, typename DSLType>
struct MethodInjector_Impl;



template <typename T, typename DSLType>
struct FieldInjector_Impl {
    TypeBuilder<T> &builder;
    DSLType         dsl;
    bool            moved = false;

    constexpr FieldInjector_Impl(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

    ~FieldInjector_Impl() {
        if (!moved) {
           // builder.template RegisterFieldImpl<typename DSLType::ClassType, typename DSLType::traits, DSLType::traits>(dsl.desc);
        }
    }
    constexpr FieldInjector_Impl(FieldInjector_Impl &&other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }

    constexpr auto EditAnywhere() { return Chain(dsl.EditAnywhere()); }
    constexpr auto ReadOnly() { return Chain(dsl.ReadOnly()); }
    constexpr auto ScriptReadWrite() { return Chain(dsl.ScriptReadWrite()); }
    constexpr auto FunctionSelect(bool onlyScriptCallable = true) { return Chain(dsl.FunctionSelect(onlyScriptCallable)); }

    template <auto CallbackPtr>
    constexpr auto OnChange() {
        return Chain(dsl.template OnChange<CallbackPtr>());
    }

    template <typename U>
    constexpr auto UI(U &&schema) { return Chain(dsl.UI(std::forward<U>(schema))); }

    template <size_t N, typename V>
    constexpr auto Meta(const char (&key)[N], V &&val) {
        return Chain(dsl.Meta(Hash(key), std::forward<V>(val)));
    }

    // Fallback for runtime strings
    template <typename V>
    constexpr auto Meta(std::string_view key, V &&val) {
        return Chain(dsl.Meta(Hash(key), std::forward<V>(val)));
    }

    template <typename V>
    requires ( IsNumberAndFloat<V> )
    constexpr auto Range(V min, V max) { return Chain(dsl.Range(min, max)); }

    template <size_t N>
    constexpr auto DisplayName(const char (&name)[N]) { return Meta("DisplayName", name); }
    constexpr auto DisplayName(std::string_view name) { return Meta("DisplayName", name); }

    template <size_t N>
    constexpr auto Category(const char (&name)[N]) { return Meta("Category", name); }
    constexpr auto Category(std::string_view name) { return Meta("Category", name); }

    template <size_t N>
    constexpr auto EditCondition(const char (&condition)[N]) { return Meta("EditCondition", condition); }
    constexpr auto EditCondition(std::string_view condition) { return Meta("EditCondition", condition); }

    private:
    template <typename NewDSL>
    constexpr auto Chain(NewDSL newDSL) {
        moved = true;
        return FieldInjector_Impl<T, NewDSL>(builder, newDSL);
    }
};

template <typename T, typename DSLType>
struct MethodInjector_Impl {
    TypeBuilder<T> &builder;
    DSLType         dsl;
    bool            moved = false;
    MethodInjector_Impl(TypeBuilder<T> &b, DSLType d) : builder(b), dsl(d) {}

    // Fix: Call RegisterMethodImpl directly to avoid recursion
    ~MethodInjector_Impl() {
        if (!moved) {
            builder.template RegisterMethodImpl<typename DSLType::ClassType, DSLType::MethodPtr, typename DSLType::ReturnType, typename DSLType::ArgsTuple>(dsl.desc);
        }
    }
    MethodInjector_Impl(MethodInjector_Impl &&other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }
    auto ScriptCallable() { return Chain(dsl.ScriptCallable()); }
    auto EditorCallable() { return Chain(dsl.EditorCallable()); }
    template <typename V>
    auto Meta(std::string_view key, V &&val) { return Chain(dsl.Meta(Hash(key), std::forward<V>(val))); }
    template <size_t N>
    auto DisplayName(const char (&name)[N]) { return Meta("DisplayName", name); }
    auto DisplayName(std::string_view name) { return Meta("DisplayName", name); }

    private:
    template <typename NewDSL>
    auto Chain(NewDSL newDSL) {
        moved = true;
        return MethodInjector_Impl<T, NewDSL>(builder, newDSL);
    }
};

template <typename T>
template <typename DSLType>
constexpr  auto TypeBuilder<T>::RegisterFieldFromDSL(const DSLType &dsl) {
    return FieldInjector_Impl<T, DSLType>(*this, dsl);
}

template <typename T>
template <typename DSLType>
constexpr auto TypeBuilder<T>::RegisterMethodFromDSL(const DSLType &dsl) {
    return MethodInjector_Impl<T, DSLType>(*this, dsl);
}
} // namespace shine::reflection

// -----------------------------------------------------------------------------
// 7. Macros
// -----------------------------------------------------------------------------

template <typename Builder>
using BuilderType = typename std::remove_reference_t<Builder>::ObjectType;

#define REFLECTION_REGISTER(Type)                            \
    inline const auto Type##_Reg = []() {                    \
        shine::reflection::TypeBuilder<Type> builder(#Type); \
        Type##Register<Type##RegisterReflection>::call(builder);   \
        builder.Register();                                  \
        return true;                                         \
    }();

#define REFLECTION_STRUCT(Type)                                                     \
    static shine::reflection::TypeBuilder<Type> Type##builder(#Type);               \
    template<auto F>  struct Type##Register {                                       \
        static constexpr decltype(auto) call(auto &&args) {                         \
            return F(std::forward<decltype(args)>(args));                           \ 
        }                                                                           \
    };                                                                              \
    void Type##RegisterReflection(shine::reflection::TypeBuilder<Type> &builder) 



#define REFLECT_FIELD(Member)                 \
    builder.RegisterFieldFromDSL(             \
        shine::reflection::DSL::FieldDSLNode< \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member>(#Member))

#define REFLECT_METHOD(Member)                 \
    builder.RegisterMethodFromDSL(             \
        shine::reflection::DSL::MakeMethodDSL< \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member>(#Member))

#define REFLECT_ENUM(Type)                                                           \
    void              Type##_Reflect(shine::reflection::TypeBuilder<Type> &builder); \
    inline const auto Type##_Reg = []() {                                            \
        shine::reflection::TypeBuilder<Type> builder(#Type);                         \
        Type##_Reflect(builder);                                                     \
        builder.Register();                                                          \
        return true;                                                                 \
    }();                                                                             \
    void Type##_Reflect(shine::reflection::TypeBuilder<Type> &builder)
