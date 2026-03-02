#pragma once

#include <string>
#include "shine_define.h"

namespace shine::render
{
    class DebugTextureSink
    {
    public:
        virtual ~DebugTextureSink() = default;
        virtual void RegisterTexture(const std::string& name, u32 textureId, int width, int height) = 0;
    };
}
