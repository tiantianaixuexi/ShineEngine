#pragma once

#include "command_list.h"


namespace shine::render::opengl3
{
    using command::ICommandList;

    // Immediate-mode OpenGL command list. Methods issue GL calls directly.
    class OpenGLCommandList final : public ICommandList
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

        // UI
        void imguiRender(void* drawData) override;

        // Present
        void swapBuffers(void* nativeSwapContext) override;
    };
}

