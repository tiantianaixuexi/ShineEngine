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
#include <concepts>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <source_location>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "Core/ConstexprOffset.h"
#include "Core/ReflectionOwnerHandle.h"
#include "Memory/ReflectionMemory.h"
#include "memory/memory.ixx"
#include "string/shine_string.h"
#include "string/shine_text_view.h"
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

template <typename T> struct MemberFunctionPtrInfo;
template <typename C, typename R, typename... Args>
struct MemberFunctionPtrInfo<R (C::*)(Args...)> {
    using ClassType = C;
    using ReturnType = R;
};
template <typename C, typename R, typename... Args>
struct MemberFunctionPtrInfo<R (C::*)(Args...) const> {
    using ClassType = C;
    using ReturnType = R;
};
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

constexpr TypeId Hash(shine::STextView str) noexcept {
    return Hash(str.sv());
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
consteval shine::STextView GetTypeName() noexcept {
    constexpr std::source_location loc = std::source_location::current();
    shine::STextView sig = shine::STextView::from_cstring(loc.function_name());
    auto start = sig.find("GetTypeName<") + 12;
    auto end   = sig.find_last_of('>');
    auto name  = sig.substr(start, end - start);
    if (name.starts_with("struct ")) return name.substr(7);
    if (name.starts_with("class "))  return name.substr(6);
    if (name.starts_with("enum "))   return name.substr(5);
    return name;
}

/// Produce a per-build runtime lookup TypeId for type T at compile time.
///
/// This ID is reproducible within the current build, but it is derived from
/// compiler-specific type-name strings and is therefore not guaranteed to stay
/// stable across compilers, toolchain versions, or persisted data.
///
/// Use this only for runtime registration and lookup. Do not treat it as a
/// long-term serialization, asset, or hot-reload identity.
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
struct FilePicker      { shine::STextView filter; bool allow_multiple = false; };
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
using MetadataValue     = std::variant<std::monostate, bool, int, float, double, shine::STextView,shine::SString>;
using DSLMetadataStorage = std::vector<std::pair<MetadataKey, MetadataValue>>;
using ReflectionMetadataStorage = ReflectionColdVector<std::pair<MetadataKey, MetadataValue>>;

inline MetadataValue MakeMetadataValue(shine::STextView value) {
    return MetadataValue{value};
}

inline MetadataValue MakeMetadataValue(shine::SString value) {
    return MetadataValue{std::move(value)};
}

inline MetadataValue MakeMetadataValue(std::string_view value) {
    return MetadataValue{shine::SString(value)};
}

inline MetadataValue MakeMetadataValue(const char* value) {
    return MetadataValue{shine::SString(value != nullptr ? value : "")};
}

template <std::size_t N>
inline MetadataValue MakeMetadataValue(const char (&value)[N]) {
    return MetadataValue{shine::STextView::from_literal(value)};
}

template <typename T>
    requires std::constructible_from<MetadataValue, T&&>
inline MetadataValue MakeMetadataValue(T&& value) {
    return MetadataValue{std::forward<T>(value)};
}

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
template <typename T, typename Limits>
struct TypeBuilder;

struct MethodCallParamStorage {
    static constexpr std::size_t kInlineParamCount = 2;

    std::array<const TypeInfo*, kInlineParamCount> inlineTypeInfos{};
    std::array<std::size_t, kInlineParamCount> inlineOffsets{};
    std::byte* overflowStorage = nullptr;
    std::size_t paramCount = 0;
    std::size_t overflowCapacity = 0;

    MethodCallParamStorage() = default;
    ~MethodCallParamStorage() {
        ReleaseOverflow();
    }

    MethodCallParamStorage(const MethodCallParamStorage&) = delete;
    MethodCallParamStorage& operator=(const MethodCallParamStorage&) = delete;
    MethodCallParamStorage(MethodCallParamStorage&&) = delete;
    MethodCallParamStorage& operator=(MethodCallParamStorage&&) = delete;

    void Clear() noexcept {
        paramCount = 0;
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        return paramCount;
    }

    [[nodiscard]] bool UsesOverflow() const noexcept {
        return overflowStorage != nullptr;
    }

    void Reserve(std::size_t requiredCount) {
        const std::size_t requiredOverflow = OverflowCount(requiredCount);
        if (requiredOverflow <= overflowCapacity) {
            return;
        }

        const std::size_t alignedTypeBytes = AlignUp(requiredOverflow * sizeof(const TypeInfo*), alignof(std::size_t));
        const std::size_t totalBytes = alignedTypeBytes + (requiredOverflow * sizeof(std::size_t));

        shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionCold);
        auto* newStorage = static_cast<std::byte*>(shine::co::Memory::Alloc(totalBytes, alignof(std::max_align_t)));
        assert(newStorage && "MethodCallParamStorage::Reserve failed");
        if (!newStorage) {
            return;
        }

        const std::size_t existingOverflow = OverflowCount(paramCount);
        if (existingOverflow != 0 && overflowStorage != nullptr) {
            std::memcpy(newStorage, overflowStorage, alignedTypeBytesFor(overflowCapacity));
            std::memcpy(newStorage + alignedTypeBytes,
                overflowStorage + alignedTypeBytesFor(overflowCapacity),
                existingOverflow * sizeof(std::size_t));
        }

        if (overflowStorage != nullptr) {
            shine::co::Memory::Free(overflowStorage);
        }

        overflowStorage = newStorage;
        overflowCapacity = requiredOverflow;
    }

    void Push(const TypeInfo* typeInfo, std::size_t offset) {
        Reserve(paramCount + 1);
        if (paramCount < kInlineParamCount) {
            inlineTypeInfos[paramCount] = typeInfo;
            inlineOffsets[paramCount] = offset;
        } else {
            const std::size_t overflowIndex = paramCount - kInlineParamCount;
            OverflowTypeInfos()[overflowIndex] = typeInfo;
            OverflowOffsets()[overflowIndex] = offset;
        }
        ++paramCount;
    }

    [[nodiscard]] const TypeInfo* GetTypeInfo(std::size_t index) const noexcept {
        return index < kInlineParamCount ? inlineTypeInfos[index] : OverflowTypeInfos()[index - kInlineParamCount];
    }

    [[nodiscard]] std::size_t GetOffset(std::size_t index) const noexcept {
        return index < kInlineParamCount ? inlineOffsets[index] : OverflowOffsets()[index - kInlineParamCount];
    }

private:
    [[nodiscard]] static constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    [[nodiscard]] static constexpr std::size_t OverflowCount(std::size_t totalCount) noexcept {
        return totalCount > kInlineParamCount ? (totalCount - kInlineParamCount) : 0;
    }

    [[nodiscard]] static constexpr std::size_t alignedTypeBytesFor(std::size_t overflowCount) noexcept {
        return AlignUp(overflowCount * sizeof(const TypeInfo*), alignof(std::size_t));
    }

    [[nodiscard]] const TypeInfo** OverflowTypeInfos() noexcept {
        return reinterpret_cast<const TypeInfo**>(overflowStorage);
    }

    [[nodiscard]] const TypeInfo* const* OverflowTypeInfos() const noexcept {
        return reinterpret_cast<const TypeInfo* const*>(overflowStorage);
    }

    [[nodiscard]] std::size_t* OverflowOffsets() noexcept {
        return reinterpret_cast<std::size_t*>(overflowStorage + alignedTypeBytesFor(overflowCapacity));
    }

    [[nodiscard]] const std::size_t* OverflowOffsets() const noexcept {
        return reinterpret_cast<const std::size_t*>(overflowStorage + alignedTypeBytesFor(overflowCapacity));
    }

    void ReleaseOverflow() noexcept {
        if (overflowStorage != nullptr) {
            shine::co::Memory::Free(overflowStorage);
            overflowStorage = nullptr;
        }
        overflowCapacity = 0;
        paramCount = 0;
    }
};

