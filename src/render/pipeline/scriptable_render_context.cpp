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
            m_CommandBuffers.push_back(cmdBuffer);
        }
    }

    void ScriptableRenderContext::Execute()
    {
        if (m_ExecuteCallback)
        {
            for (auto* cmdBuffer : m_CommandBuffers)
            {
                if (cmdBuffer)
                {
                    m_ExecuteCallback(cmdBuffer);
                }
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

