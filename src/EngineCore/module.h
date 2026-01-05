#pragma once

#include "engine_context.h"

namespace shine
{
    class IModule
    {
    public:
        virtual ~IModule() = default;

        virtual const char* Name() const = 0;
        virtual bool Init(EngineContext& ctx) = 0;
        virtual void Shutdown(EngineContext& ctx) = 0;
    };
}
