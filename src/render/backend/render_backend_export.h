#pragma once

#include "backend/opengl/opengl3_backend.h"
#include "backend/webgl2/webgl2_backend.h"

namespace shine::render::backend
{
#ifdef SHINE_WEBGL2
    using SRenderBackend = webgl2::WebGL2RenderBackend;
#elif defined(SHINE_OPENGL)
    using SRenderBackend = opengl3::OpenGLRenderBackend;
#elif defined(SHINE_DX12)
    using SRenderBackend = DX12RenderBackend;
#endif
}
