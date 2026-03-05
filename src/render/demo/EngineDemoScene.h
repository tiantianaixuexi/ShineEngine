#pragma once

#include <cstdint>

#include "EngineCore/engine_context.h"
#include "gameplay/object.h"

namespace shine::render::demo
{
    class EngineDemoScene
    {
    public:
        EngineDemoScene(shine::EngineContext& context);
        ~EngineDemoScene();

        void Init();

    private:
        shine::EngineContext& m_Context;
    };
}
