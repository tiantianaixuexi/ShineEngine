#pragma once

#include <cstdint>
#include "util/EnumFlags.h"

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


enum class FunctionFlags : uint64_t {
    None           = 0,
    ScriptCallable = 1 << 0,
    EditorCallable = 1 << 1,
    Const          = 1 << 2,
    Static         = 1 << 3,
};

 enum class ContainerType {
     None,
     Sequence,
     Associative
 };

ENABLE_ENUM_FLAGS(PropertyFlags)

ENABLE_ENUM_FLAGS(FunctionFlags)
