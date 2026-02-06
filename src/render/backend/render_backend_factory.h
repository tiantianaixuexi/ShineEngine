#pragma once

#include <memory>
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
         * @return unique_ptr to the backend, or nullptr if not supported
         */
        [[nodiscard]]
        static std::unique_ptr<IRenderBackend> create(RenderBackendType type);
    };
}
