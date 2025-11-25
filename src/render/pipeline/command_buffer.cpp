#include "command_buffer.h"
#include "render/command/command_list.h"

namespace shine::render
{
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
        Command cmd;
        cmd.type = CommandType::SetViewport;
        cmd.viewport = { x, y, width, height };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::SetClearColor(float r, float g, float b, float a)
    {
        m_ClearColorR = r;
        m_ClearColorG = g;
        m_ClearColorB = b;
        m_ClearColorA = a;
    }

    void CommandBuffer::ClearRenderTarget(bool clearColor, bool clearDepth)
    {
        Command cmd;
        cmd.type = CommandType::ClearRenderTarget;
        cmd.clearTarget = { clearColor, clearDepth };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::BindFramebuffer(u64 framebufferHandle)
    {
        Command cmd;
        cmd.type = CommandType::BindFramebuffer;
        cmd.framebufferHandle = framebufferHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::EnableDepthTest(bool enabled)
    {
        Command cmd;
        cmd.type = CommandType::EnableDepthTest;
        cmd.depthTestEnabled = enabled;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::UseProgram(u64 programHandle)
    {
        Command cmd;
        cmd.type = CommandType::UseProgram;
        cmd.programHandle = programHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::BindVertexArray(u64 vaoHandle)
    {
        Command cmd;
        cmd.type = CommandType::BindVertexArray;
        cmd.vaoHandle = vaoHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::DrawTriangles(s32 firstVertex, s32 vertexCount)
    {
        Command cmd;
        cmd.type = CommandType::DrawTriangles;
        cmd.drawTriangles = { firstVertex, vertexCount };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes)
    {
        Command cmd;
        cmd.type = CommandType::DrawIndexedTriangles;
        cmd.drawIndexed = { indexCount, indexType, indexBufferOffsetBytes };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::RenderImGui(void* drawData)
    {
        Command cmd;
        cmd.type = CommandType::RenderImGui;
        cmd.drawData = drawData;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::SwapBuffers(void* nativeSwapContext)
    {
        Command cmd;
        cmd.type = CommandType::SwapBuffers;
        cmd.swapContext = nativeSwapContext;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::Execute(command::ICommandList& cmdList)
    {
        // 先设置清除颜色
        cmdList.clearColor(m_ClearColorR, m_ClearColorG, m_ClearColorB, m_ClearColorA);

        // 执行所有命令
        for (const auto& cmd : m_Commands)
        {
            switch (cmd.type)
            {
            case CommandType::SetViewport:
                cmdList.setViewport(cmd.viewport.x, cmd.viewport.y, cmd.viewport.width, cmd.viewport.height);
                break;
            case CommandType::SetClearColor:
                cmdList.clearColor(m_ClearColorR, m_ClearColorG, m_ClearColorB, m_ClearColorA);
                break;
            case CommandType::ClearRenderTarget:
                cmdList.clear(cmd.clearTarget.clearColor, cmd.clearTarget.clearDepth);
                break;
            case CommandType::BindFramebuffer:
                cmdList.bindFramebuffer(cmd.framebufferHandle);
                break;
            case CommandType::EnableDepthTest:
                cmdList.enableDepthTest(cmd.depthTestEnabled);
                break;
            case CommandType::UseProgram:
                cmdList.useProgram(cmd.programHandle);
                break;
            case CommandType::BindVertexArray:
                cmdList.bindVertexArray(cmd.vaoHandle);
                break;
            case CommandType::DrawTriangles:
                cmdList.drawTriangles(cmd.drawTriangles.firstVertex, cmd.drawTriangles.vertexCount);
                break;
            case CommandType::DrawIndexedTriangles:
                cmdList.drawIndexedTriangles(cmd.drawIndexed.indexCount, cmd.drawIndexed.indexType, cmd.drawIndexed.offset);
                break;
            case CommandType::RenderImGui:
                cmdList.imguiRender(cmd.drawData);
                break;
            case CommandType::SwapBuffers:
                cmdList.swapBuffers(cmd.swapContext);
                break;
            }
        }
    }
}