struct MethodCallCache {
    const TypeInfo*       returnTypeInfo = nullptr;
    MethodCallParamStorage params;
    std::size_t           returnOffset = 0;
    std::size_t           frameSize = 0;
    std::size_t           frameAlignment = alignof(std::max_align_t);
    bool                  valid = false;

    void ClearParams() noexcept {
        params.Clear();
    }

    void ReserveParams(std::size_t count) {
        params.Reserve(count);
    }

    void AddParam(const TypeInfo* typeInfo, std::size_t offset) {
        params.Push(typeInfo, offset);
    }

    [[nodiscard]] std::size_t ParamCount() const noexcept {
        return params.Size();
    }

    [[nodiscard]] const TypeInfo* GetParamTypeInfo(std::size_t index) const noexcept {
        return params.GetTypeInfo(index);
    }

    [[nodiscard]] std::size_t GetParamOffset(std::size_t index) const noexcept {
        return params.GetOffset(index);
    }

    [[nodiscard]] bool UsesOverflowParamStorage() const noexcept {
        return params.UsesOverflow();
    }
};

struct FieldBuiltinMetadata {
    shine::STextView category;
    shine::STextView displayName;
    shine::STextView editCondition;
    float            minValue = 0.0f;
    float            maxValue = 0.0f;
    bool             hasRange = false;
};

