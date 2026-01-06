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
        void Log(const char* msg);
    };
}
