#include "CameraManager.h"

#include "gameplay/camera.h"

namespace shine::manager
{

    shine::gameplay::Camera* CameraManager::getMainCamera() noexcept
    {
        return mainCamera;
    }

    void CameraManager::setMainCamera(shine::gameplay::Camera* camera) noexcept
    {
        mainCamera = camera;
    }

}


