#include "command_buffer.h"

namespace shine::render
{
    // using namespace shine::render::command; // Remove this to avoid ambiguity

    CommandBuffer::CommandBuffer()
    {
        m_Commands.reserve(128);
    }

    CommandBuffer::~CommandBuffer()
    {
        Clear();
    }

    void CommandBuffer::Clear()
    {
        m_Commands.clear();
        m_HasViewport = false;
        m_HasClearColor = false;
        m_HasDepthTest = false;
        m_HasFramebuffer = false;
        m_HasProgram = false;
        m_HasVao = false;
        m_HasTextureBinding.fill(false);
    }

    void CommandBuffer::SetViewport(s32 x, s32 y, s32 width, s32 height)
    {
        const command::CmdSetViewport cmd{ x, y, width, height };
        if (m_HasViewport &&
            m_LastViewport.x == cmd.x &&
            m_LastViewport.y == cmd.y &&
            m_LastViewport.width == cmd.width &&
            m_LastViewport.height == cmd.height)
        {
            return;
        }
        m_HasViewport = true;
        m_LastViewport = cmd;
        PushRaw(command::CommandOpcode::SetViewport, cmd);
    }

    void CommandBuffer::SetClearColor(float r, float g, float b, float a)
    {
        const command::CmdClearColor cmd{ r, g, b, a };
        if (m_HasClearColor &&
            m_LastClearColor.r == cmd.r &&
            m_LastClearColor.g == cmd.g &&
            m_LastClearColor.b == cmd.b &&
            m_LastClearColor.a == cmd.a)
        {
            return;
        }
        m_HasClearColor = true;
        m_LastClearColor = cmd;
        PushRaw(command::CommandOpcode::ClearColor, cmd);
    }

    void CommandBuffer::ClearRenderTarget(bool clearColor, bool clearDepth)
    {
        const command::CmdClear cmd{ clearColor, clearDepth };
        PushRaw(command::CommandOpcode::Clear, cmd);
    }

    void CommandBuffer::BindFramebuffer(u64 framebufferHandle)
    {
        const command::CmdBindFramebuffer cmd{ framebufferHandle };
        if (m_HasFramebuffer && m_LastFramebufferHandle == cmd.framebufferHandle) return;
        m_HasFramebuffer = true;
        m_LastFramebufferHandle = cmd.framebufferHandle;
        PushRaw(command::CommandOpcode::BindFramebuffer, cmd);
    }

    void CommandBuffer::BindTexture(u32 unit, u32 textureHandle)
    {
        const command::CmdBindTexture cmd{ unit, textureHandle };
        if (cmd.unit < kTextureStateSlots &&
            m_HasTextureBinding[cmd.unit] &&
            m_LastTextureHandle[cmd.unit] == cmd.textureHandle)
        {
            return;
        }
        if (cmd.unit < kTextureStateSlots)
        {
            m_HasTextureBinding[cmd.unit] = true;
            m_LastTextureHandle[cmd.unit] = cmd.textureHandle;
        }
        PushRaw(command::CommandOpcode::BindTexture, cmd);
    }

    void CommandBuffer::EnableDepthTest(bool enabled)
    {
        const command::CmdEnableDepthTest cmd{ enabled };
        if (m_HasDepthTest && m_LastDepthTest == cmd.enabled) return;
        m_HasDepthTest = true;
        m_LastDepthTest = cmd.enabled;
        PushRaw(command::CommandOpcode::EnableDepthTest, cmd);
    }

    void CommandBuffer::UseProgram(u64 programHandle)
    {
        const command::CmdUseProgram cmd{ programHandle };
        if (m_HasProgram && m_LastProgramHandle == cmd.programHandle) return;
        m_HasProgram = true;
        m_LastProgramHandle = cmd.programHandle;
        PushRaw(command::CommandOpcode::UseProgram, cmd);
    }

    void CommandBuffer::BindVertexArray(u64 vaoHandle)
    {
        const command::CmdBindVertexArray cmd{ vaoHandle };
        if (m_HasVao && m_LastVaoHandle == cmd.vaoHandle) return;
        m_HasVao = true;
        m_LastVaoHandle = cmd.vaoHandle;
        PushRaw(command::CommandOpcode::BindVertexArray, cmd);
    }

    void CommandBuffer::DrawTriangles(s32 firstVertex, s32 vertexCount)
    {
        const command::CmdDrawTriangles cmd{ firstVertex, vertexCount };
        PushRaw(command::CommandOpcode::DrawTriangles, cmd);
    }

    void CommandBuffer::DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes)
    {
        const command::CmdDrawIndexedTriangles cmd{ indexCount, indexType, indexBufferOffsetBytes };
        PushRaw(command::CommandOpcode::DrawIndexedTriangles, cmd);
    }

    void CommandBuffer::SetUniform1f(s32 location, float value)
    {
        const command::CmdSetUniform1f cmd{ location, value };
        PushRaw(command::CommandOpcode::SetUniform1f, cmd);
    }

    void CommandBuffer::SetUniform1i(s32 location, s32 value)
    {
        const command::CmdSetUniform1i cmd{ location, value };
        PushRaw(command::CommandOpcode::SetUniform1i, cmd);
    }

    void CommandBuffer::SetUniform2f(s32 location, float x, float y)
    {
        const command::CmdSetUniform2f cmd{ location, x, y };
        PushRaw(command::CommandOpcode::SetUniform2f, cmd);
    }

    void CommandBuffer::SetUniform3f(s32 location, float x, float y, float z)
    {
        const command::CmdSetUniform3f cmd{ location, x, y, z };
        PushRaw(command::CommandOpcode::SetUniform3f, cmd);
    }

    void CommandBuffer::SetUniform4f(s32 location, float x, float y, float z, float w)
    {
        const command::CmdSetUniform4f cmd{ location, x, y, z, w };
        PushRaw(command::CommandOpcode::SetUniform4f, cmd);
    }

    void CommandBuffer::SetUniformMatrix4fv(s32 location, const std::array<float, 16>& data, bool transpose)
    {
        const command::CmdSetUniformMatrix4fv cmd{ location, data, transpose };
        PushRaw(command::CommandOpcode::SetUniformMatrix4fv, cmd);
    }

    void CommandBuffer::RenderImGui(void* drawData)
    {
        const command::CmdImguiRender cmd{ drawData };
        PushRaw(command::CommandOpcode::ImguiRender, cmd);
    }

    void CommandBuffer::SwapBuffers(void* nativeSwapContext)
    {
        const command::CmdSwapBuffers cmd{ nativeSwapContext };
        PushRaw(command::CommandOpcode::SwapBuffers, cmd);
    }
}
