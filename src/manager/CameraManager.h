#pragma once

#include "EngineCore/subsystem.h"
#include "EngineCore/engine_context.h"
#include "gameplay/camera.h"

namespace shine::manager
{
    class CameraManager : public Subsystem
    {
    public:
        static CameraManager& get() { return *EngineContext::Get().GetSystem<CameraManager>(); }

        gameplay::Camera* getMainCamera() noexcept;
        void setMainCamera(gameplay::Camera* camera) noexcept;
        
    private:
        gameplay::Camera* mainCamera;
    };
}
