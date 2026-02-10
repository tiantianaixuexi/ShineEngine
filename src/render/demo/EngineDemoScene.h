#pragma once

#include "EngineCore/engine_context.h"
#include "gameplay/object.h"
#include <vector>
#include <memory>

namespace shine::render::demo
{
    class EngineDemoScene
    {
    public:
        EngineDemoScene(shine::EngineContext& context);
        ~EngineDemoScene();

        void Init();
        void Tick(float deltaTime);

    private:
        shine::EngineContext& m_Context;
        std::vector<std::unique_ptr<shine::gameplay::SObject>> m_Objects;
        
        float m_RotationAngle = 0.0f;
    };
}
