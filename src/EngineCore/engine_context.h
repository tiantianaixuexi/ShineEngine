#pragma once

#include "subsystem.h"
#include <unordered_map>
#include <string_view>
#include <type_traits>

namespace shine
{
    // FNV-1a Compile-time Hash
    constexpr size_t HashString(const char* str)
    {
        size_t hash = 14695981039346656037ull;
        while (*str)
        {
            hash ^= static_cast<size_t>(*str++);
            hash *= 1099511628211ull;
        }
        return hash;
    }

    class EngineContext
    {
    public:
        template<typename T>
        T* Get() const
        {
            static_assert(requires { T::GetStaticID(); }, "Subsystem must define static constexpr size_t GetStaticID()");
            
            auto it = m_systems.find(T::GetStaticID());
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
            static_assert(requires { T::GetStaticID(); }, "Subsystem must define static constexpr size_t GetStaticID()");
            
            m_systems[T::GetStaticID()] = system;
        }

        template<typename T>
        void Unregister()
        {
            static_assert(requires { T::GetStaticID(); }, "Subsystem must define static constexpr size_t GetStaticID()");
            m_systems.erase(T::GetStaticID());
        }

    private:
        std::unordered_map<size_t, Subsystem*> m_systems;
    };
}
