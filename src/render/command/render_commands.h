#pragma once

#include "util/shine_define.h"
#include <variant>
#include <vector>

namespace shine::render::command
{
    enum class IndexType : u8
    {
        Uint16,
        Uint32
    };

    // Lifecycle
    struct CmdBegin {};
    struct CmdEnd {};
    struct CmdExecute {}; // Might be redundant if we just iterate the vector, but keeping for parity
    struct CmdReset {};

    // Frame / target
    struct CmdBindFramebuffer {
        u64 framebufferHandle;
    };

    struct CmdSetViewport {
        s32 x, y, width, height;
    };

    // Clear / state
    struct CmdClearColor {
        float r, g, b, a;
    };

    struct CmdClear {
        bool clearColorBuffer;
        bool clearDepthBuffer;
    };

    struct CmdEnableDepthTest {
        bool enabled;
    };

    // Pipeline / geometry
    struct CmdUseProgram {
        u64 programHandle;
    };

    struct CmdBindVertexArray {
        u64 vaoHandle;
    };

    struct CmdDrawTriangles {
        s32 firstVertex;
        s32 vertexCount;
    };

    struct CmdDrawIndexedTriangles {
        s32 indexCount;
        IndexType indexType;
        u64 indexBufferOffsetBytes;
    };

    // Uniforms
    struct CmdSetUniform1f {
        s32 location;
        float value;
    };

    struct CmdSetUniform3f {
        s32 location;
        float x, y, z;
    };

    // UI
    struct CmdImguiRender {
        void* drawData;
    };

    // Present
    struct CmdSwapBuffers {
        void* nativeSwapContext;
    };

    // The Variant
    using RenderCommand = std::variant<
        CmdBegin,
        CmdEnd,
        CmdExecute,
        CmdReset,
        CmdBindFramebuffer,
        CmdSetViewport,
        CmdClearColor,
        CmdClear,
        CmdEnableDepthTest,
        CmdUseProgram,
        CmdBindVertexArray,
        CmdDrawTriangles,
        CmdDrawIndexedTriangles,
        CmdSetUniform1f,
        CmdSetUniform3f,
        CmdImguiRender,
        CmdSwapBuffers
    >;

    // The Command Buffer
    using CommandBuffer = std::vector<RenderCommand>;
}