struct FieldColdData {
    UI::Schema          uiSchema = UI::None{};
    shine::STextView    name;
    FieldBuiltinMetadata builtinMetadata;
    ReflectionMetadataStorage metadata;
};

struct MethodColdData {
    shine::STextView  name;
    ReflectionColdVector<TypeId> paramTypes;
    ReflectionMetadataStorage metadata;
};

struct EnumEntry {
    int64_t value = 0;
    shine::STextView name;
};

struct TypeColdData {
    shine::STextView name;
    ReflectionColdVector<EnumEntry> enumEntries;
};

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
    ReflectionOwnerHandle owner;
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
    uint32_t          nameHash = 0;

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

    [[nodiscard]] const MetadataValue* GetMeta(MetadataKey key) const {
        for (const auto& [k, v] : GetMetadata())
            if (k == key) return &v;
        return nullptr;
    }

    [[nodiscard]] bool HasMeta(MetadataKey key) const { return GetMeta(key) != nullptr; }

    void SetName(shine::STextView value) {
        EnsureColdData().name = InternReflectionText(value);
    }

    [[nodiscard]] shine::STextView GetNameView() const noexcept {
        return coldData ? coldData->name : shine::STextView{};
    }

    void SetUISchema(UI::Schema value) {
        EnsureColdData().uiSchema = std::move(value);
    }

    [[nodiscard]] const UI::Schema& GetUISchema() const noexcept {
        static const UI::Schema kDefaultSchema = UI::None{};
        return coldData ? coldData->uiSchema : kDefaultSchema;
    }

    [[nodiscard]] const ReflectionMetadataStorage& GetMetadata() const noexcept {
        static const ReflectionMetadataStorage kEmptyMetadata;
        return coldData ? coldData->metadata : kEmptyMetadata;
    }

    ReflectionMetadataStorage& MutableMetadata() {
        return EnsureColdData().metadata;
    }

    [[nodiscard]] const TypeInfo* GetOwnerType() const {
        return owner.AsType();
    }

    void SetRange(float min, float max) {
        auto& builtin = EnsureColdData().builtinMetadata;
        builtin.minValue = min;
        builtin.maxValue = max;
        builtin.hasRange = true;
    }

    void SetCategory(shine::STextView value) {
        EnsureColdData().builtinMetadata.category = InternReflectionText(value);
    }

    void SetDisplayName(shine::STextView value) {
        EnsureColdData().builtinMetadata.displayName = InternReflectionText(value);
    }

    void SetEditCondition(shine::STextView value) {
        EnsureColdData().builtinMetadata.editCondition = InternReflectionText(value);
    }

    [[nodiscard]] bool TrySetBuiltinMetadata(MetadataKey key, const MetadataValue& value) {
        if (key == MetaKeys::Category) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                SetCategory(*text);
                return true;
            }
            return false;
        }

        if (key == MetaKeys::DisplayName) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                SetDisplayName(*text);
                return true;
            }
            return false;
        }

        if (key == MetaKeys::EditCondition) {
            if (const auto* text = std::get_if<shine::STextView>(&value)) {
                SetEditCondition(*text);
                return true;
            }
            return false;
        }

        if (key == MetaKeys::Min || key == MetaKeys::Max) {
            float converted = 0.0f;
            if (const auto* asFloat = std::get_if<float>(&value)) {
                converted = *asFloat;
            } else if (const auto* asInt = std::get_if<int>(&value)) {
                converted = static_cast<float>(*asInt);
            } else if (const auto* asDouble = std::get_if<double>(&value)) {
                converted = static_cast<float>(*asDouble);
            } else {
                return false;
            }

            if (key == MetaKeys::Min) {
                EnsureColdData().builtinMetadata.minValue = converted;
            } else {
                EnsureColdData().builtinMetadata.maxValue = converted;
            }
            EnsureColdData().builtinMetadata.hasRange = true;
            return true;
        }

        return false;
    }

    [[nodiscard]] bool HasCategory() const noexcept {
        return coldData && !coldData->builtinMetadata.category.empty();
    }
    [[nodiscard]] bool HasDisplayName() const noexcept {
        return coldData && !coldData->builtinMetadata.displayName.empty();
    }
    [[nodiscard]] bool HasEditCondition() const noexcept {
        return coldData && !coldData->builtinMetadata.editCondition.empty();
    }
    [[nodiscard]] bool HasRange() const noexcept {
        return coldData && coldData->builtinMetadata.hasRange;
    }
    [[nodiscard]] shine::STextView GetCategoryView() const noexcept {
        return coldData ? coldData->builtinMetadata.category : shine::STextView{};
    }
    [[nodiscard]] shine::STextView GetDisplayNameView() const noexcept {
        if (coldData && !coldData->builtinMetadata.displayName.empty()) {
            return coldData->builtinMetadata.displayName;
        }
        return GetNameView();
    }
    [[nodiscard]] shine::STextView GetEditConditionView() const noexcept {
        return coldData ? coldData->builtinMetadata.editCondition : shine::STextView{};
    }
    [[nodiscard]] float GetMinValue() const noexcept {
        return coldData ? coldData->builtinMetadata.minValue : 0.0f;
    }
    [[nodiscard]] float GetMaxValue() const noexcept {
        return coldData ? coldData->builtinMetadata.maxValue : 0.0f;
    }

    [[nodiscard]] const FieldColdData* GetColdDataPtrForTests() const noexcept {
        return coldData.get();
    }

    [[nodiscard]] static consteval std::size_t ColdDataOffsetForTests() {
        return offsetof(FieldInfo, coldData);
    }

