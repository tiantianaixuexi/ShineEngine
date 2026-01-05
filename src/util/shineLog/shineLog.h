#pragma once

#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"
#include <iostream>

namespace shine::log
{
    // 日志类
    class ShineLog : public shine::Subsystem
    {
    public:
        static constexpr size_t GetStaticID() { return shine::HashString("ShineLog"); }

        void Log(const char* msg);
    };
}
