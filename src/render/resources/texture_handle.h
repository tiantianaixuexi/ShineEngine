#pragma once

#include <cstdint>

namespace shine::render
{
    /**
     * @brief 纹理句柄（跨API统一）
     */
    struct TextureHandle
    {
        uint64_t id = 0;
        bool isValid() const { return id != 0; }
    };
}

