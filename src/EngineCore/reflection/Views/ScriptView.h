#pragma once

// =============================================================================
// ScriptView.h — Script Integration View for Reflection System
// =============================================================================
//
// Provides script-friendly access to reflected types, fields, and methods.
// Handles type-erased value conversion via ScriptBridge.
//
// C++23 / MSVC
// =============================================================================

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <new>

#include "memory/memory.ixx"
#include "../ReflectionCore.h"
#include "../TypeRegistry.h"
#include "../Script/ScriptValue.h"
#include "../Script/ScriptBridge.h"
#include "../Core/TypeView.h"


namespace shine::reflection {

namespace detail {

constexpr std::size_t AlignUp(std::size_t value, std::size_t alignment) noexcept {
    if (alignment <= 1) return value;
    const auto mask = alignment - 1;
    return (value + mask) & ~mask;
}

/// Stack-or-heap scratch buffer for type-erased temporaries.
struct ScratchBuffer {
    static constexpr std::size_t kStack = 128;
    alignas(std::max_align_t) std::byte stackBuf[kStack]{};
    void* ptr = nullptr;
    std::size_t heapAlignment = 0;

    explicit ScratchBuffer(std::size_t sz, std::size_t align = 16)
    {
        const auto requestedSize = std::max<std::size_t>(sz, 1);
        const auto requestedAlign = std::max<std::size_t>(align, alignof(std::max_align_t));

        if (requestedSize <= kStack && requestedAlign <= alignof(std::max_align_t)) {
            ptr = stackBuf;
            return;
        }

        shine::co::MemoryScope scope(shine::co::MemoryTag::ScriptBridgeTemp);
        ptr = shine::co::Memory::Alloc(requestedSize, requestedAlign);
        heapAlignment = requestedAlign;
    }

    ~ScratchBuffer() {
        if (heapAlignment != 0) {
            shine::co::Memory::Free(ptr);
        }
    }
};

template <typename T, std::size_t StackCount, shine::co::MemoryTag Tag>
struct TaggedTempArray {
    static_assert(StackCount > 0, "TaggedTempArray requires at least one inline slot");

    std::array<T, StackCount> stackStorage{};
    T* ptr = stackStorage.data();
    std::size_t count = 0;
    bool usesHeap = false;

    explicit TaggedTempArray(std::size_t requestedCount)
        : count(requestedCount) {
        if (requestedCount <= StackCount) {
            return;
        }

        shine::co::MemoryScope scope(Tag);
        ptr = static_cast<T*>(shine::co::Memory::Alloc(sizeof(T) * requestedCount, alignof(T)));
        assert(ptr && "TaggedTempArray allocation failed");
        usesHeap = ptr != nullptr;
    }

    ~TaggedTempArray() {
        if (usesHeap) {
            shine::co::Memory::Free(ptr);
        }
    }

    TaggedTempArray(const TaggedTempArray&) = delete;
    TaggedTempArray& operator=(const TaggedTempArray&) = delete;
    TaggedTempArray(TaggedTempArray&&) = delete;
    TaggedTempArray& operator=(TaggedTempArray&&) = delete;

    T& operator[](std::size_t index) noexcept {
        return ptr[index];
    }

    const T& operator[](std::size_t index) const noexcept {
        return ptr[index];
    }

    T* data() noexcept {
        return ptr;
    }
};

inline bool BuildMethodCallCache(const MethodInfo& method) {
    auto& cache = method.EnsureCallCache();
    cache.returnTypeInfo = nullptr;
    cache.ClearParams();
    cache.returnOffset = 0;
    cache.frameSize = 0;
    cache.frameAlignment = alignof(std::max_align_t);

    if (method.returnType != GetTypeId<void>()) {
        const auto* returnType = TypeRegistry::Get().FindFast(method.returnType);
        if (!returnType) {
            cache.valid = false;
            return false;
        }

        cache.returnTypeInfo = returnType;
        cache.frameAlignment = std::max(cache.frameAlignment, returnType->alignment);
        cache.returnOffset = AlignUp(cache.frameSize, returnType->alignment);
        cache.frameSize = cache.returnOffset + returnType->size;
    }

    cache.ReserveParams(method.paramTypes.size());
    for (const auto paramTypeId : method.paramTypes) {
        const auto* paramType = TypeRegistry::Get().FindFast(paramTypeId);
        if (!paramType) {
            cache.valid = false;
            return false;
        }

        cache.frameAlignment = std::max(cache.frameAlignment, paramType->alignment);
        const auto offset = AlignUp(cache.frameSize, paramType->alignment);
        cache.AddParam(paramType, offset);
        cache.frameSize = offset + paramType->size;
    }

    cache.frameSize = AlignUp(cache.frameSize, cache.frameAlignment);
    cache.valid = true;
    return true;
}

} // namespace detail

// =============================================================================
// ScriptView — Script integration interface
// =============================================================================

struct ScriptView : TypeView {
    static const TypeInfo* GetTypeInfo(TypeId id) {
        return TypeRegistry::Get().FindFast(id);
    }

