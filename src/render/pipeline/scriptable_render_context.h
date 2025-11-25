#pragma once

#include "shine_define.h"
#include "command_buffer.h"  // 需要完整定义，因为 std::vector<CommandBuffer> 需要知道大小
#include <vector>
#include <memory>
#include <functional>

namespace shine::render
{
    class RenderingData;

    /**
     * @brief 可编程渲染上下文（类似 Unity ScriptableRenderContext）
     * 用于记录和提交渲染命令，支持延迟执行和批处理
     */
    class ScriptableRenderContext
    {
    public:
        ScriptableRenderContext();
        ~ScriptableRenderContext();

        /**
         * @brief 提交命令缓冲区
         * @param cmdBuffer 命令缓冲区
         */
        void Submit(CommandBuffer* cmdBuffer);

        /**
         * @brief 执行所有提交的命令
         */
        void Execute();

        /**
         * @brief 清空所有待执行的命令
         */
        void Clear();

        /**
         * @brief 设置执行回调（用于实际执行命令）
         */
        void SetExecuteCallback(std::function<void(CommandBuffer*)> callback);

        /**
         * @brief 获取待执行的命令缓冲区数量
         */
        size_t GetPendingCommandCount() const { return m_CommandBuffers.size(); }

    private:
        std::vector<CommandBuffer> m_CommandBuffers;  // 存储副本而不是指针，避免生命周期问题
        std::function<void(CommandBuffer*)> m_ExecuteCallback;
    };
}

