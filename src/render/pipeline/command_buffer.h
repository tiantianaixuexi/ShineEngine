#pragma once

#include "shine_define.h"
#include "render/command/command_list.h"
#include <vector>
#include <memory>

namespace shine::render
{
    /**
     * @brief 命令缓冲区（类似 Unity CommandBuffer）
     * 记录渲染命令，可以提交到 ScriptableRenderContext
     */
    class CommandBuffer
    {
    public:
        CommandBuffer();
        ~CommandBuffer();

        /**
         * @brief 清空命令缓冲区
         */
        void Clear();

        /**
         * @brief 设置视口
         */
        void SetViewport(s32 x, s32 y, s32 width, s32 height);

        /**
         * @brief 设置清除颜色
         */
        void SetClearColor(float r, float g, float b, float a);

        /**
         * @brief 清除缓冲区
         */
        void ClearRenderTarget(bool clearColor, bool clearDepth);

        /**
         * @brief 绑定帧缓冲
         */
        void BindFramebuffer(u64 framebufferHandle);

        /**
         * @brief 启用/禁用深度测试
         */
        void EnableDepthTest(bool enabled);

        /**
         * @brief 使用着色器程序
         */
        void UseProgram(u64 programHandle);

        /**
         * @brief 绑定顶点数组
         */
        void BindVertexArray(u64 vaoHandle);

        /**
         * @brief 绘制三角形
         */
        void DrawTriangles(s32 firstVertex, s32 vertexCount);

        /**
         * @brief 绘制索引三角形
         */
        void DrawIndexedTriangles(s32 indexCount, command::IndexType indexType, u64 indexBufferOffsetBytes = 0);

        /**
         * @brief 渲染 ImGui
         */
        void RenderImGui(void* drawData);

        /**
         * @brief 交换缓冲区
         */
        void SwapBuffers(void* nativeSwapContext);

        /**
         * @brief 执行命令（通过 ICommandList 接口）
         */
        void Execute(command::ICommandList& cmdList);

        /**
         * @brief 获取命令数量
         */
        size_t GetCommandCount() const { return m_Commands.size(); }

    private:
        // 命令类型枚举
        enum class CommandType : u8
        {
            SetViewport,
            SetClearColor,
            ClearRenderTarget,
            BindFramebuffer,
            EnableDepthTest,
            UseProgram,
            BindVertexArray,
            DrawTriangles,
            DrawIndexedTriangles,
            RenderImGui,
            SwapBuffers
        };

        // 命令结构
        struct Command
        {
            CommandType type;
            union
            {
                struct { s32 x, y, width, height; } viewport;
                struct { float r, g, b, a; } clearColor;
                struct { bool clearColor, clearDepth; } clearTarget;
                u64 framebufferHandle;
                bool depthTestEnabled;
                u64 programHandle;
                u64 vaoHandle;
                struct { s32 firstVertex, vertexCount; } drawTriangles;
                struct { s32 indexCount; command::IndexType indexType; u64 offset; } drawIndexed;
                void* drawData;
                void* swapContext;
            };
        };

        std::vector<Command> m_Commands;
        float m_ClearColorR = 0.0f;
        float m_ClearColorG = 0.0f;
        float m_ClearColorB = 0.0f;
        float m_ClearColorA = 1.0f;
    };
}

