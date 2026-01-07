#include "command_buffer.h"

namespace shine::render
{
    // using namespace shine::render::command; // Remove this to avoid ambiguity

    CommandBuffer::CommandBuffer()
    {
    }

    CommandBuffer::~CommandBuffer()
    {
        Clear();
    }

    void CommandBuffer::Clear()
    {
        m_Commands.clear();
    }

    void CommandBuffer::SetViewport(s32 x, s32 y, s32 width, s32 height)
    {
        m_Commands.push_back(command::CmdSetViewport{ x, y, width, height });
    }

    void CommandBuffer::SetClearColor(float r, float g, float b, float a)
    {
        m_ClearColorR = r;
        m_ClearColorG = g;
        m_ClearColorB = b;
        m_ClearColorA = a;
        // Also push the command so it executes in order
        m_Commands.push_back(command::CmdClearColor{ r, g, b, a });
    }

    void CommandBuffer::ClearRenderTarget(bool clearColor, bool clearDepth)
    {
        m_Commands.push_back(command::CmdClear{ clearColor, clearDepth });
    }

    void CommandBuffer::BindFramebuffer(u64 framebufferHandle)
    {
        m_Commands.push_back(command::CmdBindFramebuffer{ framebufferHandle });
    }

    void CommandBuffer::EnableDepthTest(bool enabled)
    {
        m_Commands.push_back(command::CmdEnableDepthTest{ enabled });
    }

    void CommandBuffer::UseProgram(u64 programHandle)
    {
        m_Commands.push_back(command::CmdUseProgram{ programHandle });
    }

    void CommandBuffer::BindVertexArray(u64 vaoHandle)
    {
        m_Commands.push_back(command::CmdBindVertexArray{ vaoHandle });
    }

    void CommandBuffer::DrawTriangles(s32 firstVertex, s32 vertexCount)
    {
        m_Commands.push_back(command::CmdDrawTriangles{ firstVertex, vertexCount });
    }

    void CommandBuffer::DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes)
    {
        m_Commands.push_back(command::CmdDrawIndexedTriangles{ indexCount, indexType, indexBufferOffsetBytes });
    }

    void CommandBuffer::SetUniform1f(s32 location, float value)
    {
        m_Commands.push_back(command::CmdSetUniform1f{ location, value });
    }

    void CommandBuffer::SetUniform3f(s32 location, float x, float y, float z)
    {
        m_Commands.push_back(command::CmdSetUniform3f{ location, x, y, z });
    }

    void CommandBuffer::RenderImGui(void* drawData)
    {
        m_Commands.push_back(command::CmdImguiRender{ drawData });
    }

    void CommandBuffer::SwapBuffers(void* nativeSwapContext)
    {
        m_Commands.push_back(command::CmdSwapBuffers{ nativeSwapContext });
    }
}
