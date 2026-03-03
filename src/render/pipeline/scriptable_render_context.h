#pragma once

#include "command_buffer.h"  // 完整定义，因为 std::vector<CommandBuffer> 需要大小
#include <functional>
#include <vector>

namespace shine::render
{
    class RenderingData;

    /**
     * @brief 可编程渲染上下文（类似 Unity ScriptableRenderContext）
     * 记录和提交渲染命令，支持延迟执行和批处理
     */
    class ScriptableRenderContext
    {
    public:
        ScriptableRenderContext() = default;
        ~ScriptableRenderContext();

        // 禁止拷贝，允许移动
        ScriptableRenderContext(const ScriptableRenderContext&) = delete;
        ScriptableRenderContext& operator=(const ScriptableRenderContext&) = delete;
        ScriptableRenderContext(ScriptableRenderContext&&) noexcept = default;
        ScriptableRenderContext& operator=(ScriptableRenderContext&&) noexcept = default;

        /**
         * @brief 提交命令缓冲区 (move semantics — 避免深拷贝)
         */
        void Submit(CommandBuffer&& cmdBuffer);

        /**
         * @brief 提交命令缓冲区 (从指针，拷贝版本保留兼容性)
         */
        void Submit(CommandBuffer* cmdBuffer);

        /** 执行所有提交的命令 */
        void Execute();

        /** 清空所有待执行的命令 */
        void Clear();
        void Reserve(size_t count);

        /** 设置执行回调 — 使用 std::move_only_function (C++23) */
        void SetExecuteCallback(std::move_only_function<void(CommandBuffer*)> callback);

        [[nodiscard]]
        size_t GetPendingCommandCount() const { return m_CommandBuffers.size(); }

    private:
        std::vector<CommandBuffer> m_CommandBuffers;
        std::move_only_function<void(CommandBuffer*)> m_ExecuteCallback;
    };
}
