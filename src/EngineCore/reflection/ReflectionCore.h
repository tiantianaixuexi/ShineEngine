#pragma once

// =============================================================================
// ReflectionCore.h — Core type definitions for the Shine reflection system
// =============================================================================
//
// Consolidates: TypeId / Hash / Flags / UI schema / FieldInfo / MethodInfo /
//               TypeInfo / container traits — all in one header.
//
// C++23 / MSVC
// =============================================================================

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>
#include <source_location>
#include "util/EnumFlags.h"
#include "constexpr/constexpr_vector.h"
namespace shine::reflection {

// =============================================================================
// MemberPtrInfo — extract class + member type from member-pointer
// =============================================================================

namespace detail {
// Helper: extract class + member type from member-pointer
template <typename T> struct MemberPtrInfo;
template <typename C, typename M>
struct MemberPtrInfo<M C::*> { using ClassType = C; using MemberType = M; };
} // namespace detail

// =============================================================================
// Type Identity — hash, names, IDs
// =============================================================================

using TypeId = uint32_t;

/// FNV-1a hash — works at both compile time and runtime.
constexpr TypeId Hash(std::string_view str) noexcept {
    uint32_t h = 2166136261u;
    for (char c : str) {
        h ^= static_cast<uint8_t>(c);
        h *= 16777619u;
    }
    return h;
}

/// Compile-time overload for string literals.
template <std::size_t N>
consteval TypeId Hash(const char (&str)[N]) noexcept {
    uint32_t h = 2166136261u;
    for (std::size_t i = 0; i + 1 < N; ++i) {
        h ^= static_cast<uint8_t>(str[i]);
        h *= 16777619u;
    }
    return h;
}

/// Extract a clean type name from __FUNCSIG__ (MSVC) at compile time.
template <typename T>
consteval std::string_view GetTypeName() noexcept {
    constexpr std::source_location loc = std::source_location::current();
    std::string_view sig = loc.function_name();
    auto start = sig.find("GetTypeName<") + 12;
    auto end   = sig.find_last_of('>');
    auto name  = sig.substr(start, end - start);
    if (name.starts_with("struct ")) return name.substr(7);
    if (name.starts_with("class "))  return name.substr(6);
    if (name.starts_with("enum "))   return name.substr(5);
    return name;
}

/// Produce a unique TypeId for type T at compile time.
template <typename T>
consteval TypeId GetTypeId() noexcept {
    return Hash(GetTypeName<T>());
}

/// User-defined literal for compile-time hashing: "Category"_hash
consteval TypeId operator""_hash(const char* str, std::size_t len) noexcept {
    uint32_t h = 2166136261u;
    for (std::size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint8_t>(str[i]);
        h *= 16777619u;
    }
    return h;
}

/// Pre-computed metadata key constants — guaranteed compile-time evaluation.
namespace MetaKeys {
    inline constexpr TypeId Category          = "Category"_hash;
    inline constexpr TypeId DisplayName       = "DisplayName"_hash;
    inline constexpr TypeId Min               = "Min"_hash;
    inline constexpr TypeId Max               = "Max"_hash;
    inline constexpr TypeId EditCondition     = "EditCondition"_hash;
    inline constexpr TypeId BlueprintFunction = "BlueprintFunction"_hash;
} // namespace MetaKeys

} // namespace shine::reflection  — closed for ENABLE_ENUM_FLAGS macros

// =============================================================================
// Flags — defined at file scope so ENABLE_ENUM_FLAGS works without hassle
// =============================================================================

enum class PropertyFlags : uint64_t {
    None            = 0,
    EditAnywhere    = 1 << 0,
    ReadOnly        = 1 << 1,
    Transient       = 1 << 2,
    ScriptRead      = 1 << 3,
    ScriptWrite     = 1 << 4,
    ScriptReadWrite = (1 << 3) | (1 << 4),
    SaveGame        = 1 << 5,
};

enum class FunctionFlags : uint64_t {
    None           = 0,
    ScriptCallable = 1 << 0,
    EditorCallable = 1 << 1,
    Const          = 1 << 2,
    Static         = 1 << 3,
};

enum class ContainerType : uint8_t {
    None,
    Sequence,
    Associative,
};

