#pragma once

#include "shine_define.h"

namespace shine::gameplay
{
    enum class ETickGroup : u8
    {
        PrePhysics = 0,
        Physics,
        PostPhysics,
        Late,

        COUNT
    };

    enum class ETickUpdateMode : u8
    {
        Fixed,
        Variable,

        COUNT
    };

}