    [[nodiscard]] const FieldInfo*  GetFieldInfo(shine::STextView n) const { return typeInfo->FindField(n); }
     [[nodiscard]] const FieldInfo*  GetFieldInfo(std::size_t i)      const {
        return i < typeInfo->fields.size() ? &typeInfo->fields[i] : nullptr;
    }
    [[nodiscard]] const MethodInfo* GetMethodInfo(shine::STextView n) const { return typeInfo->FindMethod(n); }
     [[nodiscard]] const MethodInfo* GetMethodInfo(std::size_t i)      const {
        return i < typeInfo->methods.size() ? &typeInfo->methods[i] : nullptr;
    }

    // --- Field get / set (with stack-buffer optimisation) --------------------

    ScriptValue GetField(void* instance, const FieldInfo* field, const ScriptBridge& bridge) const {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptRead)) return {};

        detail::ScratchBuffer buf(field->size, field->alignment);
        if (!field->isPod) Construct(buf.ptr, field->typeId);
        field->Get(instance, buf.ptr);
        auto result = bridge.ToScriptField(buf.ptr, field);
        if (!field->isPod) Destruct(buf.ptr, field->typeId);
        return result;
    }

    void SetField(void* instance, const FieldInfo* field,
                  const ScriptValue& value, const ScriptBridge& bridge) const
    {
        if (!field || !HasFlag(field->flags, PropertyFlags::ScriptWrite)) return;

        detail::ScratchBuffer buf(field->size, field->alignment);
        if (!field->isPod) Construct(buf.ptr, field->typeId);
        bridge.FromScriptField(value, buf.ptr, field);
        field->Set(instance, buf.ptr);
        if (!field->isPod) Destruct(buf.ptr, field->typeId);
    }

    ScriptValue GetField(void* inst, shine::STextView n, const ScriptBridge& br) const {
        return GetField(inst, GetFieldInfo(n), br);
    }
    void SetField(void* inst, shine::STextView n, const ScriptValue& v, const ScriptBridge& br) const {
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
        if ((!method->GetCallCache() || !method->GetCallCache()->valid) && !detail::BuildMethodCallCache(*method))
            return {};

        const auto* cachePtr = method->GetCallCache();
        if (!cachePtr) {
            return {};
        }

        const auto& cache = *cachePtr;
        detail::ScratchBuffer buffer(cache.frameSize, cache.frameAlignment);
        char* current = static_cast<char*>(buffer.ptr);

        void* retPtr = nullptr;
        if (const auto* returnType = cache.returnTypeInfo) {
            retPtr = current + cache.returnOffset;
            if (!returnType->isPod) Construct(retPtr, returnType->id);
        }

        detail::TaggedTempArray<void*, 2, shine::co::MemoryTag::ReflectionTemp> rawArgs(args.size());
        for (std::size_t i = 0; i < args.size(); ++i) {
            const TypeInfo* paramType = cache.GetParamTypeInfo(i);
            void* argPtr = current + cache.GetParamOffset(i);
            if (!paramType->isPod) Construct(argPtr, paramType->id);
            bridge.FromScript(args[i], argPtr, paramType->id);
            rawArgs[i] = argPtr;
        }

        method->Invoke(instance, rawArgs.data(), retPtr);

        ScriptValue result = (retPtr && cache.returnTypeInfo)
            ? bridge.ToScript(retPtr, method->returnType)
            : ScriptValue{};

        // Cleanup
        if (retPtr && cache.returnTypeInfo && !cache.returnTypeInfo->isPod)
            Destruct(retPtr, cache.returnTypeInfo->id);
        for (std::size_t i = 0; i < args.size(); ++i) {
            const TypeInfo* paramType = cache.GetParamTypeInfo(i);
            if (!paramType->isPod)
                Destruct(rawArgs[i], paramType->id);
        }

        return result;
    }

    ScriptValue CallMethod(void* inst, shine::STextView n,
                           const std::vector<ScriptValue>& a, const ScriptBridge& br) const {
        return CallMethod(inst, GetMethodInfo(n), a, br);
    }
};

} // namespace shine::reflection
