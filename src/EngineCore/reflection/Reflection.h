#pragma once

#include <string_view>
#include <vector>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <tuple>
#include <array>
#include <variant>
#include <source_location>
#include <memory>
#include <cstdint>
#include <string>
#include <cstring>
#include <cstddef>
#include <concepts>
#include <cstdlib>
#include <utility>

// -----------------------------------------------------------------------------
// 1. Core Hash & TypeID (Stable, Compile-time)
// -----------------------------------------------------------------------------
namespace Shine::Reflection {

    using TypeId = uint32_t;

    constexpr uint32_t Hash(std::string_view str) {
        uint32_t hash = 2166136261u;
        for (char c : str) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 16777619u;
        }
        return hash;
    }

    template <typename T>
    consteval std::string_view GetTypeName() {
#if defined(__clang__) || defined(__GNUC__)
        std::string_view name = __PRETTY_FUNCTION__;
        size_t start = name.find("T = ") + 4;
        size_t end = name.find_last_of(']');
        return name.substr(start, end - start);
#elif defined(_MSC_VER)
        std::string_view name = __FUNCSIG__;
        size_t start = name.find("GetTypeName<") + 12;
        size_t end = name.find_last_of('>');
        auto sub = name.substr(start, end - start);
        if (sub.starts_with("struct ")) return sub.substr(7);
        if (sub.starts_with("class ")) return sub.substr(6);
        return sub;
#else
        return "Unknown";
#endif
    }

    template <typename T>
    consteval TypeId GetTypeId() {
        return Hash(GetTypeName<T>());
    }
}

// -----------------------------------------------------------------------------
// 2. Type Erasure & Script Bridge
// -----------------------------------------------------------------------------
namespace Shine::Reflection {

    struct TypeInfo;

    struct ScriptValue {
        enum class Type { Null, Bool, Int64, Double, Pointer };
        Type type = Type::Null;
        TypeId ptrTypeId = 0; // For Pointer type
        union {
            bool bValue;
            int64_t iValue;
            double dValue;
            void* pValue;
        };
        ScriptValue() : type(Type::Null), pValue(nullptr) {}
        ScriptValue(bool v) : type(Type::Bool), bValue(v) {}
        ScriptValue(int64_t v) : type(Type::Int64), iValue(v) {}
        ScriptValue(double v) : type(Type::Double), dValue(v) {}
        ScriptValue(void* v, TypeId tid = 0) : type(Type::Pointer), pValue(v), ptrTypeId(tid) {}
        template<typename T> T As() const;
    };

    template<> inline bool ScriptValue::As<bool>() const { return type == Type::Bool ? bValue : false; }
    template<> inline int64_t ScriptValue::As<int64_t>() const { return type == Type::Int64 ? iValue : (type == Type::Double ? (int64_t)dValue : 0); }
    template<> inline double ScriptValue::As<double>() const { return type == Type::Double ? dValue : (type == Type::Int64 ? (double)iValue : 0.0); }
    template<> inline void* ScriptValue::As<void*>() const { return type == Type::Pointer ? pValue : nullptr; }
    template<> inline int ScriptValue::As<int>() const { return (int)As<int64_t>(); }
    template<> inline float ScriptValue::As<float>() const { return (float)As<double>(); }

    struct ScriptBridge {
        using ToScriptFunc = ScriptValue (*)(void* context, const void* src, TypeId typeId);
        using FromScriptFunc = void (*)(void* context, const ScriptValue& val, void* dst, TypeId typeId);
        void* context = nullptr;
        ToScriptFunc toScript = nullptr;
        FromScriptFunc fromScript = nullptr;
        ScriptValue ToScript(const void* src, TypeId typeId) const {
            return toScript ? toScript(context, src, typeId) : ScriptValue();
        }
        void FromScript(const ScriptValue& val, void* dst, TypeId typeId) const {
            if (fromScript) fromScript(context, val, dst, typeId);
        }
    };

    using GetterFunc = void(*)(const void* instance, void* out_value, size_t offset, size_t size);
    using SetterFunc = void(*)(void* instance, const void* in_value, size_t offset, size_t size);

    inline void MemcpyGetter(const void* instance, void* out_value, size_t offset, size_t size) {
        std::memcpy(out_value, static_cast<const char*>(instance) + offset, size);
    }
    inline void MemcpySetter(void* instance, const void* in_value, size_t offset, size_t size) {
        std::memcpy(static_cast<char*>(instance) + offset, in_value, size);
    }
    template <typename Class, typename MemberType, MemberType Class::*Member>
    void GenericGetter(const void* instance, void* out_value, size_t /*offset*/, size_t /*size*/) {
        const auto* obj = static_cast<const Class*>(instance);
        *static_cast<MemberType*>(out_value) = obj->*Member;
    }
    template <typename Class, typename MemberType, MemberType Class::*Member>
    void GenericSetter(void* instance, const void* in_value, size_t /*offset*/, size_t /*size*/) {
        auto* obj = static_cast<Class*>(instance);
        obj->*Member = *static_cast<const MemberType*>(in_value);
    }
}