private:
    friend class TypeRegistry;
    template <typename, typename>
    friend struct TypeBuilder;

    FieldColdData& EnsureColdData() {
        if (!coldData) {
            coldData = MakeReflectionColdData<FieldColdData>();
        }
        return *coldData;
    }

    void SetColdData(ReflectionColdPtr<FieldColdData> value) noexcept {
        coldData = std::move(value);
    }

    [[nodiscard]] FieldColdData& MutableColdData() {
        return EnsureColdData();
    }

    ReflectionColdPtr<FieldColdData> coldData;
};

// =============================================================================
// MethodInfo — runtime method descriptor
// =============================================================================

struct MethodInfo {
    uint32_t            nameHash      = 0;
    InvokeFn            invokeFn      = nullptr;
    TypeId              returnType    = 0;
    uint64_t            signatureHash = 0;
    FunctionFlags       flags         = FunctionFlags::None;
    ReflectionOwnerHandle owner;
    mutable ReflectionColdPtr<MethodCallCache> callCache;

    void Invoke(void* inst, void** args, void* ret) const {
        if (invokeFn) invokeFn(inst, args, ret);
    }

    const MetadataValue* GetMeta(MetadataKey key) const {
        for (const auto& [k, v] : GetMetadata())
            if (k == key) return &v;
        return nullptr;
    }

