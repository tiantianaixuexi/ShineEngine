#pragma once

#include "shine_define.h"
#include "render/command/render_commands.h"
#include <vector>

namespace shine::render
{
    /**
     * @brief Command buffer using std::variant for static polymorphism
     */
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
        void EnableDepthTest(bool enabled);
        void UseProgram(u64 programHandle);
        void BindVertexArray(u64 vaoHandle);
        void DrawTriangles(s32 firstVertex, s32 vertexCount);
        void DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes = 0);
        void SetUniform1f(s32 location, float value);
        void SetUniform3f(s32 location, float x, float y, float z);
        void RenderImGui(void* drawData);
        void SwapBuffers(void* nativeSwapContext);

        // Access the underlying vector of variants
        const command::CommandBuffer& GetCommands() const { return m_Commands; }
        size_t GetCommandCount() const { return m_Commands.size(); }

    private:
        command::CommandBuffer m_Commands;
        
        // Cache clear color to push it when needed or just push state changes?
        // Original code pushed a ClearColor command when SetClearColor was called? 
        // No, original SetClearColor just set member vars. Execute() called cmdList.clearColor at start and when CommandType::SetClearColor was found.
        // We should probably just push the command immediately.
        float m_ClearColorR = 0.0f;
        float m_ClearColorG = 0.0f;
        float m_ClearColorB = 0.0f;
        float m_ClearColorA = 1.0f;
    };
}
