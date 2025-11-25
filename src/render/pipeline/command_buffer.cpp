#include "command_buffer.h"
#include "render/command/command_list.h"
#include <imgui.h>  // 用于 ImGui::GetDrawData()

namespace shine::render
{
    // CommandBufferAdapter 实现
    CommandBufferAdapter::CommandBufferAdapter(CommandBuffer& buffer) : m_Buffer(buffer)
    {
    }

    void CommandBufferAdapter::begin() {}
    void CommandBufferAdapter::end() {}
    void CommandBufferAdapter::execute() {}
    void CommandBufferAdapter::reset() { m_Buffer.Clear(); }

    void CommandBufferAdapter::bindFramebuffer(u64 framebufferHandle) { m_Buffer.BindFramebuffer(framebufferHandle); }
    void CommandBufferAdapter::setViewport(s32 x, s32 y, s32 width, s32 height) { m_Buffer.SetViewport(x, y, width, height); }
    void CommandBufferAdapter::clearColor(float r, float g, float b, float a) { m_Buffer.SetClearColor(r, g, b, a); }
    void CommandBufferAdapter::clear(bool clearColorBuffer, bool clearDepthBuffer) { m_Buffer.ClearRenderTarget(clearColorBuffer, clearDepthBuffer); }
    void CommandBufferAdapter::enableDepthTest(bool enabled) { m_Buffer.EnableDepthTest(enabled); }
    void CommandBufferAdapter::useProgram(u64 programHandle) { m_Buffer.UseProgram(programHandle); }
    void CommandBufferAdapter::bindVertexArray(u64 vaoHandle) { m_Buffer.BindVertexArray(vaoHandle); }
    void CommandBufferAdapter::drawTriangles(s32 firstVertex, s32 vertexCount) { m_Buffer.DrawTriangles(firstVertex, vertexCount); }
    void CommandBufferAdapter::drawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes)
    {
        m_Buffer.DrawIndexedTriangles(indexCount, indexType, indexBufferOffsetBytes);
    }
    void CommandBufferAdapter::setUniform1f(s32 location, float value) { m_Buffer.SetUniform1f(location, value); }
    void CommandBufferAdapter::setUniform3f(s32 location, float x, float y, float z) { m_Buffer.SetUniform3f(location, x, y, z); }
    void CommandBufferAdapter::imguiRender(void* drawData) { m_Buffer.RenderImGui(drawData); }
    void CommandBufferAdapter::swapBuffers(void* nativeSwapContext) { m_Buffer.SwapBuffers(nativeSwapContext); }

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
        Command cmd{};  // 确保 union 被正确初始化
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
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::ClearRenderTarget;
        cmd.clearTarget = { clearColor, clearDepth };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::BindFramebuffer(u64 framebufferHandle)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::BindFramebuffer;
        cmd.framebufferHandle = framebufferHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::EnableDepthTest(bool enabled)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::EnableDepthTest;
        cmd.depthTestEnabled = enabled;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::UseProgram(u64 programHandle)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::UseProgram;
        cmd.programHandle = programHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::BindVertexArray(u64 vaoHandle)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::BindVertexArray;
        cmd.vaoHandle = vaoHandle;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::DrawTriangles(s32 firstVertex, s32 vertexCount)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::DrawTriangles;
        cmd.drawTriangles = { firstVertex, vertexCount };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::DrawIndexedTriangles;
        cmd.drawIndexed = { indexCount, indexType, indexBufferOffsetBytes };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::SetUniform1f(s32 location, float value)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::SetUniform1f;
        cmd.uniform1f = { location, value };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::SetUniform3f(s32 location, float x, float y, float z)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::SetUniform3f;
        cmd.uniform3f = { location, x, y, z };
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::RenderImGui(void* drawData)
    {
        Command cmd{};  // 确保 union 被正确初始化
        cmd.type = CommandType::RenderImGui;
        cmd.drawData = drawData;
        m_Commands.push_back(cmd);
    }

    void CommandBuffer::SwapBuffers(void* nativeSwapContext)
    {
        Command cmd{};
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
                // 安全检查：确保 viewport 数据有效（允许负坐标，但宽高必须为正）
                if (cmd.viewport.width > 0 && cmd.viewport.height > 0)
                {
                    cmdList.setViewport(cmd.viewport.x, cmd.viewport.y, cmd.viewport.width, cmd.viewport.height);
                }
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
            case CommandType::SetUniform1f:
                cmdList.setUniform1f(cmd.uniform1f.location, cmd.uniform1f.value);
                break;
            case CommandType::SetUniform3f:
                cmdList.setUniform3f(cmd.uniform3f.location, cmd.uniform3f.x, cmd.uniform3f.y, cmd.uniform3f.z);
                break;
            case CommandType::RenderImGui:
                // 重要：ImGui::GetDrawData() 返回的指针只在当前帧有效
                // 如果命令被延迟执行（通过 CommandBuffer），存储的 drawData 指针可能已经失效
                // 因此在这里重新获取最新的 drawData，而不是使用存储的指针
                // 注意：这要求 ImGui::Render() 必须在 Execute() 之前被调用
                {
                    ImDrawData* currentDrawData = ImGui::GetDrawData();
                    if (currentDrawData)
                    {
                        cmdList.imguiRender(currentDrawData);
                    }
                    // 如果 currentDrawData 为空，说明 ImGui::Render() 还没有被调用，这是调用顺序问题
                }
                break;
            case CommandType::SwapBuffers:
                cmdList.swapBuffers(cmd.swapContext);
                break;
            }
        }
    }
}

