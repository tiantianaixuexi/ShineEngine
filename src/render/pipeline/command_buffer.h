#pragma once

#include "shine_define.h"
#include "render/command/render_commands.h"
#include <array>
#include <cassert>
#include <vector>

namespace shine::render
{
    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer();

        void Clear();

        void SetViewport(s32 x, s32 y, s32 width, s32 height);
        void SetClearColor(float r, float g, float b, float a);
        void ClearRenderTarget(bool clearColor, bool clearDepth);
        void BindFramebuffer(u64 framebufferHandle);
        void BindTexture(u32 unit, u32 textureHandle);
        void EnableDepthTest(bool enabled);
        void UseProgram(u64 programHandle);
        void BindVertexArray(u64 vaoHandle);
        void DrawTriangles(s32 firstVertex, s32 vertexCount);
        void DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes = 0);
        void SetUniform1f(s32 location, float value);
        void SetUniform1i(s32 location, s32 value);
        void SetUniform2f(s32 location, float x, float y);
        void SetUniform3f(s32 location, float x, float y, float z);
        void SetUniform4f(s32 location, float x, float y, float z, float w);
        void SetUniformMatrix4fv(s32 location, const std::array<float, 16>& data, bool transpose = false);
        void RenderImGui(void* drawData);
        void SwapBuffers(void* nativeSwapContext);

        const command::CommandBuffer& GetRawCommands() const { return m_Commands; }
        size_t GetCommandCount() const { return m_Commands.size(); }

    private:
        static constexpr size_t kTextureStateSlots = 32;

        template <typename T>
        void PushRaw(command::CommandOpcode opcode, const T& data)
        {
            assert(static_cast<size_t>(opcode) < static_cast<size_t>(command::CommandOpcode::Count));
            m_Commands.push_back(command::RawRenderCommand::Make(opcode, data));
        }

        command::CommandBuffer m_Commands;
        bool m_HasViewport = false;
        command::CmdSetViewport m_LastViewport{};
        bool m_HasClearColor = false;
        command::CmdClearColor m_LastClearColor{};
        bool m_HasDepthTest = false;
        bool m_LastDepthTest = false;
        bool m_HasFramebuffer = false;
        u64 m_LastFramebufferHandle = 0;
        bool m_HasProgram = false;
        u64 m_LastProgramHandle = 0;
        bool m_HasVao = false;
        u64 m_LastVaoHandle = 0;
        std::array<bool, kTextureStateSlots> m_HasTextureBinding{};
        std::array<u32, kTextureStateSlots> m_LastTextureHandle{};
    };
}
