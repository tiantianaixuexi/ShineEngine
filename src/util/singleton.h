#pragma once

namespace shine::util
{
    template <typename T>
    class Singleton
    {
    public:
		static T& get() noexcept
        {
            static T instance;
            return instance;
        }


    protected:

        constexpr Singleton() noexcept = default;
        ~Singleton() = default;

    private:

        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton&&) = delete;
    };
}

