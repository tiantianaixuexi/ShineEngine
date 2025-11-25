#include "scriptable_render_context.h"
#include "command_buffer.h"

namespace shine::render
{
    ScriptableRenderContext::ScriptableRenderContext()
    {
    }

    ScriptableRenderContext::~ScriptableRenderContext()
    {
        Clear();
    }

    void ScriptableRenderContext::Submit(CommandBuffer* cmdBuffer)
    {
        if (cmdBuffer)
        {
            // 存储 CommandBuffer 的副本，避免生命周期问题
            m_CommandBuffers.push_back(*cmdBuffer);
        }
    }

    void ScriptableRenderContext::Execute()
    {
        if (m_ExecuteCallback)
        {
            for (auto& cmdBuffer : m_CommandBuffers)
            {
                // 传递引用，因为现在存储的是对象而不是指针
                m_ExecuteCallback(&cmdBuffer);
            }
        }
        Clear();
    }

    void ScriptableRenderContext::Clear()
    {
        m_CommandBuffers.clear();
    }

    void ScriptableRenderContext::SetExecuteCallback(std::function<void(CommandBuffer*)> callback)
    {
        m_ExecuteCallback = callback;
    }
}

