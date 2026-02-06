#include "render_backend_factory.h"

// Include concrete backend headers
#if defined(SHINE_PLATFORM_WIN) || defined(SHINE_PLATFORM_LINUX)
#include "opengl/opengl3_backend.h"
#endif

#if defined(SHINE_PLATFORM_WASM)
#include "webgl2/webgl2_backend.h"
#endif

namespace shine::render::backend
{
    std::unique_ptr<IRenderBackend> RenderBackendFactory::create(RenderBackendType type)
    {
        switch (type)
        {
        case RenderBackendType::OpenGL:
#if defined(SHINE_PLATFORM_WIN) || defined(SHINE_PLATFORM_LINUX)
            return std::make_unique<opengl3::OpenGLRenderBackend>();
#else
            return nullptr;
#endif

        case RenderBackendType::WebGL:
#if defined(SHINE_PLATFORM_WASM)
            return std::make_unique<webgl2::WebGL2RenderBackend>();
#else
            return nullptr;
#endif

        case RenderBackendType::DX12:
        case RenderBackendType::Vulkan:
        case RenderBackendType::Metal:
        default:
            return nullptr;
        }
    }
}