ENABLE_ENUM_FLAGS(PropertyFlags)
ENABLE_ENUM_FLAGS(FunctionFlags)

// =============================================================================
// Everything else lives inside the namespace
// =============================================================================

namespace shine::reflection {

using PropertyFlags = ::PropertyFlags;
using FunctionFlags = ::FunctionFlags;
using ContainerType = ::ContainerType;


// =============================================================================
// UI Schema types — for editor integration via std::visit
// =============================================================================

namespace UI {

struct None {};
struct TextInput       { std::size_t max_length = 0; bool multiline = false; };
struct NumberInput     { double min_value = 0.0; double max_value = 100.0; double step = 1.0; };
struct Slider          { float min = 0.f; float max = 100.f; float step = 1.f; };
struct Dropdown        {};
struct Checkbox        {};
struct ColorPicker     {};
struct FilePicker      { std::string_view filter; bool allow_multiple = false; };
struct FunctionSelector{ bool onlyScriptCallable = true; };
struct VectorEditor    { std::size_t dimensions = 3; double min_value = -1000.0; double max_value = 1000.0; };
struct MatrixEditor    { std::size_t rows = 4; std::size_t cols = 4; };

// Backward-compatible aliases
using InputText = TextInput;
using Color     = ColorPicker;

using Schema = std::variant<
    None, TextInput, NumberInput, Slider, Dropdown,
    Checkbox, ColorPicker, FilePicker, FunctionSelector,
    VectorEditor, MatrixEditor
>;

} // namespace UI

// =============================================================================
// Metadata
// =============================================================================

using MetadataKey       = TypeId;
using MetadataValue     = std::variant<std::monostate, bool, int, float, double, std::string_view>;
using MetadataContainer = std::vector<std::pair<MetadataKey, MetadataValue>>;

// =============================================================================
// Type-erased function-pointer signatures (optimized)
// =============================================================================

using GetterFn   = void(*)(const void* inst, void* out);
using SetterFn   = void(*)(void* inst, const void* in);
using OnChangeFn = void(*)(void* instance, const void* oldValue);
using EqualsFn   = bool(*)(const void* a, const void* b, std::size_t size);
using CopyFn     = void(*)(void* dst, const void* src, std::size_t size);
using InvokeFn   = void(*)(void* instance, void** args, void* ret);

// =============================================================================
// Runtime container traits (type-erased)
// =============================================================================

struct SequenceTrait {
    TypeId       elementType                                    = 0;
    std::size_t  (*GetSize)(const void*)                        = nullptr;
    void*        (*GetElement)(void*, std::size_t)              = nullptr;
    const void*  (*GetElementConst)(const void*, std::size_t)   = nullptr;
    void         (*Resize)(void*, std::size_t)                  = nullptr;
};

using ArrayTrait = SequenceTrait;   // backward-compat alias

struct MapTrait {
    TypeId       keyType                           = 0;
    TypeId       valueType                         = 0;
    std::size_t  (*GetSize)(const void*)           = nullptr;
    void         (*Clear)(void*)                   = nullptr;
    void         (*Iterate)(const void*, void*, void(*)(const void*, const void*, void*)) = nullptr;
    void         (*InsertKV)(void*, const void*, const void*) = nullptr;
};

// =============================================================================
// Forward declaration
// =============================================================================

struct TypeInfo;

// =============================================================================
// Compile-time field access - 完全模板化，编译期绑定
// =============================================================================

namespace detail {

// 编译期字段访问器 - 通过模板在编译时绑定
template <typename T, typename Owner, T Owner::*Member>
struct CompileTimeAccessor {
    static constexpr std::size_t offset = reinterpret_cast<std::uintptr_t>(
    &(reinterpret_cast<const Owner*>(0)->*Member)
);

    static void Get(const void* inst, void* out) {
        const auto& val = static_cast<const Owner*>(inst)->*Member;
        if constexpr (std::is_trivially_copyable_v<T>)
            std::memcpy(out, &val, sizeof(T));
        else
            *static_cast<T*>(out) = val;
    }

