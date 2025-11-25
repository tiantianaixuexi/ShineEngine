#pragma once

#include "command_list.h"

namespace shine::render::webgl2
{
    using command::ICommandList;

    // WebGL2 command list implementation using OpenGL ES 3.0 API
    class WebGL2CommandList final : public ICommandList
    {
    public:
        // Lifecycle
        void begin() override;
        void end() override;
        void execute() override;
        void reset() override;

        // Frame / target
        void bindFramebuffer(u64 framebufferHandle) override;
        void setViewport(s32 x, s32 y, s32 width, s32 height) override;

        // Clear / state
        void clearColor(float r, float g, float b, float a) override;
        void clear(bool clearColorBuffer, bool clearDepthBuffer) override;
        void enableDepthTest(bool enabled) override;

        // Pipeline / geometry
        void useProgram(u64 programHandle) override;
        void bindVertexArray(u64 vaoHandle) override;
        void drawTriangles(s32 firstVertex, s32 vertexCount) override;
        void drawIndexedTriangles(s32 indexCount, command::IndexType indexType,
                                  u64 indexBufferOffsetBytes = 0) override;

        // Uniforms
        void setUniform1f(s32 location, float value) override;
        void setUniform3f(s32 location, float x, float y, float z) override;

        // UI
        void imguiRender(void* drawData) override;

        // Present
        void swapBuffers(void* nativeSwapContext) override;
    };
}