    void SetName(shine::STextView value) {
        EnsureColdData().name = InternReflectionText(value);
    }

    [[nodiscard]] shine::STextView GetNameView() const noexcept {
        return coldData ? coldData->name : shine::STextView{};
    }

    [[nodiscard]] const ReflectionMetadataStorage& GetMetadata() const noexcept {
        static const ReflectionMetadataStorage kEmptyMetadata;
        return coldData ? coldData->metadata : kEmptyMetadata;
    }

    [[nodiscard]] std::span<const TypeId> GetParamTypes() const noexcept {
        if (!coldData) {
            return {};
        }
        return std::span<const TypeId>(coldData->paramTypes.data(), coldData->paramTypes.size());
    }

    [[nodiscard]] std::size_t GetParamCount() const noexcept {
        return coldData ? coldData->paramTypes.size() : 0;
    }

    [[nodiscard]] TypeId GetParamType(std::size_t index) const noexcept {
        const auto paramTypes = GetParamTypes();
        return index < paramTypes.size() ? paramTypes[index] : TypeId{};
    }

    ReflectionMetadataStorage& MutableMetadata() {
        return EnsureColdData().metadata;
    }

    ReflectionColdVector<TypeId>& MutableParamTypes() {
        return EnsureColdData().paramTypes;
    }

    [[nodiscard]] const TypeInfo* GetOwnerType() const {
        return owner.AsType();
    }

    [[nodiscard]] bool HasCallCache() const noexcept {
        return static_cast<bool>(callCache);
    }

    [[nodiscard]] const MethodCallCache* GetCallCache() const noexcept {
        return callCache.get();
    }

    [[nodiscard]] MethodCallCache& EnsureCallCache() const {
        if (!callCache) {
            callCache = MakeReflectionColdData<MethodCallCache>();
        }
        return *callCache;
    }

    [[nodiscard]] const MethodColdData* GetColdDataPtrForTests() const noexcept {
        return coldData.get();
    }

    [[nodiscard]] static consteval std::size_t ColdDataOffsetForTests() {
        return offsetof(MethodInfo, coldData);
    }

private:
    friend class TypeRegistry;
    template <typename, typename>
    friend struct TypeBuilder;

    MethodColdData& EnsureColdData() {
        if (!coldData) {
            coldData = MakeReflectionColdData<MethodColdData>();
        }
        return *coldData;
    }

    void SetColdData(ReflectionColdPtr<MethodColdData> value) noexcept {
        coldData = std::move(value);
    }

    [[nodiscard]] MethodColdData& MutableColdData() {
        return EnsureColdData();
    }

    ReflectionColdPtr<MethodColdData> coldData;
};

// =============================================================================
// TypeInfo — complete runtime type descriptor
// =============================================================================

struct TypeInfo {
    TypeId              id        = 0;
    std::size_t         size      = 0;
    std::size_t         alignment = 0;
    bool                isEnum    = false;
    bool                isPod     = false;  // 是否可平凡复制类型

    struct LookupEntry { uint32_t hash; uint32_t index; };

    [[nodiscard]] const FieldInfo* FindFieldFast(shine::STextView fieldName) const {
        uint32_t hash = Hash(fieldName);
        auto it = std::ranges::lower_bound(fieldLookup_, hash, {}, &LookupEntry::hash);

        // Handle collisions and multiple matches (rare)
        while (it != fieldLookup_.end() && it->hash == hash) {
            const auto& f = fields_[it->index];
            if (f.GetNameView() == fieldName) return &f;
            ++it;
        }
        return nullptr;
    }

    [[nodiscard]] const MethodInfo* FindMethodFast(shine::STextView methodName) const {
        uint32_t hash = Hash(methodName);
        auto it = std::ranges::lower_bound(methodLookup_, hash, {}, &LookupEntry::hash);

        while (it != methodLookup_.end() && it->hash == hash) {
            const auto& m = methods_[it->index];
            if (m.GetNameView() == methodName) return &m;
            ++it;
        }
        return nullptr;
    }

