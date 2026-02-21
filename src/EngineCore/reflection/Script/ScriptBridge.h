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

namespace shine::reflection {

// =============================================================================
// ScriptBridge — abstract conversion interface
// =============================================================================

struct ScriptBridge {
    virtual ~ScriptBridge() = default;
    virtual ScriptValue ToScript(const void*, TypeId)                    const { return {}; }
    virtual void        FromScript(const ScriptValue&, void*, TypeId)    const {}
};

// =============================================================================
// Helper functions for construction/destruction
// =============================================================================

inline void Construct(void*, TypeId) {}
inline void Destruct(void*, TypeId)  {}

} // namespace shine::reflection