    static void Set(void* inst, const void* in) {
        auto& ref = static_cast<Owner*>(inst)->*Member;
        if constexpr (std::is_trivially_copyable_v<T>)
            std::memcpy(&ref, in, sizeof(T));
        else
            ref = *static_cast<const T*>(in);
    }
};

// 编译期访问器注册表 - 使用类型ID映射
template <typename T>
inline T* (*g_compileTimeGetter)(const void*, void*) = nullptr;
template <typename T>
inline void (*g_compileTimeSetter)(void*, const void*) = nullptr;

} // namespace detail

// 注册编译期访问器（模板特化版本）
template <typename T, typename Owner, T Owner::*Member>
void RegisterCompileTimeAccessor(uint32_t fieldId) {
   // using Accessor = detail::CompileTimeAccessor<T, Owner, Member>;
    // 这里可以存储到全局映射表中
    // 实际使用时通过模板分派
}

// =============================================================================
// FieldInfo — runtime field descriptor
// =============================================================================

struct FieldInfo {
    TypeId          typeId        = 0;
    ContainerType   containerType = ContainerType::None;
    std::size_t     offset        = 0;
    std::size_t     size          = 0;
    std::size_t     alignment     = 0;
    bool            isPod         = false;

    // Optimized accessors (零间接调用)
    GetterFn   getterFn   = nullptr;
    SetterFn   setterFn   = nullptr;
    OnChangeFn onChangeFn = nullptr;
    EqualsFn   equalsFn   = nullptr;
    CopyFn     copyFn     = nullptr;
    InvokeFn   invokeFn   = nullptr;

    // ---- Metadata -----------------------------------------------------------
    const void*       containerTrait = nullptr;
    PropertyFlags     flags    = PropertyFlags::None;
    UI::Schema        uiSchema = UI::None{};
    std::string_view  name;
    uint32_t          nameHash = 0;
    MetadataContainer metadata;

    // ---- Accessors ----------------------------------------------------------

    // Optimized accessor (零间接调用)
    void Get(const void* inst, void* out) const {
        if (getterFn) getterFn(inst, out);
    }
    void Set(void* inst, const void* in) const {
        if (setterFn) setterFn(inst, in);
    }

    // Compile-time inline accessor - 零开销！
    template <typename T, typename Owner, T Owner::*Member>
    void CTGet(const void* inst, void* out) const {
        const auto& val = static_cast<const Owner*>(inst)->*Member;
        if constexpr (std::is_trivially_copyable_v<T>)
            std::memcpy(out, &val, sizeof(T));
        else
            *static_cast<T*>(out) = val;
    }

    template <typename T, typename Owner, T Owner::*Member>
    void CTSet(void* inst, const void* in) const {
        auto& ref = static_cast<Owner*>(inst)->*Member;
        if constexpr (std::is_trivially_copyable_v<T>)
            std::memcpy(&ref, in, sizeof(T));
        else
            ref = *static_cast<const T*>(in);
    }

    void OnChange(void* inst, const void* old)  const { if (onChangeFn) onChangeFn(inst, old); }
    bool Equals(const void* a, const void* b)   const { return equalsFn ? equalsFn(a, b, size) : false; }
    void Copy(void* dst, const void* src)       const { if (copyFn) copyFn(dst, src, size); }

    // ---- Compile-time inline accessor (零开销，完全内联) ------------------------

    // 编译期字段访问器 - 通过模板参数完全确定，零间接调用
    template <typename C, typename M, M C::*Ptr>
    struct FieldAccessor {
        using ClassType = C;
        using MemberType = M;
        static constexpr std::size_t offset = reinterpret_cast<std::uintptr_t>(
    &(reinterpret_cast<const C*>(0)->*Ptr)
);

        [[gnu::always_inline]] static M Get(const C& obj) { return obj.*Ptr; }
        [[gnu::always_inline]] static void Set(C& obj, const M& val) { obj.*Ptr = val; }
    };

    // 编译期偏移获取
    template <auto MemberPtr>
    inline constexpr std::size_t CT_OFFSETOF() {
        using Info = detail::MemberPtrInfo<decltype(MemberPtr)>;
        return reinterpret_cast<std::uintptr_t>(
    &(reinterpret_cast<const typename Info::ClassType*>(0)->*MemberPtr)
);
    }

