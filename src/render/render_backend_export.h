#pragma once

#include "opengl/opengl3_backend.h"

namespace shine::render::backend
{
#ifdef SHINE_OPENGL
    using SRenderBackend = opengl3::OpenGLRenderBackend;
#elif SHINE_DX12
    using SRenderBackend = DX12RenderBackend;
#endif
}
