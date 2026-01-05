#include "engine_context.h"

namespace shine
{
    EngineContext* EngineContext::s_Instance = nullptr;

    EngineContext::EngineContext()
    {
        s_Instance = this;
    }

    EngineContext::~EngineContext()
    {
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
