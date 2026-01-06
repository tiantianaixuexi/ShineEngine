#pragma once

#include "subsystem.h"
#include <cstddef>
#include <unordered_map>
#include <string_view>
#include <type_traits>
#include <vector>

namespace shine
{
    class EngineContext
    {
    public:
        EngineContext();
        ~EngineContext();

        static EngineContext& Get() { return *s_Instance; }
        static bool IsInitialized() { return s_Instance != nullptr; }

        bool InitAll();
        void ShutdownAll() noexcept;

        template<typename T>
        T* GetSystem() const
        {
            static_assert(std::is_base_of_v<Subsystem, T>, "T must inherit from Subsystem");
            
            auto it = m_systems.find(shine::GetStaticID<T>());
            if (it != m_systems.end())
            {
                return static_cast<T*>(it->second);
            }
            return nullptr;
        }

        template<typename T>
        void Register(T* system)
        {
            static_assert(std::is_base_of_v<Subsystem, T>, "T must inherit from Subsystem");

            const size_t id = shine::GetStaticID<T>();
            auto it = m_systems.find(id);
            if (it != m_systems.end())
            {
                delete it->second;
                it->second = system;
                return;
            }

            m_systems.emplace(id, system);
            m_systemOrder.push_back(id);
        }

        template<typename T>
        void Unregister()
        {
            static_assert(std::is_base_of_v<Subsystem, T>, "T must inherit from Subsystem");

            const size_t id = shine::GetStaticID<T>();
            auto it = m_systems.find(id);
            if (it != m_systems.end())
            {
                it->second->Shutdown(*this);
                delete it->second;
                m_systems.erase(it);
            }

            for (size_t i = 0; i < m_systemOrder.size(); ++i)
            {
                if (m_systemOrder[i] == id)
                {
                    m_systemOrder.erase(m_systemOrder.begin() + static_cast<std::ptrdiff_t>(i));
                    break;
                }
            }
        }

    private:
        std::unordered_map<size_t, Subsystem*> m_systems;
        std::vector<size_t> m_systemOrder;
        bool m_isShutdown = false;
        static EngineContext* s_Instance;
    };
}
