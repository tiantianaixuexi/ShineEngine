#pragma once




#include "shine_define.h"

namespace shine::render::command
{
    enum class IndexType : u8
    {
        Uint16,
        Uint32
    };

    enum class RenderAPI : u8
    {
        OpenGL,
        Vulkan,
        DirectX12,
        WebGL

    };

    class ICommandList
    {
    public:
        virtual ~ICommandList() = default;

        // Lifecycle
        virtual void begin() = 0;
        virtual void end() = 0;
        virtual void execute() = 0;
        virtual void reset() = 0;

        // Frame / target
        virtual void bindFramebuffer(u64 framebufferHandle) = 0; //
        virtual void setViewport(s32 x, s32 y, s32 width, s32 height) = 0;

        // Clear / state
        virtual void clearColor(float r, float g, float b, float a) = 0;
        virtual void clear(bool clearColorBuffer, bool clearDepthBuffer) = 0;
        virtual void enableDepthTest(bool enabled) = 0;

        // Pipeline / geometry
        virtual void useProgram(u64 programHandle) = 0;
        virtual void bindVertexArray(u64 vaoHandle) = 0;
        virtual void drawTriangles(s32 firstVertex,s32 vertexCount) = 0;
        virtual void drawIndexedTriangles(s32 indexCount, IndexType indexType,
                                         u64 indexBufferOffsetBytes = 0) = 0;

        // UI
        virtual void imguiRender(void* drawData) = 0; // backend-specific draw data

        // Present
        virtual void swapBuffers(void* nativeSwapContext) = 0; // e.g. HDC on Windows + OpenGL

    };
}

