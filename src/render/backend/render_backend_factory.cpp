#include "render_backend_factory.h"

// Include concrete backend headers
#if defined(SHINE_PLATFORM_WIN) || defined(SHINE_PLATFORM_LINUX)
#include "opengl/opengl3_backend.h"
#endif

#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
#include "webgl2/webgl2_backend.h"
#endif

// Future: #include "dx12/dx12_backend.h"
// Future: #include "vulkan/vulkan_backend.h"

namespace shine::render::backend
{
    IRenderBackend* RenderBackendFactory::create(RenderBackendType type)
    {
        switch (type)
        {
        case RenderBackendType::OpenGL:
#if defined(SHINE_PLATFORM_WIN) || defined(SHINE_PLATFORM_LINUX)
            // In a real scenario, we might want to check system capabilities here
            return new opengl3::OpenGLRenderBackend();
#else
            return nullptr;
#endif

        case RenderBackendType::WebGL:
#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
            return new webgl2::WebGL2RenderBackend();
#else
            return nullptr;
#endif

        case RenderBackendType::DX12:
            // return new dx12::DX12RenderBackend();
            return nullptr;

        case RenderBackendType::Vulkan:
            // return new vulkan::VulkanRenderBackend();
            return nullptr;

        case RenderBackendType::Metal:
            return nullptr;

        default:
            return nullptr;
        }
    }
}