// -----------------------------------------------------------------------------
// 3. Metadata & Info Structures
// -----------------------------------------------------------------------------
namespace Shine::Reflection {

    enum class PropertyFlags : uint64_t {
        None            = 0,
        EditAnywhere    = 1 << 0,
        ReadOnly        = 1 << 1,
        Transient       = 1 << 2,
        ScriptRead      = 1 << 3,
        ScriptWrite     = 1 << 4,
        ScriptReadWrite = ScriptRead | ScriptWrite,
        SaveGame        = 1 << 5,
    };
    constexpr PropertyFlags operator|(PropertyFlags lhs, PropertyFlags rhs) { return (PropertyFlags)((uint64_t)lhs | (uint64_t)rhs); }
    constexpr bool HasFlag(PropertyFlags flags, PropertyFlags check) { return ((uint64_t)flags & (uint64_t)check) == (uint64_t)check; }

    enum class FunctionFlags : uint64_t {
        None            = 0,
        ScriptCallable  = 1 << 0,
        EditorCallable  = 1 << 1,
        Const           = 1 << 2, 
        Static          = 1 << 3,
    };
    constexpr FunctionFlags operator|(FunctionFlags lhs, FunctionFlags rhs) { return (FunctionFlags)((uint64_t)lhs | (uint64_t)rhs); }
    constexpr bool HasFlag(FunctionFlags flags, FunctionFlags check) { return ((uint64_t)flags & (uint64_t)check) == (uint64_t)check; }

    using MetadataKey = TypeId;
    using MetadataValue = std::variant<int, float, bool, std::string_view>;
    using MetadataContainer = std::vector<std::pair<MetadataKey, MetadataValue>>;

    // UI Schema (Constexpr Friendly)
    namespace UI {
        struct None {};
        struct Slider { float min = 0.0f; float max = 0.0f; float step = 1.0f; };
        struct Color { bool hasAlpha = true; };
        struct Checkbox {};
        struct InputText { size_t maxLength = 256; };
        using Schema = std::variant<None, Slider, Color, Checkbox, InputText>;
    }

    struct ArrayTrait {
        size_t (*getSize)(const void* arrayPtr);
        void* (*getElement)(void* arrayPtr, size_t index);
        const void* (*getElementConst)(const void* arrayPtr, size_t index);
        void (*resize)(void* arrayPtr, size_t newSize);
    };

    struct FieldInfo {
        std::string_view name;
        TypeId typeId;
        size_t offset;
        size_t size;
        size_t alignment;
        
        GetterFunc getter;
        SetterFunc setter;
        
        bool (*equals)(const void* a, const void* b, size_t size);
        void (*copy)(void* dst, const void* src, size_t size);

        bool isPod;
        const ArrayTrait* arrayTrait = nullptr; 
        PropertyFlags flags = PropertyFlags::None;
        MetadataContainer metadata;
        UI::Schema uiSchema = UI::None{};

        const MetadataValue* GetMeta(MetadataKey key) const {
            auto it = std::lower_bound(metadata.begin(), metadata.end(), key, [](const auto& pair, MetadataKey k) { return pair.first < k; });
            if (it != metadata.end() && it->first == key) return &it->second;
            return nullptr;
        }

        void Get(const void* instance, void* out_value) const { getter(instance, out_value, offset, size); }
        void Set(void* instance, const void* in_value) const { setter(instance, in_value, offset, size); }
    };

    struct MethodInfo {
        std::string_view name;
        using InvokeFunc = void (*)(void* instance, void** args, void* ret);
        InvokeFunc invoke;
        TypeId returnType;
        std::vector<TypeId> paramTypes;
        uint64_t signatureHash = 0; 
        FunctionFlags flags = FunctionFlags::None;
        MetadataContainer metadata;
    };

    struct EnumEntry { int64_t value; std::string_view name; };

    struct TypeInfo {
        std::string_view name;
        TypeId id;
        size_t size;
        size_t alignment;
        
        std::vector<FieldInfo> fields;
        std::vector<MethodInfo> methods;
        
        const TypeInfo* baseType = nullptr;
        TypeId baseTypeId = 0;
        
        bool isEnum = false;
        std::vector<EnumEntry> enumEntries;

        void* (*create)(); 
        void (*destroy)(void*);
        bool isTrivial;

        const FieldInfo* FindField(std::string_view fieldName) const {
            auto it = std::find_if(fields.begin(), fields.end(), [fieldName](const FieldInfo& f) { return f.name == fieldName; });
            if (it != fields.end()) return &(*it);
            return baseType ? baseType->FindField(fieldName) : nullptr;
        }
        const MethodInfo* FindMethod(std::string_view methodName) const {
             auto it = std::find_if(methods.begin(), methods.end(), [methodName](const MethodInfo& m) { return m.name == methodName; });
            if (it != methods.end()) return &(*it);
            return baseType ? baseType->FindMethod(methodName) : nullptr;
        }
    };
}

