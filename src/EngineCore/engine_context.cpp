#include "engine_context.h"

namespace shine
{
    EngineContext* EngineContext::s_Instance = nullptr;

    bool EngineContext::InitAll()
    {
        for (size_t id : m_systemOrder)
        {
            auto it = m_systems.find(id);
            if (it == m_systems.end())
            {
                continue;
            }

            Subsystem* sys = it->second;
            if (!sys)
            {
                continue;
            }

            if (!sys->Init(*this))
            {
                return false;
            }
        }
        return true;
    }

    void EngineContext::ShutdownAll() noexcept
    {
        if (m_isShutdown)
        {
            return;
        }
        m_isShutdown = true;

        for (auto it = m_systemOrder.rbegin(); it != m_systemOrder.rend(); ++it)
        {
            const size_t id = *it;
            auto found = m_systems.find(id);
            if (found == m_systems.end())
            {
                continue;
            }

            Subsystem* sys = found->second;
            if (sys)
            {
                sys->Shutdown(*this);
            }
        }
    }

    EngineContext::EngineContext()
    {
        s_Instance = this;
    }

    EngineContext::~EngineContext()
    {
        ShutdownAll();

        if (s_Instance == this)
        {
            s_Instance = nullptr;
        }
        
        for (auto& pair : m_systems)
        {
            delete pair.second;
        }
        m_systems.clear();
    }
}
