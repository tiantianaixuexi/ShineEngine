#include "scriptable_render_context.h"
#include "command_buffer.h"

namespace shine::render
{
    ScriptableRenderContext::~ScriptableRenderContext()
    {
        Clear();
    }

    void ScriptableRenderContext::Submit(CommandBuffer&& cmdBuffer)
    {
        m_CommandBuffers.push_back(std::move(cmdBuffer));
    }

    void ScriptableRenderContext::Submit(CommandBuffer* cmdBuffer)
    {
        if (cmdBuffer)
        {
            // Move from the source — caller should not use the buffer after Submit
            m_CommandBuffers.push_back(std::move(*cmdBuffer));
        }
    }

    void ScriptableRenderContext::Execute()
    {
        if (m_ExecuteCallback)
        {
            for (auto& cmdBuffer : m_CommandBuffers)
            {
                m_ExecuteCallback(&cmdBuffer);
            }
        }
        Clear();
    }

    void ScriptableRenderContext::Clear()
    {
        m_CommandBuffers.clear();
    }

    void ScriptableRenderContext::Reserve(size_t count)
    {
        if (count > m_CommandBuffers.capacity())
        {
            m_CommandBuffers.reserve(count);
        }
    }

    void ScriptableRenderContext::SetExecuteCallback(std::move_only_function<void(CommandBuffer*)> callback)
    {
        m_ExecuteCallback = std::move(callback);
    }
}
