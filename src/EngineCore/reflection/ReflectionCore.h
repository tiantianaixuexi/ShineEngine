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

#include "util/EnumFlags.h"

// =============================================================================
// Type Identity — hash, names, IDs
// =============================================================================

namespace shine::reflection {

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
    std::string_view sig = __FUNCSIG__;
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
// Type-erased function-pointer signatures
// =============================================================================

using GetterFn   = void(*)(const void* instance, void* out_value, std::size_t offset, std::size_t size);
using SetterFn   = void(*)(void* instance, const void* in_value, std::size_t offset, std::size_t size);
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
};

// =============================================================================
// Forward declaration
// =============================================================================

struct TypeInfo;

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

    GetterFn   getterFn   = nullptr;
    SetterFn   setterFn   = nullptr;
    OnChangeFn onChangeFn = nullptr;
    EqualsFn   equalsFn   = nullptr;
    CopyFn     copyFn     = nullptr;
    InvokeFn   invokeFn   = nullptr;

    const void*       containerTrait = nullptr;   // → SequenceTrait or MapTrait
    PropertyFlags     flags    = PropertyFlags::None;
    UI::Schema        uiSchema = UI::None{};
    std::string_view  name;
    MetadataContainer metadata;

    // ---- Accessors ----------------------------------------------------------

    void Get(const void* inst, void* out)       const { if (getterFn) getterFn(inst, out, offset, size); }
    void Set(void* inst, const void* in)        const { if (setterFn) setterFn(inst, in, offset, size); }
    void OnChange(void* inst, const void* old)  const { if (onChangeFn) onChangeFn(inst, old); }
    bool Equals(const void* a, const void* b)   const { return equalsFn ? equalsFn(a, b, size) : false; }
    void Copy(void* dst, const void* src)       const { if (copyFn) copyFn(dst, src, size); }

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
    InvokeFn            invokeFn      = nullptr;
    TypeId              returnType    = 0;
    std::vector<TypeId> paramTypes;
    uint64_t            signatureHash = 0;
    FunctionFlags       flags         = FunctionFlags::None;
    MetadataContainer   metadata;
    const TypeInfo*     owner         = nullptr;

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

struct TypeInfo {
    TypeId              id        = 0;
    std::string_view    name;
    std::size_t         size      = 0;
    std::size_t         alignment = 0;
    bool                isEnum    = false;
    bool                isPod     = false;

    std::vector<FieldInfo>  fields;
    std::vector<MethodInfo> methods;

    struct EnumEntry { int64_t value; std::string_view name; };
    std::vector<EnumEntry> enumEntries;

    const FieldInfo* FindField(std::string_view fieldName) const {
        for (const auto& f : fields)
            if (f.name == fieldName) return &f;
        return nullptr;
    }

    const MethodInfo* FindMethod(std::string_view methodName) const {
        for (const auto& m : methods)
            if (m.name == methodName) return &m;
        return nullptr;
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

} // namespace shine::reflection
