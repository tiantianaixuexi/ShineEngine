#pragma once

#include "util/singleton.h"
#include "gameplay/camera.h"

namespace shine::manager
{

    class CameraManager : public shine::util::Singleton<CameraManager>
    {
        public:

            gameplay::Camera* getMainCamera() noexcept;
            void setMainCamera(gameplay::Camera* camera) noexcept;
        private:

            gameplay::Camera* mainCamera;
    };


}

