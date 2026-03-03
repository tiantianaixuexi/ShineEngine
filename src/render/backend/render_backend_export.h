#pragma once

// ============================================================================
// Compile-time backend selection via type alias
// Use RenderBackendFactory for runtime selection instead when possible.
// ============================================================================

#if defined(SHINE_OPENGL)
#include "opengl/opengl3_backend.h"
namespace shine::render::backend {
    using SRenderBackend = opengl3::OpenGLRenderBackend;
}
#elif defined(SHINE_DX12)
// Future: #include "dx12/dx12_backend.h"
// namespace shine::render::backend { using SRenderBackend = dx12::DX12RenderBackend; }
#endif