    // ---- Metadata -----------------------------------------------------------

    const MetadataValue* GetMeta(MetadataKey key) const {
        for (const auto& [k, v] : metadata)
            if (k == key) return &v;
        return nullptr;
    }

    bool HasMeta(MetadataKey key) const { return GetMeta(key) != nullptr; }
};

// =============================================================================
// MethodInfo — runtime method descriptor
// =============================================================================

struct MethodInfo {
    std::string_view    name;
    uint32_t            nameHash      = 0;
    InvokeFn            invokeFn      = nullptr;
    TypeId              returnType    = 0;
    std::vector<TypeId> paramTypes;
    uint64_t            signatureHash = 0;
    FunctionFlags       flags         = FunctionFlags::None;
    MetadataContainer   metadata;
    const TypeInfo*     owner         = nullptr;
    mutable const TypeInfo*               cachedReturnTypeInfo = nullptr;
    mutable std::vector<const TypeInfo*>  cachedParamTypeInfos;
    mutable std::vector<std::size_t>      cachedParamOffsets;
    mutable std::size_t                   cachedReturnOffset = 0;
    mutable std::size_t                   cachedFrameSize = 0;
    mutable std::size_t                   cachedFrameAlignment = alignof(std::max_align_t);
    mutable bool                          callCacheValid = false;

    void Invoke(void* inst, void** args, void* ret) const {
        if (invokeFn) invokeFn(inst, args, ret);
    }

    const MetadataValue* GetMeta(MetadataKey key) const {
        for (const auto& [k, v] : metadata)
            if (k == key) return &v;
        return nullptr;
    }
};

// =============================================================================
// TypeInfo — complete runtime type descriptor
// =============================================================================

struct EnumEntry {
    int64_t value = 0;
    std::string_view name;
};

struct TypeInfo {
    TypeId              id        = 0;
    std::string_view    name;
    std::size_t         size      = 0;
    std::size_t         alignment = 0;
    bool                isEnum    = false;
    bool                isPod     = false;  // 是否可平凡复制类型

    std::vector<FieldInfo>  fields;
    std::vector<MethodInfo> methods;
    std::vector<EnumEntry>  enumEntries;

    // Fast lookup sorted indices for performance
    struct LookupEntry { uint32_t hash; uint32_t index; };
    std::vector<LookupEntry> fieldLookup_;
    std::vector<LookupEntry> methodLookup_;
    bool lookupSorted_ = false;

    const FieldInfo* FindField(std::string_view fieldName) const {
        uint32_t hash = Hash(fieldName);
        auto it = std::ranges::lower_bound(fieldLookup_, hash, {}, &LookupEntry::hash);

        // Handle collisions and multiple matches (rare)
        while (it != fieldLookup_.end() && it->hash == hash) {
            const auto& f = fields[it->index];
            if (f.name == fieldName) return &f;
            ++it;
        }
        return nullptr;
    }

    const MethodInfo* FindMethod(std::string_view methodName) const {
        uint32_t hash = Hash(methodName);
        auto it = std::ranges::lower_bound(methodLookup_, hash, {}, &LookupEntry::hash);

        while (it != methodLookup_.end() && it->hash == hash) {
            const auto& m = methods[it->index];
            if (m.name == methodName) return &m;
            ++it;
        }
        return nullptr;
    }

public:
    void BuildLookup() {
        if (lookupSorted_) return;

        fieldLookup_.clear();
        fieldLookup_.reserve(fields.size());
        for (uint32_t i = 0; i < (uint32_t)fields.size(); ++i) {
            fieldLookup_.push_back({Hash(fields[i].name), i});
        }
        std::ranges::sort(fieldLookup_, {}, &LookupEntry::hash);

        methodLookup_.clear();
        methodLookup_.reserve(methods.size());
        for (uint32_t i = 0; i < (uint32_t)methods.size(); ++i) {
            methodLookup_.push_back({Hash(methods[i].name), i});
        }
        std::ranges::sort(methodLookup_, {}, &LookupEntry::hash);

        lookupSorted_ = true;
    }

