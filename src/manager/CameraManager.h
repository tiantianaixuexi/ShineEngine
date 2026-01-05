#pragma once

#include "../../EngineCore/subsystem.h"
#include "../../EngineCore/engine_context.h"
#include "gameplay/camera.h"

namespace shine::manager
{
    class CameraManager : public shine::Subsystem
    {
    public:
        static constexpr size_t GetStaticID() { return shine::HashString("CameraManager"); }

        gameplay::Camera* getMainCamera() noexcept;
        void setMainCamera(gameplay::Camera* camera) noexcept;
        
    private:
        gameplay::Camera* mainCamera;
    };
}