    [[nodiscard]] const FieldInfo* FindField(shine::STextView fieldName) const {
        return FindFieldFast(fieldName);
    }

    [[nodiscard]] const MethodInfo* FindMethod(shine::STextView methodName) const {
        return FindMethodFast(methodName);
    }

public:
    void SetName(shine::STextView value) {
        MutableColdData().name = InternReflectionText(value);
    }

    [[nodiscard]] shine::STextView GetNameView() const noexcept {
        return coldData ? coldData->name : shine::STextView{};
    }

    [[nodiscard]] const ReflectionColdVector<EnumEntry>& GetEnumEntries() const noexcept {
        static const ReflectionColdVector<EnumEntry> kEmptyEnumEntries;
        return coldData ? coldData->enumEntries : kEmptyEnumEntries;
    }

    [[nodiscard]] bool HasEnumEntries() const noexcept {
        return coldData && !coldData->enumEntries.empty();
    }

    [[nodiscard]] const ReflectionColdVector<FieldInfo>& GetFields() const noexcept {
        return fields_;
    }

    [[nodiscard]] const ReflectionColdVector<MethodInfo>& GetMethods() const noexcept {
        return methods_;
    }

    [[nodiscard]] const FieldInfo* GetFieldAt(std::size_t index) const noexcept {
        return index < fields_.size() ? &fields_[index] : nullptr;
    }

    [[nodiscard]] const MethodInfo* GetMethodAt(std::size_t index) const noexcept {
        return index < methods_.size() ? &methods_[index] : nullptr;
    }

    [[nodiscard]] static consteval std::size_t FieldsOffsetForTests() {
        return offsetof(TypeInfo, fields_);
    }

    [[nodiscard]] static consteval std::size_t MethodsOffsetForTests() {
        return offsetof(TypeInfo, methods_);
    }

    [[nodiscard]] static consteval std::size_t ColdDataOffsetForTests() {
        return offsetof(TypeInfo, coldData);
    }

    [[nodiscard]] static consteval std::size_t FieldLookupOffsetForTests() {
        return offsetof(TypeInfo, fieldLookup_);
    }

    [[nodiscard]] static consteval std::size_t MethodLookupOffsetForTests() {
        return offsetof(TypeInfo, methodLookup_);
    }

    [[nodiscard]] static consteval std::size_t LookupSortedOffsetForTests() {
        return offsetof(TypeInfo, lookupSorted_);
    }

private:
    friend class TypeRegistry;
    template <typename, typename>
    friend struct TypeBuilder;

    TypeColdData& MutableColdData() {
        if (!coldData) {
            coldData = MakeReflectionColdData<TypeColdData>();
        }
        return *coldData;
    }

    ReflectionColdVector<FieldInfo>& MutableFields() noexcept {
        lookupSorted_ = false;
        return fields_;
    }

    ReflectionColdVector<MethodInfo>& MutableMethods() noexcept {
        lookupSorted_ = false;
        return methods_;
    }

    ReflectionColdVector<EnumEntry>& MutableEnumEntries() {
        return MutableColdData().enumEntries;
    }

    ReflectionColdVector<FieldInfo>  fields_;
    ReflectionColdVector<MethodInfo> methods_;
    ReflectionColdPtr<TypeColdData> coldData;
    ReflectionColdVector<LookupEntry> fieldLookup_;
    ReflectionColdVector<LookupEntry> methodLookup_;
    bool lookupSorted_ = false;

public:

    void BuildLookup() {
        if (lookupSorted_) return;

        fieldLookup_.clear();
        fieldLookup_.reserve(fields_.size());
        for (uint32_t i = 0; i < (uint32_t)fields_.size(); ++i) {
            fieldLookup_.push_back({fields_[i].nameHash, i});
        }
        std::ranges::sort(fieldLookup_, {}, &LookupEntry::hash);

        methodLookup_.clear();
        methodLookup_.reserve(methods_.size());
        for (uint32_t i = 0; i < (uint32_t)methods_.size(); ++i) {
            methodLookup_.push_back({methods_[i].nameHash, i});
        }
        std::ranges::sort(methodLookup_, {}, &LookupEntry::hash);

        lookupSorted_ = true;
    }

