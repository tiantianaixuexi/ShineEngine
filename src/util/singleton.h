#pragma once

namespace shine::util
{
    template <typename T>
    class SINGLETON_API Singleton
    {
    public:
        static T& get() noexcept;


    protected:

        constexpr Singleton() noexcept = default;
        ~Singleton() = default;
        
    private:

        static void set_instance(T* ptr) noexcept;

        Singleton(const Singleton&) = delete;
        Singleton& operator=(const Singleton&) = delete;
        Singleton(Singleton&&) = delete;
        Singleton& operator=(Singleton&&) = delete;

    private:
        inline static T* s_instance = nullptr;
        };
}