// -----------------------------------------------------------------------------
// 4. Registry
// -----------------------------------------------------------------------------
namespace Shine::Reflection {
    class TypeRegistry {
    public:
        static TypeRegistry& Get() { static TypeRegistry instance; return instance; }
        void Register(TypeInfo info) {
            auto it = std::lower_bound(types.begin(), types.end(), info.id, [](const TypeInfo& t, TypeId id) { return t.id < id; });
            if (it == types.end() || it->id != info.id) types.insert(it, std::move(info));
        }
        const TypeInfo* Find(TypeId id) const {
            auto it = std::lower_bound(types.begin(), types.end(), id, [](const TypeInfo& t, TypeId id) { return t.id < id; });
            if (it != types.end() && it->id == id) return &(*it);
            return nullptr;
        }
        template<typename T> const TypeInfo* Find() const { return Find(GetTypeId<T>()); }
        static void RegisterAllTypes();
        std::vector<TypeInfo> types;
    };
    inline std::vector<TypeInfo>& GetPendingTypes() { static std::vector<TypeInfo> pending; return pending; }
    
    inline void TypeRegistry::RegisterAllTypes() {
        auto& registry = Get();
        auto& pending = GetPendingTypes();
        for (auto& t : pending) registry.Register(std::move(t));
        pending.clear();
        for (auto& t : registry.types) {
            if (t.baseTypeId != 0) t.baseType = registry.Find(t.baseTypeId);
        }
    }
}

// -----------------------------------------------------------------------------
// 5. Views (Inspector / Script / ECS)
// -----------------------------------------------------------------------------
namespace Shine::Reflection {

    struct TypeView {
        const TypeInfo* typeInfo = nullptr;
        bool IsValid() const { return typeInfo != nullptr; }
    };

    struct InspectorView : public TypeView {
        struct FieldIterator {
            const TypeInfo* type;
            size_t index;
            bool operator!=(const FieldIterator& other) const { return index != other.index; }
            void operator++() { ++index; }
            const FieldInfo& operator*() const { return type->fields[index]; }
        };
        FieldIterator begin() const { return { typeInfo, 0 }; }
        FieldIterator end() const { return { typeInfo, typeInfo->fields.size() }; }
        
        bool IsEditable(const FieldInfo& field) const {
            return HasFlag(field.flags, PropertyFlags::EditAnywhere) && !HasFlag(field.flags, PropertyFlags::ReadOnly);
        }
        const UI::Schema& GetUISchema(const FieldInfo& field) const { return field.uiSchema; }
        void SetValue(void* instance, const FieldInfo& field, const void* value) const {
            if (IsEditable(field)) field.Set(instance, value);
        }
    };

    struct ScriptView : public TypeView {
        static const TypeInfo* GetTypeInfo(TypeId id) { return TypeRegistry::Get().Find(id); }

        // --- Field Access (String & Index) ---
        const FieldInfo* GetFieldInfo(std::string_view name) const { return typeInfo->FindField(name); }
        const FieldInfo* GetFieldInfo(size_t index) const { 
            if (index < typeInfo->fields.size()) return &typeInfo->fields[index];
            return nullptr; 
        }

        ScriptValue GetField(void* instance, const FieldInfo* field, const ScriptBridge& bridge) const {
            if (!field || !HasFlag(field->flags, PropertyFlags::ScriptRead)) return ScriptValue();
            size_t align = field->alignment > 0 ? field->alignment : 8;
            alignas(16) char buffer[64]; 
            void* storage = (field->size <= 64 && align <= 16) ? buffer : _aligned_malloc(field->size, align);
            field->Get(instance, storage);
            ScriptValue result = bridge.ToScript(storage, field->typeId);
            if (storage != buffer) _aligned_free(storage);
            return result;
        }
        
        void SetField(void* instance, const FieldInfo* field, const ScriptValue& value, const ScriptBridge& bridge) const {
            if (!field || !HasFlag(field->flags, PropertyFlags::ScriptWrite)) return;
            size_t align = field->alignment > 0 ? field->alignment : 8;
            alignas(16) char buffer[64];
            void* storage = (field->size <= 64 && align <= 16) ? buffer : _aligned_malloc(field->size, align);
            bridge.FromScript(value, storage, field->typeId);
            field->Set(instance, storage);
            if (storage != buffer) _aligned_free(storage);
        }

        ScriptValue GetField(void* instance, std::string_view name, const ScriptBridge& bridge) const { return GetField(instance, GetFieldInfo(name), bridge); }
        void SetField(void* instance, std::string_view name, const ScriptValue& val, const ScriptBridge& bridge) const { SetField(instance, GetFieldInfo(name), val, bridge); }

