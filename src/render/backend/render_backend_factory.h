#pragma once

#include "render_backend.h"
#include "render_backend_type.h"

namespace shine::render::backend
{
    class RenderBackendFactory
    {
    public:
        /**
         * @brief Create a render backend instance
         * @param type The type of backend to create
         * @return Pointer to the backend instance, or nullptr if not supported/available
         */
        static IRenderBackend* create(RenderBackendType type);
    };
}
