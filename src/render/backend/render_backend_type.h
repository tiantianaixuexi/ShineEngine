#pragma once
#include "util/shine_define.h"

namespace shine::render::backend
{
    enum class RenderBackendType : u8
    {
        OpenGL,
        Vulkan,
        DX12,
        Metal,
        WebGL
    };
}
