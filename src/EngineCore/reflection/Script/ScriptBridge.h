#pragma once

// =============================================================================
// ScriptBridge.h — Script Bridge Interface for Reflection System
// =============================================================================
//
// Abstract interface for converting between C++ values and script values.
// Allows integration with different scripting engines (Lua, JavaScript, etc.).
//
// C++23 / MSVC
// =============================================================================

#include "ScriptValue.h"
#include "../ReflectionCore.h"
#include "string/shine_string.h"

#include <cstdint>
#include <new>
#include <string>

namespace shine::reflection {

struct FieldInfo; // Forward decl

// =============================================================================
// ScriptBridge — abstract conversion interface
// =============================================================================

struct ScriptBridge {
    virtual ~ScriptBridge() = default;
    virtual ScriptValue ToScript(const void*, TypeId)                    const { return {}; }
    virtual void        FromScript(const ScriptValue&, void*, TypeId)    const {}
    
    virtual ScriptValue ToScriptField(const void* ptr, const FieldInfo* field) const {
        if (!field) return {};
        return ToScript(ptr, field->typeId);
    }
    virtual void        FromScriptField(const ScriptValue& val, void* ptr, const FieldInfo* field) const {
        if (field) FromScript(val, ptr, field->typeId);
    }
};

// =============================================================================
// Helper functions for construction/destruction
// =============================================================================

inline void Construct(void* target, TypeId typeId)
{
    if (!target)
    {
        return;
    }
    if (typeId == GetTypeId<std::string>())
    {
        new (target) std::string();
        return;
    }
    if (typeId == GetTypeId<shine::SString>())
    {
        new (target) shine::SString();
        return;
    }
    if (typeId == GetTypeId<bool>())
    {
        new (target) bool(false);
        return;
    }
    if (typeId == GetTypeId<int>())
    {
        new (target) int(0);
        return;
    }
    if (typeId == GetTypeId<uint32_t>())
    {
        new (target) uint32_t(0);
        return;
    }
    if (typeId == GetTypeId<float>())
    {
        new (target) float(0.0f);
        return;
    }
    if (typeId == GetTypeId<double>())
    {
        new (target) double(0.0);
    }
}

inline void Destruct(void* target, TypeId typeId)
{
    if (!target)
    {
        return;
    }
    if (typeId == GetTypeId<std::string>())
    {
        static_cast<std::string*>(target)->~basic_string();
        return;
    }
    if (typeId == GetTypeId<shine::SString>())
    {
        static_cast<shine::SString*>(target)->~SString();
    }
}

} // namespace shine::reflection