    std::size_t GetFieldCount()  const { return fields_.size(); }
    std::size_t GetMethodCount() const { return methods_.size(); }
    std::size_t GetEnumCount()   const { return GetEnumEntries().size(); }
};

static_assert(alignof(TypeInfo) >= ReflectionOwnerHandle::kRequiredAlignment,
    "TypeInfo alignment must preserve owner handle tag bits");
static_assert(alignof(FieldInfo) >= ReflectionOwnerHandle::kRequiredAlignment,
    "FieldInfo alignment must preserve owner handle tag bits");
static_assert(alignof(MethodInfo) >= ReflectionOwnerHandle::kRequiredAlignment,
    "MethodInfo alignment must preserve owner handle tag bits");

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

// BoundMember - 编译期绑定访问包装，统一提供名称/偏移/指针/引用/get/set
// 用法: using Age = BoundMember<&Player::age>;
//      auto* ptr = Age::Ptr(player);
//      Age::Set(player, 42);
template <auto MemberPtr,
          typename C = typename detail::MemberPtrInfo<decltype(MemberPtr)>::ClassType,
          typename M = typename detail::MemberPtrInfo<decltype(MemberPtr)>::MemberType>
struct BoundMember {
    using ClassType = C;
    using MemberType = M;

    static constexpr auto Pointer = MemberPtr;

    [[nodiscard]] static constexpr shine::STextView Name() noexcept {
        return member_name<MemberPtr>();
    }

    [[nodiscard]] static std::size_t Offset() noexcept {
        return compute_offset(MemberPtr);
    }

    [[nodiscard]] [[gnu::always_inline]] static M* Ptr(C& obj) noexcept {
        return std::addressof(obj.*MemberPtr);
    }

    [[nodiscard]] [[gnu::always_inline]] static const M* Ptr(const C& obj) noexcept {
        return std::addressof(obj.*MemberPtr);
    }

    [[nodiscard]] [[gnu::always_inline]] static M& Ref(C& obj) noexcept {
        return obj.*MemberPtr;
    }

    [[nodiscard]] [[gnu::always_inline]] static const M& Ref(const C& obj) noexcept {
        return obj.*MemberPtr;
    }

    [[nodiscard]] [[gnu::always_inline]] static M Get(const C& obj) {
        return obj.*MemberPtr;
    }

    [[gnu::always_inline]] static void Set(C& obj, const M& val) {
        obj.*MemberPtr = val;
    }
};

namespace detail {

template <auto FirstMemberPtr, auto... RestMemberPtrs>
struct LastMemberPtr {
    static constexpr auto Value = LastMemberPtr<RestMemberPtrs...>::Value;
};

template <auto LastMemberPtrValue>
struct LastMemberPtr<LastMemberPtrValue> {
    static constexpr auto Value = LastMemberPtrValue;
};

template <typename CurrentType, auto... MemberPtrs>
struct BoundPathLeafType {
    using Type = CurrentType;
};

template <typename CurrentType, auto MemberPtr, auto... RestMemberPtrs>
struct BoundPathLeafType<CurrentType, MemberPtr, RestMemberPtrs...> {
    using Info = MemberPtrInfo<decltype(MemberPtr)>;
    static_assert(std::same_as<remove_cvref_t<CurrentType>, typename Info::ClassType>,
        "BoundPath member pointer chain does not match the previous member type");
    using Type = typename BoundPathLeafType<typename Info::MemberType, RestMemberPtrs...>::Type;
};

template <auto FirstMemberPtr, auto... RestMemberPtrs>
struct BoundPathAccess {
    template <typename Obj>
    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Ref(Obj&& obj) noexcept {
        if constexpr (sizeof...(RestMemberPtrs) == 0) {
            return (std::forward<Obj>(obj).*FirstMemberPtr);
        } else {
            return BoundPathAccess<RestMemberPtrs...>::Ref((std::forward<Obj>(obj).*FirstMemberPtr));
        }
    }
};

} // namespace detail

