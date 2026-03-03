#pragma once

#include "shine_define.h"
#include "util/EnumFlags.h"

namespace shine::gameplay
{

    enum class EObjectFlags : u64
    {
        OF_Active      = 1u << 0,
        OF_Visible     = 1u << 1,
        OF_Tick        = 1u << 2,
        OF_Render      = 1u << 3,
        OF_Pointer     = 1u << 4,
        OF_PendingKill = 1u << 5,
        OF_GCMark      = 1u << 6,
    };
}

ENABLE_ENUM_FLAGS(shine::gameplay::EObjectFlags)