        // --- Method Access (String & Index - Zero String Support) ---
        const MethodInfo* GetMethodInfo(std::string_view name) const { return typeInfo->FindMethod(name); }
        const MethodInfo* GetMethodInfo(size_t index) const {
            if (index < typeInfo->methods.size()) return &typeInfo->methods[index];
            return nullptr;
        }

        ScriptValue CallMethod(void* instance, const MethodInfo* method, const std::vector<ScriptValue>& args, const ScriptBridge& bridge) const {
            if (!method || !HasFlag(method->flags, FunctionFlags::ScriptCallable)) return ScriptValue();
            if (args.size() != method->paramTypes.size()) return ScriptValue();
            
            // Prepare Args
            std::vector<void*> rawArgs(args.size());
            struct ArgBuffer { void* ptr; bool isHeap; ArgBuffer() : ptr(nullptr), isHeap(false) {} ~ArgBuffer() { if(isHeap) _aligned_free(ptr); } };
            std::vector<ArgBuffer> argBuffers(args.size());
            
            for (size_t i = 0; i < args.size(); ++i) {
                const TypeInfo* pType = GetTypeInfo(method->paramTypes[i]);
                if (!pType) return ScriptValue(); // Param type not found
                
                if (pType->size <= 8 && pType->alignment <= 8) {
                    // Small optimization for simple types (int/float/bool) - Reuse pointer if possible or alloc small
                    // But to be safe and uniform, we just alloc. For strict perf, use stack for small items.
                    argBuffers[i].ptr = _aligned_malloc(pType->size, pType->alignment);
                } else {
                    argBuffers[i].ptr = _aligned_malloc(pType->size, pType->alignment);
                }
                argBuffers[i].isHeap = true;
                bridge.FromScript(args[i], argBuffers[i].ptr, method->paramTypes[i]);
                rawArgs[i] = argBuffers[i].ptr;
            }

            // Prepare Return
            const TypeInfo* rType = (method->returnType != GetTypeId<void>()) ? GetTypeInfo(method->returnType) : nullptr;
            void* retPtr = nullptr;
            ArgBuffer retBuf;
            if (rType) {
                retBuf.ptr = _aligned_malloc(rType->size, rType->alignment);
                retBuf.isHeap = true;
                retPtr = retBuf.ptr;
            }

            // Invoke
            method->invoke(instance, rawArgs.data(), retPtr);

            // Return
            if (rType && retPtr) return bridge.ToScript(retPtr, method->returnType);
            return ScriptValue();
        }

        ScriptValue CallMethod(void* instance, std::string_view name, const std::vector<ScriptValue>& args, const ScriptBridge& bridge) const {
            return CallMethod(instance, GetMethodInfo(name), args, bridge);
        }
    };

    struct ECSView {
        struct ComponentLayout { size_t size; size_t alignment; const TypeInfo* layoutSource; };
        ComponentLayout layout;
        size_t GetSize() const { return layout.size; }
        size_t GetAlignment() const { return layout.alignment; }
    };
}

// -----------------------------------------------------------------------------
// 6. DSL & Builder (Constexpr / C++20)
// -----------------------------------------------------------------------------
namespace Shine::Reflection::DSL {

    struct MetaEntry { TypeId key; MetadataValue value; };

    template<size_t NMeta>
    struct FieldDescriptor {
        std::string_view name;
        TypeId typeId;
        size_t offset;
        size_t size;
        size_t alignment;
        bool isPod;
        PropertyFlags flags;
        std::array<MetaEntry, NMeta> metadata;
        UI::Schema uiSchema = UI::None{};
    };

    template<size_t NArgs, size_t NMeta>
    struct MethodDescriptor {
        std::string_view name;
        TypeId returnType;
        std::array<TypeId, NArgs> paramTypes;
        FunctionFlags flags;
        uint64_t signatureHash;
        std::array<MetaEntry, NMeta> metadata;
    };

    template<typename T, typename FieldType, FieldType T::*Ptr, size_t NMeta>
    struct FieldDSLNode {
        using ClassType = T;
        using MemberType = FieldType;
        static constexpr FieldType T::*MemberPtr = Ptr;
        FieldDescriptor<NMeta> desc;

        constexpr FieldDSLNode(std::string_view name, size_t offset) : desc{
            name, GetTypeId<FieldType>(), offset, sizeof(FieldType), alignof(FieldType),
            std::is_trivially_copyable_v<FieldType>, PropertyFlags::None, {}, UI::None{}
        } {}
        constexpr FieldDSLNode(FieldDescriptor<NMeta> d) : desc(d) {}

