#pragma once

#include "util/shine_define.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>

namespace shine::render::command
{
    template <typename... Ts>
    constexpr size_t MaxSizeOf()
    {
        size_t maxSize = 0;
        ((maxSize = maxSize < sizeof(Ts) ? sizeof(Ts) : maxSize), ...);
        return maxSize;
    }

    template <typename... Ts>
    constexpr size_t MaxAlignOf()
    {
        size_t maxAlign = 0;
        ((maxAlign = maxAlign < alignof(Ts) ? alignof(Ts) : maxAlign), ...);
        return maxAlign;
    }

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

    struct CmdBindTexture {
        u32 unit;
        u32 textureHandle;
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

    struct CmdSetUniform1i {
        s32 location;
        s32 value;
    };

    struct CmdSetUniform2f {
        s32 location;
        float x, y;
    };

    struct CmdSetUniform3f {
        s32 location;
        float x, y, z;
    };

    struct CmdSetUniform4f {
        s32 location;
        float x, y, z, w;
    };

    struct CmdSetUniformMatrix4fv {
        s32 location;
        std::array<float, 16> data;
        bool transpose;
    };

    // UI
    struct CmdImguiRender {
        void* drawData;
    };

    // Present
    struct CmdSwapBuffers {
        void* nativeSwapContext;
    };

    enum class CommandOpcode : u8
    {
        Begin,
        End,
        Execute,
        Reset,
        BindFramebuffer,
        BindTexture,
        SetViewport,
        ClearColor,
        Clear,
        EnableDepthTest,
        UseProgram,
        BindVertexArray,
        DrawTriangles,
        DrawIndexedTriangles,
        SetUniform1f,
        SetUniform1i,
        SetUniform2f,
        SetUniform3f,
        SetUniform4f,
        SetUniformMatrix4fv,
        ImguiRender,
        SwapBuffers,
        Count
    };

    constexpr size_t kRawCommandPayloadSize = MaxSizeOf<
        CmdBegin,
        CmdEnd,
        CmdExecute,
        CmdReset,
        CmdBindFramebuffer,
        CmdBindTexture,
        CmdSetViewport,
        CmdClearColor,
        CmdClear,
        CmdEnableDepthTest,
        CmdUseProgram,
        CmdBindVertexArray,
        CmdDrawTriangles,
        CmdDrawIndexedTriangles,
        CmdSetUniform1f,
        CmdSetUniform1i,
        CmdSetUniform2f,
        CmdSetUniform3f,
        CmdSetUniform4f,
        CmdSetUniformMatrix4fv,
        CmdImguiRender,
        CmdSwapBuffers
    >();

    constexpr size_t kRawCommandPayloadAlign = MaxAlignOf<
        CmdBegin,
        CmdEnd,
        CmdExecute,
        CmdReset,
        CmdBindFramebuffer,
        CmdBindTexture,
        CmdSetViewport,
        CmdClearColor,
        CmdClear,
        CmdEnableDepthTest,
        CmdUseProgram,
        CmdBindVertexArray,
        CmdDrawTriangles,
        CmdDrawIndexedTriangles,
        CmdSetUniform1f,
        CmdSetUniform1i,
        CmdSetUniform2f,
        CmdSetUniform3f,
        CmdSetUniform4f,
        CmdSetUniformMatrix4fv,
        CmdImguiRender,
        CmdSwapBuffers
    >();

    struct RawRenderCommand
    {
        CommandOpcode opcode{};
        alignas(kRawCommandPayloadAlign) std::array<std::byte, kRawCommandPayloadSize> payload{};

        template <typename T>
        static RawRenderCommand Make(CommandOpcode op, const T& data)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= kRawCommandPayloadSize);
            RawRenderCommand cmd;
            cmd.opcode = op;
            std::memcpy(cmd.payload.data(), &data, sizeof(T));
            return cmd;
        }

        template <typename T>
        const T& AsRef() const
        {
            static_assert(std::is_trivially_copyable_v<T>);
            static_assert(sizeof(T) <= kRawCommandPayloadSize);
            static_assert(alignof(T) <= kRawCommandPayloadAlign);
            return *reinterpret_cast<const T*>(payload.data());
        }

        template <typename T>
        T As() const
        {
            return AsRef<T>();
        }
    };

    using CommandBuffer = std::vector<RawRenderCommand>;
}