template <auto FirstMemberPtr, auto... RestMemberPtrs>
struct BoundPath {
    using RootType = typename detail::MemberPtrInfo<decltype(FirstMemberPtr)>::ClassType;
    using LeafType = typename detail::BoundPathLeafType<RootType, FirstMemberPtr, RestMemberPtrs...>::Type;

    static constexpr std::size_t Depth = 1 + sizeof...(RestMemberPtrs);

    [[nodiscard]] static constexpr shine::STextView LeafName() noexcept {
        return member_name<detail::LastMemberPtr<FirstMemberPtr, RestMemberPtrs...>::Value>();
    }

    [[nodiscard]] static shine::SString Path() {
        shine::SString path(BoundMember<FirstMemberPtr>::Name());
        ((path += shine::STextView::from_literal("."), path += member_name<RestMemberPtrs>()), ...);
        return path;
    }

    [[nodiscard]] static std::size_t Offset() noexcept {
        std::size_t offset = compute_offset(FirstMemberPtr);
        ((offset += compute_offset(RestMemberPtrs)), ...);
        return offset;
    }

    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Ref(RootType& obj) noexcept {
        return detail::BoundPathAccess<FirstMemberPtr, RestMemberPtrs...>::Ref(obj);
    }

    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Ref(const RootType& obj) noexcept {
        return detail::BoundPathAccess<FirstMemberPtr, RestMemberPtrs...>::Ref(obj);
    }

    [[nodiscard]] [[gnu::always_inline]] static auto* Ptr(RootType& obj) noexcept {
        return std::addressof(Ref(obj));
    }

    [[nodiscard]] [[gnu::always_inline]] static const auto* Ptr(const RootType& obj) noexcept {
        return std::addressof(Ref(obj));
    }

    [[nodiscard]] [[gnu::always_inline]] static LeafType Get(const RootType& obj) {
        return Ref(obj);
    }

    [[gnu::always_inline]] static void Set(RootType& obj, const LeafType& val) {
        Ref(obj) = val;
    }
};

template <auto MethodPtr,
          typename C = typename detail::MemberFunctionPtrInfo<decltype(MethodPtr)>::ClassType,
          typename R = typename detail::MemberFunctionPtrInfo<decltype(MethodPtr)>::ReturnType>
struct BoundMethod {
    using ClassType = C;
    using ReturnType = R;

    static constexpr auto Pointer = MethodPtr;

    [[nodiscard]] static constexpr shine::STextView Name() noexcept {
        return member_name<MethodPtr>();
    }

    template <typename Obj, typename... Args>
    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Invoke(Obj&& obj, Args&&... args) {
        return std::invoke(MethodPtr, std::forward<Obj>(obj), std::forward<Args>(args)...);
    }
};

template <auto MethodPtr, auto... MemberPtrs>
struct BoundMethodPath {
    using PathType = BoundPath<MemberPtrs...>;
    using RootType = typename PathType::RootType;
    using TargetType = remove_cvref_t<typename PathType::LeafType>;
    using MethodType = BoundMethod<MethodPtr>;

    static_assert(std::same_as<TargetType, typename MethodType::ClassType>,
        "BoundMethodPath target type does not match the leaf type of the member path");

    [[nodiscard]] static shine::SString Path() {
        shine::SString path = PathType::Path();
        path += shine::STextView::from_literal(".");
        path += MethodType::Name();
        return path;
    }

    [[nodiscard]] static constexpr shine::STextView MethodName() noexcept {
        return MethodType::Name();
    }

    template <typename... Args>
    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Invoke(RootType& obj, Args&&... args) {
        return MethodType::Invoke(PathType::Ref(obj), std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] [[gnu::always_inline]] static decltype(auto) Invoke(const RootType& obj, Args&&... args) {
        return MethodType::Invoke(PathType::Ref(obj), std::forward<Args>(args)...);
    }
};

} // namespace shine::reflection