        constexpr auto Flags(PropertyFlags f) const {
            auto newDesc = desc; newDesc.flags = newDesc.flags | f;
            return FieldDSLNode<T, FieldType, Ptr, NMeta>(newDesc);
        }
        constexpr auto EditAnywhere() const { return Flags(PropertyFlags::EditAnywhere); }
        constexpr auto ReadOnly() const { return Flags(PropertyFlags::ReadOnly); }
        constexpr auto ScriptReadWrite() const { return Flags(PropertyFlags::ScriptReadWrite); }
        template <typename U> constexpr auto UI(U&& schema) const {
            auto newDesc = desc; newDesc.uiSchema = std::forward<U>(schema);
            return FieldDSLNode<T, FieldType, Ptr, NMeta>(newDesc);
        }
        template <typename V> constexpr auto Meta(TypeId key, V&& value) const {
            FieldDescriptor<NMeta + 1> newDesc;
            newDesc.name = desc.name; newDesc.typeId = desc.typeId; newDesc.offset = desc.offset;
            newDesc.size = desc.size; newDesc.alignment = desc.alignment; newDesc.isPod = desc.isPod;
            newDesc.flags = desc.flags; newDesc.uiSchema = desc.uiSchema;
            for(size_t i=0; i<NMeta; ++i) newDesc.metadata[i] = desc.metadata[i];
            newDesc.metadata[NMeta] = { key, MetadataValue(std::forward<V>(value)) };
            return FieldDSLNode<T, FieldType, Ptr, NMeta + 1>(newDesc);
        }
        constexpr auto Meta(std::string_view key, auto&& value) const { return Meta(Hash(key), value); }
        template <typename V> constexpr auto Range(V min, V max) const {
            return Meta("Min", min).Meta("Max", max);
        }
    };

    template<typename T, typename Ret, size_t NMeta, auto Func, typename... Args>
    struct MethodDSLNode {
        using ClassType = T;
        using ReturnType = Ret;
        using ArgsTuple = std::tuple<Args...>;
        static constexpr auto MethodPtr = Func;
        MethodDescriptor<sizeof...(Args), NMeta> desc;

        consteval MethodDSLNode(std::string_view name) {
            desc.name = name; desc.returnType = GetTypeId<Ret>();
            std::array<TypeId, sizeof...(Args)> params = { GetTypeId<Args>()... };
            desc.paramTypes = params; desc.flags = FunctionFlags::None;
            desc.signatureHash = 2166136261u;
            for(auto t : params) { desc.signatureHash ^= t; desc.signatureHash *= 16777619u; }
        }
        consteval MethodDSLNode(MethodDescriptor<sizeof...(Args), NMeta> d) : desc(d) {}

        consteval auto Flags(FunctionFlags f) const {
            auto newDesc = desc; newDesc.flags = newDesc.flags | f;
            return MethodDSLNode<T, Ret, NMeta, Func, Args...>(newDesc);
        }
        consteval auto ScriptCallable() const { return Flags(FunctionFlags::ScriptCallable); }
        consteval auto EditorCallable() const { return Flags(FunctionFlags::EditorCallable); }
        template <typename V> consteval auto Meta(TypeId key, V&& value) const {
            MethodDescriptor<sizeof...(Args), NMeta + 1> newDesc;
            newDesc.name = desc.name; newDesc.returnType = desc.returnType;
            newDesc.paramTypes = desc.paramTypes; newDesc.flags = desc.flags; newDesc.signatureHash = desc.signatureHash;
            for(size_t i=0; i<NMeta; ++i) newDesc.metadata[i] = desc.metadata[i];
            newDesc.metadata[NMeta] = { key, MetadataValue(std::forward<V>(value)) };
            return MethodDSLNode<T, Ret, NMeta + 1, Func, Args...>(newDesc);
        }
    };

    template<typename Class, typename Ret, typename... Args>
    consteval auto MakeMethodDSLHelper(Ret(Class::*Ptr)(Args...), std::string_view name) { return MethodDSLNode<Class, Ret, 0, Ptr, Args...>(name); }
    template<typename Class, typename Ret, typename... Args>
    consteval auto MakeMethodDSLHelper(Ret(Class::*Ptr)(Args...) const, std::string_view name) { return MethodDSLNode<Class, Ret, 0, Ptr, Args...>(name); }
    template<auto Ptr> consteval auto MakeMethodDSL(std::string_view name) { return MakeMethodDSLHelper(Ptr, name); }
}

namespace Shine::Reflection {

    template<typename T> struct IsVector : std::false_type {};
    template<typename T, typename A> struct IsVector<std::vector<T, A>> : std::true_type {};

    template<typename Vec> struct VectorThunks {
        static size_t GetSize(const void* ptr) { return static_cast<const Vec*>(ptr)->size(); }
        static void* GetElement(void* ptr, size_t index) { return &(*static_cast<Vec*>(ptr))[index]; }
        static const void* GetElementConst(const void* ptr, size_t index) { return &(*static_cast<const Vec*>(ptr))[index]; }
        static void Resize(void* ptr, size_t size) { static_cast<Vec*>(ptr)->resize(size); }
        static const ArrayTrait trait;
    };
    template<typename Vec> const ArrayTrait VectorThunks<Vec>::trait = { GetSize, GetElement, GetElementConst, Resize };

