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

#include "../ReflectionCore.h"
#include "../TypeRegistry.h"
#include "../Script/ScriptValue.h"
#include "../Script/ScriptBridge.h"
#include "../Core/TypeView.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>

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

        ptr = ::operator new(requestedSize, std::align_val_t{requestedAlign});
        heapAlignment = requestedAlign;
    }

    ~ScratchBuffer() {
        if (heapAlignment != 0) {
            ::operator delete(ptr, std::align_val_t{heapAlignment});
        }
    }
};

inline bool BuildMethodCallCache(const MethodInfo& method) {
    method.cachedReturnTypeInfo = nullptr;
    method.cachedParamTypeInfos.clear();
    method.cachedParamOffsets.clear();
    method.cachedReturnOffset = 0;
    method.cachedFrameSize = 0;
    method.cachedFrameAlignment = alignof(std::max_align_t);

    if (method.returnType != GetTypeId<void>()) {
        const auto* returnType = TypeRegistry::Get().FindFast(method.returnType);
        if (!returnType) {
            method.callCacheValid = false;
            return false;
        }

        method.cachedReturnTypeInfo = returnType;
        method.cachedFrameAlignment = std::max(method.cachedFrameAlignment, returnType->alignment);
        method.cachedReturnOffset = AlignUp(method.cachedFrameSize, returnType->alignment);
        method.cachedFrameSize = method.cachedReturnOffset + returnType->size;
    }

    method.cachedParamTypeInfos.reserve(method.paramTypes.size());
    method.cachedParamOffsets.reserve(method.paramTypes.size());
    for (const auto paramTypeId : method.paramTypes) {
        const auto* paramType = TypeRegistry::Get().FindFast(paramTypeId);
        if (!paramType) {
            method.callCacheValid = false;
            return false;
        }

        method.cachedParamTypeInfos.push_back(paramType);
        method.cachedFrameAlignment = std::max(method.cachedFrameAlignment, paramType->alignment);
        const auto offset = AlignUp(method.cachedFrameSize, paramType->alignment);
        method.cachedParamOffsets.push_back(offset);
        method.cachedFrameSize = offset + paramType->size;
    }

    method.cachedFrameSize = AlignUp(method.cachedFrameSize, method.cachedFrameAlignment);
    method.callCacheValid = true;
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
        if (!method->callCacheValid && !detail::BuildMethodCallCache(*method))
            return {};

        detail::ScratchBuffer buffer(method->cachedFrameSize, method->cachedFrameAlignment);
        char* current = static_cast<char*>(buffer.ptr);

        void* retPtr = nullptr;
        if (const auto* returnType = method->cachedReturnTypeInfo) {
            retPtr = current + method->cachedReturnOffset;
            if (!returnType->isPod) Construct(retPtr, returnType->id);
        }

        std::vector<void*> rawArgs(args.size());
        for (std::size_t i = 0; i < args.size(); ++i) {
            const TypeInfo* paramType = method->cachedParamTypeInfos[i];
            void* argPtr = current + method->cachedParamOffsets[i];
            if (!paramType->isPod) Construct(argPtr, paramType->id);
            bridge.FromScript(args[i], argPtr, paramType->id);
            rawArgs[i] = argPtr;
        }

        method->Invoke(instance, rawArgs.data(), retPtr);
        
        ScriptValue result = (retPtr && method->cachedReturnTypeInfo)
            ? bridge.ToScript(retPtr, method->returnType)
            : ScriptValue{};
        
        // Cleanup
        if (retPtr && method->cachedReturnTypeInfo && !method->cachedReturnTypeInfo->isPod)
            Destruct(retPtr, method->cachedReturnTypeInfo->id);
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (!method->cachedParamTypeInfos[i]->isPod)
                Destruct(rawArgs[i], method->cachedParamTypeInfos[i]->id);
        }
        
        return result;
    }

    ScriptValue CallMethod(void* inst, std::string_view n,
                           const std::vector<ScriptValue>& a, const ScriptBridge& br) const {
        return CallMethod(inst, GetMethodInfo(n), a, br);
    }
};

} // namespace shine::reflection
