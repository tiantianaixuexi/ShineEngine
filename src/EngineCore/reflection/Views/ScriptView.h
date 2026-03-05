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
#include <memory>

namespace shine::reflection {

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

// =============================================================================
// ScriptView — Script integration interface
// =============================================================================

struct ScriptView : TypeView {
    static const TypeInfo* GetTypeInfo(TypeId id) {
        auto result = TypeRegistry::Get().Find(id);
        return result.has_value() ? result.value() : nullptr;
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
            if (!pt->isPod) Construct(buf.get(), pt->id);
            bridge.FromScript(args[i], buf.get(), method->paramTypes[i]);
            rawArgs[i] = buf.get();
            argBufs.push_back(std::move(buf));
        }

        const TypeInfo* rt = (method->returnType != GetTypeId<void>())
                                 ? GetTypeInfo(method->returnType) : nullptr;
        std::unique_ptr<char[]> retBuf;
        void* retPtr = nullptr;
        if (rt) {
            retBuf = std::make_unique<char[]>(rt->size);
            retPtr = retBuf.get();
            if (!rt->isPod) Construct(retPtr, rt->id);
        }

        method->Invoke(instance, rawArgs.data(), retPtr);
        ScriptValue result = (rt && retPtr) ? bridge.ToScript(retPtr, method->returnType) : ScriptValue{};
        if (rt && retPtr && !rt->isPod) Destruct(retPtr, rt->id);
        for (std::size_t i = 0; i < args.size(); ++i) {
            const TypeInfo* pt = GetTypeInfo(method->paramTypes[i]);
            if (pt && !pt->isPod) Destruct(rawArgs[i], pt->id);
        }
        return result;
    }

    ScriptValue CallMethod(void* inst, std::string_view n,
                           const std::vector<ScriptValue>& a, const ScriptBridge& br) const {
        return CallMethod(inst, GetMethodInfo(n), a, br);
    }
};

} // namespace shine::reflection