    template<typename T>
    struct TypeBuilder;

    // Forward declare FieldInjector_Impl and MethodInjector_Impl
    template<typename T, typename DSLType> struct FieldInjector_Impl;
    template<typename T, typename DSLType> struct MethodInjector_Impl;

    // Define TypeBuilder first (but with forward declared methods if needed, or inline)
    template<typename T>
    struct TypeBuilder {
        using ObjectType = T;
        TypeInfo info;
        TypeBuilder(std::string_view name) {
            info.name = name; info.id = GetTypeId<T>(); info.size = sizeof(T); info.alignment = alignof(T);
            info.create = []() -> void* { return new T(); };
            info.destroy = [](void* ptr) { delete static_cast<T*>(ptr); };
            info.isTrivial = std::is_trivially_copyable_v<T>;
        }
        template<typename BaseType> void Base() { info.baseTypeId = GetTypeId<BaseType>(); }
        void Enum(std::string_view name, int64_t value) { info.isEnum = true; info.enumEntries.push_back({ value, name }); }
        
        template<typename DSLType> auto RegisterFieldFromDSL(const DSLType& dsl);
        template<typename DSLType> auto RegisterMethodFromDSL(const DSLType& dsl);

        void Register() { GetPendingTypes().push_back(std::move(info)); }

    private:
        template<typename Class, typename MemberType, MemberType Class::*Ptr, size_t N>
        void RegisterFieldImpl(const DSL::FieldDescriptor<N>& desc) {
             FieldInfo field;
             field.name = desc.name; field.typeId = desc.typeId; field.offset = desc.offset;
             field.size = desc.size; field.alignment = desc.alignment; field.isPod = desc.isPod;
             field.flags = desc.flags; field.uiSchema = desc.uiSchema;
             for(const auto& m : desc.metadata) {
                auto it = std::lower_bound(field.metadata.begin(), field.metadata.end(), m.key, [](const auto& pair, TypeId k) { return pair.first < k; });
                field.metadata.insert(it, { m.key, m.value });
             }
             if constexpr (IsVector<MemberType>::value) field.arrayTrait = &VectorThunks<MemberType>::trait;
             if (field.isPod) {
                field.getter = MemcpyGetter; field.setter = MemcpySetter;
                field.equals = [](const void* a, const void* b, size_t size) { return std::memcmp(a, b, size) == 0; };
                field.copy = [](void* dst, const void* src, size_t size) { std::memcpy(dst, src, size); };
            } else {
                field.getter = GenericGetter<Class, MemberType, Ptr>; field.setter = GenericSetter<Class, MemberType, Ptr>;
                field.equals = nullptr; field.copy = nullptr;
            }
            info.fields.push_back(std::move(field));
        }

        template<typename Class, auto MethodPtr, typename Ret, typename ArgsTuple, size_t NArgs, size_t NMeta>
        void RegisterMethodImpl(const DSL::MethodDescriptor<NArgs, NMeta>& desc) {
            std::apply([&](auto... args) { RegisterMethodThunkImpl<Class, MethodPtr, Ret, decltype(args)...>(desc); }, ArgsTuple{});
        }
        
        template<typename Class, auto MethodPtr, typename Ret, typename... Args, size_t NArgs, size_t NMeta>
        void RegisterMethodThunkImpl(const DSL::MethodDescriptor<NArgs, NMeta>& desc) {
             MethodInfo method;
             method.name = desc.name; method.returnType = desc.returnType;
             method.paramTypes = { desc.paramTypes.begin(), desc.paramTypes.end() };
             method.signatureHash = desc.signatureHash; method.flags = desc.flags;
             for(const auto& m : desc.metadata) method.metadata.push_back({ m.key, m.value });
             std::sort(method.metadata.begin(), method.metadata.end(), [](auto& a, auto& b){ return a.first < b.first; });
             GenerateInvokeHelper<Class, Ret, Args...>(method, MethodPtr);
             info.methods.push_back(std::move(method));
        }

        template<typename Class, typename Ret, typename... Args>
        void GenerateInvokeHelper(MethodInfo& outMethod, Ret(Class::*Func)(Args...)) {
            outMethod.invoke = [](void* inst, void** args, void* ret) {
                InvokeThunkImpl<Class, Ret, Args...>(static_cast<Class*>(inst), Func, args, ret, std::make_index_sequence<sizeof...(Args)>{});
            };
        }
        template<typename Class, typename Ret, typename... Args>
        void GenerateInvokeHelper(MethodInfo& outMethod, Ret(Class::*Func)(Args...) const) {
            outMethod.invoke = [](void* inst, void** args, void* ret) {
                InvokeThunkImpl<const Class, Ret, Args...>(static_cast<const Class*>(inst), Func, args, ret, std::make_index_sequence<sizeof...(Args)>{});
            };
            outMethod.flags = outMethod.flags | FunctionFlags::Const;
        }

