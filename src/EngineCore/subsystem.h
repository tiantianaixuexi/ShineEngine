#pragma once


#include <string_view>

namespace shine
{
    class EngineContext;

    constexpr size_t HashString(std::string_view str)
    {
        size_t hash = 14695981039346656037ull;
        for (char c : str)
        {
            hash ^= static_cast<size_t>(static_cast<unsigned char>(c));
            hash *= 1099511628211ull;
        }
        return hash;
    }

    constexpr size_t HashString(const char* str)
    {
        size_t hash = 14695981039346656037ull;
        while (*str)
        {
            hash ^= static_cast<size_t>(static_cast<unsigned char>(*str++));
            hash *= 1099511628211ull;
        }
        return hash;
    }

    namespace detail
    {
        template <typename T>
        consteval std::string_view TypeIdString()
        {
#if defined(_MSC_VER)
            return std::string_view{ __FUNCSIG__ };
#else
            return std::string_view{ __PRETTY_FUNCTION__ };
#endif
        }
    }

    template <typename T>
    consteval size_t GetStaticID()
    {
        return HashString(detail::TypeIdString<T>());
    }

    class Subsystem
    {
    public:
        virtual ~Subsystem() = default;
        virtual bool Init(EngineContext& ctx) { return true; }
        virtual void Shutdown(EngineContext& ctx) {}
    };
}