    std::size_t GetFieldCount()  const { return fields.size(); }
    std::size_t GetMethodCount() const { return methods.size(); }
};

// =============================================================================
// ListThunks — builds a runtime SequenceTrait for any sequence container
// =============================================================================

template <typename List>
struct ListThunks {
    static std::size_t GetSize(const void* p) {
        return static_cast<const List*>(p)->size();
    }
    static void* GetElement(void* p, std::size_t i) {
        auto it = static_cast<List*>(p)->begin();
        std::advance(it, i);
        return &(*it);
    }
    static const void* GetElementConst(const void* p, std::size_t i) {
        auto it = static_cast<const List*>(p)->begin();
        std::advance(it, i);
        return &(*it);
    }
    static void Resize(void* p, std::size_t n) {
        static_cast<List*>(p)->resize(n);
    }

    static const SequenceTrait& GetTrait() {
        static const SequenceTrait trait{
            GetTypeId<typename List::value_type>(),
            &GetSize, &GetElement, &GetElementConst, &Resize
        };
        return trait;
    }
};

// =============================================================================
// MapThunks — builds a runtime MapTrait for std::map/std::unordered_map
// =============================================================================

template <typename Map>
struct MapThunks {
    using KeyType = typename Map::key_type;
    using ValueType = typename Map::mapped_type;

    static std::size_t GetSize(const void* p) {
        return static_cast<const Map*>(p)->size();
    }

    static void Clear(void* p) {
        static_cast<Map*>(p)->clear();
    }

    static void* Find(void* p, const void* key) {
        auto it = static_cast<Map*>(p)->find(*static_cast<const KeyType*>(key));
        if (it != static_cast<Map*>(p)->end()) {
            return &it->second;
        }
        return nullptr;
    }

    static const void* FindConst(const void* p, const void* key) {
        auto it = static_cast<const Map*>(p)->find(*static_cast<const KeyType*>(key));
        if (it != static_cast<const Map*>(p)->end()) {
            return &it->second;
        }
        return nullptr;
    }

    static void Insert(void* p, const void* key, const void* value) {
        (*static_cast<Map*>(p))[*static_cast<const KeyType*>(key)] = *static_cast<const ValueType*>(value);
    }

    static void Erase(void* p, const void* key) {
        static_cast<Map*>(p)->erase(*static_cast<const KeyType*>(key));
    }

    static bool Contains(const void* p, const void* key) {
        return static_cast<const Map*>(p)->contains(*static_cast<const KeyType*>(key));
    }

    static void Iterate(const void* p, void* userData, void(*cb)(const void*, const void*, void*)) {
        for (const auto& [k, v] : *static_cast<const Map*>(p)) {
            cb(&k, &v, userData);
        }
    }

    static void InsertKV(void* p, const void* k, const void* v) {
        (*static_cast<Map*>(p))[*static_cast<const KeyType*>(k)] = *static_cast<const ValueType*>(v);
    }

    static const MapTrait& GetTrait() {
        static const MapTrait trait{
            GetTypeId<KeyType>(),
            GetTypeId<ValueType>(),
            &GetSize,
            &Clear,
            &Iterate,
            &InsertKV
        };
        return trait;
    }
};

// =============================================================================
// SetThunks — builds a runtime MapTrait for std::set/std::unordered_set
// =============================================================================

template <typename Set>
struct SetThunks {
    using ValueType = typename Set::value_type;

    static std::size_t GetSize(const void* p) {
        return static_cast<const Set*>(p)->size();
    }

    static void Clear(void* p) {
        static_cast<Set*>(p)->clear();
    }

    static bool Contains(const void* p, const void* value) {
        return static_cast<const Set*>(p)->contains(*static_cast<const ValueType*>(value));
    }

    static void Insert(void* p, const void* value) {
        static_cast<Set*>(p)->insert(*static_cast<const ValueType*>(value));
    }

    static void Erase(void* p, const void* value) {
        static_cast<Set*>(p)->erase(*static_cast<const ValueType*>(value));
    }

    static void Iterate(const void* p, void* userData, void(*cb)(const void*, const void*, void*)) {
        for (const auto& k : *static_cast<const Set*>(p)) {
            cb(&k, &k, userData);
        }
    }