        friend struct FieldInjector_Impl<T, void>; // Partial friend (simplified)
        template<typename U, typename D> friend struct FieldInjector_Impl;
        template<typename U, typename D> friend struct MethodInjector_Impl;
    };

    template<typename T, typename DSLType>
    struct FieldInjector_Impl {
        TypeBuilder<T>& builder;
        DSLType dsl;
        bool moved = false;
        
        constexpr FieldInjector_Impl(TypeBuilder<T>& b, DSLType d) : builder(b), dsl(d) {}
        

        ~FieldInjector_Impl() { 
            if (!moved) {
                 builder.template RegisterFieldImpl<typename DSLType::ClassType, typename DSLType::MemberType, DSLType::MemberPtr>(dsl.desc);
            }
        }
        constexpr FieldInjector_Impl(FieldInjector_Impl&& other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }
        
        // Fix: Explicitly specify return type instead of auto to help compiler deduction, 
        // OR simpler: make Chain a friend or public helper.
        // The issue 'NewDSL=void' suggests that dsl.EditAnywhere() might be returning void or failing SFINAE.
        // But we changed DSL methods to return FieldDSLNode<...>.
        // Let's verify FieldDSLNode methods.
        
        // Wait, C7595 error on 'Hash' (immediate function) means we are calling it with runtime arguments.
        // "Meta(Hash(key), value)" inside Meta() template.
        // 'key' is std::string_view parameter (runtime), passed to Hash (consteval).
        // FIX: We cannot pass runtime string_view to consteval Hash.
        // We need to propagate the constant-ness or use a runtime hash if the key is not constant.
        // BUT, our DSL usage is .Meta("Key", val). "Key" is constant.
        // However, the function `Meta(std::string_view key, ...)` takes it as a runtime parameter.
        // We need `template<size_t N> Meta(const char (&key)[N], ...)` to capture it as compile-time.
        
        constexpr auto EditAnywhere() { return Chain(dsl.EditAnywhere()); }
        constexpr auto ReadOnly() { return Chain(dsl.ReadOnly()); }
        constexpr auto ScriptReadWrite() { return Chain(dsl.ScriptReadWrite()); }
        template<typename U> constexpr auto UI(U&& schema) { return Chain(dsl.UI(std::forward<U>(schema))); }
        
        // Fix Meta for string literals
        // We capture the string literal as a reference to an array.
        // We pass it to Hash in a way that is constant expression.
        // The issue is that std::forward<V>(val) might not be constant expression if V is not literal type or reference.
        // But Hash(key) MUST be constant. 
        // "key" is a function parameter, so it is NOT constant expression inside the function body.
        // UNLESS we use `consteval`. But `FieldInjector_Impl` methods are not `consteval` (they are called from runtime code).
        
        // Solution: We cannot call `consteval Hash` with a function parameter `key`, even if that parameter is a template param.
        // Wait, if `key` is a non-type template parameter, it would work. But we can't pass string literals as NTTP easily in C++20 (yet, partial support).
        
        // Alternative: Use a macro or a helper struct.
        // Or, make `Hash` constexpr instead of consteval, so it can run at runtime.
        // If we make Hash constexpr, then dsl.Meta(Hash(key), val) will be evaluated at runtime (creating a new DSL node).
        // Is that okay?
        // FieldDSLNode::Meta returns a new type `FieldDSLNode<..., N+1>`.
        // The `desc` member of the new node is populated.
        // `desc.metadata[N] = { key, ... }`.
        // If `key` (TypeId) is calculated at runtime, `desc` is no longer a compile-time constant.
        // But `FieldDSLNode` constructors are `constexpr`. They allow runtime evaluation.
        // So `constexpr Hash` is the solution.
        
        template<size_t N, typename V> 
        constexpr auto Meta(const char (&key)[N], V&& val) { 
            // Call constexpr Hash (we need to change Hash to constexpr)
            return Chain(dsl.Meta(Hash(key), std::forward<V>(val))); 
        }
        
        // Fallback for runtime strings
        template<typename V> 
        constexpr auto Meta(std::string_view key, V&& val) { 
            return Chain(dsl.Meta(Hash(key), std::forward<V>(val))); 
        }
        
        template<typename V> constexpr auto Range(V min, V max) { return Chain(dsl.Range(min, max)); }
        
    private:
        template<typename NewDSL> 
        constexpr auto Chain(NewDSL newDSL) { 
            moved = true; 
            return FieldInjector_Impl<T, NewDSL>(builder, newDSL); 
        }
    };