    static void InsertKV(void* p, const void*, const void* v) {
        static_cast<Set*>(p)->insert(*static_cast<const ValueType*>(v));
    }

    // Set 使用与 Map 相同的 trait 结构，但 keyType == valueType
    static const MapTrait& GetTrait() {
        static const MapTrait trait{
            GetTypeId<ValueType>(),
            GetTypeId<ValueType>(),
            &GetSize,
            &Clear,
            &Iterate,
            &InsertKV
        };
        return trait;
    }
};

// =============================================================================
// Compile-Time Reflection Structures
// =============================================================================

// 编译期字段信息
struct ConstexprFieldInfo {
    const char* name;
    uint32_t typeId;
    size_t offset;
    size_t size;
    size_t alignment;
    bool isPod;

    // 编译期字段构建助手
    template<typename T>
    consteval static ConstexprFieldInfo Create(const char* field_name) {
        return ConstexprFieldInfo{
            field_name,
            GetTypeId<T>(),
            0,  // offset需要运行时确定
            sizeof(T),
            alignof(T),
            std::is_trivially_copyable_v<T>
        };
    }
};

// 编译期类型信息模板
template<typename T>
struct ConstexprTypeInfo {
    uint32_t id;
    const char* name;
    size_t size;
    size_t alignment;
    bool isEnum;
    bool isPod;

    // 使用原始constexpr容器
    shine::constexpr_::constexpr_vector<ConstexprFieldInfo, 16> fields;

    // 编译期类型信息构建
    consteval static ConstexprTypeInfo Create(const char* type_name = "unknown") {
        return ConstexprTypeInfo{
            GetTypeId<T>(),
            type_name,
            sizeof(T),
            alignof(T),
            std::is_enum_v<T>,
            std::is_trivially_copyable_v<T>
        };
    }

    // 添加字段
    template<typename FieldType>
    consteval ConstexprTypeInfo& AddField(const char* field_name) {
        if (!fields.full()) {
            fields.push_back(ConstexprFieldInfo::Create<FieldType>(field_name));
        }
        return *this;
    }

    // 编译期字段查找（使用简单的字符串比较）
    constexpr const ConstexprFieldInfo* FindField(const char* field_name) const {
        for (size_t i = 0; i < fields.size(); ++i) {
            // 简单的字符串比较，避免使用 strcmp
            const char* a = fields[i].name;
            const char* b = field_name;
            while (*a && *b && *a == *b) {
                ++a; ++b;
            }
            if (*a == *b) {  // both are null terminator
                return &fields[i];
            }
        }
        return nullptr;
    }

    // 获取字段数量
    constexpr size_t GetFieldCount() const {
        return fields.size();
    }
};

// =============================================================================
// Automatic Compile-Time Registration System
// =============================================================================

// 自动编译期注册器 - 将编译期信息自动注册到运行时系统
template<typename T>
struct AutoConstexprRegistration {
    static consteval auto Register() {
        // 编译期构建类型信息
        auto ct_info = ConstexprTypeInfo<T>::Create();

        // 返回编译期信息（运行时注册将在其他地方处理）
        return ct_info;
    }
};

// =============================================================================
// Compile-time field access utilities (namespace-level, zero-overhead)
// =============================================================================

// CT_GET - 编译期绑定的字段 getter，完全内联零开销
// 用法: auto value = CT_GET<&Player::age>(player);
template <auto MemberPtr, typename C = typename detail::MemberPtrInfo<decltype(MemberPtr)>::ClassType,
          typename M = typename detail::MemberPtrInfo<decltype(MemberPtr)>::MemberType>
[[gnu::always_inline]] inline M CT_GET(const C& obj) {
    return obj.*MemberPtr;
}

// CT_SET - 编译期绑定的字段 setter，完全内联零开销
// 用法: CT_SET<&Player::age>(player, 42);
template <auto MemberPtr, typename C = typename detail::MemberPtrInfo<decltype(MemberPtr)>::ClassType,
          typename M = typename detail::MemberPtrInfo<decltype(MemberPtr)>::MemberType>
[[gnu::always_inline]] inline void CT_SET(C& obj, const M& val) {
    obj.*MemberPtr = val;
}

} // namespace shine::reflection