    template<typename T, typename DSLType>
    struct MethodInjector_Impl {
        TypeBuilder<T>& builder;
        DSLType dsl;
        bool moved = false;
        MethodInjector_Impl(TypeBuilder<T>& b, DSLType d) : builder(b), dsl(d) {}
        
        // Fix: Call RegisterMethodImpl directly to avoid recursion
        ~MethodInjector_Impl() { 
            if (!moved) {
                builder.template RegisterMethodImpl<typename DSLType::ClassType, DSLType::MethodPtr, typename DSLType::ReturnType, typename DSLType::ArgsTuple>(dsl.desc);
            }
        }
        MethodInjector_Impl(MethodInjector_Impl&& other) : builder(other.builder), dsl(other.dsl) { other.moved = true; }
        auto ScriptCallable() { return Chain(dsl.ScriptCallable()); }
        auto EditorCallable() { return Chain(dsl.EditorCallable()); }
        template<typename V> auto Meta(std::string_view key, V&& val) { return Chain(dsl.Meta(Hash(key), std::forward<V>(val))); }
    private:
        template<typename NewDSL> auto Chain(NewDSL newDSL) { moved = true; return MethodInjector_Impl<T, NewDSL>(builder, newDSL); }
    };

    template<typename T>
    template<typename DSLType>
    auto TypeBuilder<T>::RegisterFieldFromDSL(const DSLType& dsl) {
        return FieldInjector_Impl<T, DSLType>(*this, dsl);
    }

    template<typename T>
    template<typename DSLType>
    auto TypeBuilder<T>::RegisterMethodFromDSL(const DSLType& dsl) {
        return MethodInjector_Impl<T, DSLType>(*this, dsl);
    }
}

// -----------------------------------------------------------------------------
// 7. Macros
// -----------------------------------------------------------------------------

template<typename Builder> using BuilderType = typename std::remove_reference_t<Builder>::ObjectType;

#define REFLECTION_REGISTER(Type) \
    inline const auto Type##_Reg = []() { \
        Shine::Reflection::TypeBuilder<Type> builder(#Type); \
        Type::RegisterReflection(builder); \
        builder.Register(); \
        return true; \
    }();

#define REFLECT_STRUCT(Type) \
    friend class Shine::Reflection::TypeBuilder<Type>; \
    template<typename Builder> \
    static void RegisterReflection(Builder& builder)

// Use typename std::remove_reference_t<Builder>::ObjectType to get T from Builder&
// Note: MSVC has trouble with decltype(builder) inside macros when template parameters are involved.
// We should rely on TypeBuilder<T>::ObjectType or similar if possible.
// But Builder is generic. We added ObjectType alias to BuilderType helper.

#define REFLECT_FIELD(Member) \
    builder.RegisterFieldFromDSL( \
        Shine::Reflection::DSL::FieldDSLNode< \
            typename std::remove_reference_t<decltype(builder)>::ObjectType, \
            decltype(std::declval<typename std::remove_reference_t<decltype(builder)>::ObjectType>().Member), \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member, \
            0 \
        >(#Member, offsetof(typename std::remove_reference_t<decltype(builder)>::ObjectType, Member)) \
    )

#define REFLECT_METHOD(Member) \
    builder.RegisterMethodFromDSL( \
        Shine::Reflection::DSL::MakeMethodDSL< \
            &std::remove_reference_t<decltype(builder)>::ObjectType::Member \
        >(#Member) \
    )

// -----------------------------------------------------------------------------
// 8. Codegen Example (Compile-Time Template)
// -----------------------------------------------------------------------------
namespace Shine::Reflection::Codegen {
    
    // Example: Zero-String Binding Generator
    // Usage: GlueGenerator<MakeMethodDSL<&MyType::Func>("Func")>::Generate(outCode);
    template<auto MethodNode>
    struct GlueGenerator {
        static void Generate(std::string& outCode) {
            constexpr auto& desc = MethodNode.desc;
            outCode += "// Binding for: "; outCode += desc.name; outCode += "\n";
            outCode += "int glue_"; outCode += desc.name; outCode += "(void* ctx) {\n";
            
            // 1. Check Arg Count
            outCode += "  if (GetArgCount(ctx) != "; outCode += std::to_string(desc.paramTypes.size()); outCode += ") return Error;\n";

            // 2. Fetch Args (Type Safe)
            for(size_t i=0; i<desc.paramTypes.size(); ++i) {
                outCode += "  auto arg"; outCode += std::to_string(i); 
                outCode += " = GetArg<"; 
                // In real codegen, we would map TypeId to string name here
                outCode += std::to_string(desc.paramTypes[i]); 
                outCode += ">(ctx, "; outCode += std::to_string(i); outCode += ");\n";
            }
            
            // 3. Call (Using Index, assuming we know the index at generation time)
            // If generation happens offline, we can compute the index.
            outCode += "  // CallMethod(instance, MethodIndex, args...)\n";
            outCode += "  return 0;\n";
            outCode += "}\n";
        }
    };
}
