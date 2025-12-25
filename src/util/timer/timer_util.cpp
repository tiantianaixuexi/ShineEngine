#include "timer_util.h"


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif




namespace shine::util{

    template<typename T>
    T get_now_ms_platform()
    {
#if defined(_WIN32)

        static LARGE_INTEGER freq = []()
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f;
        }();

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return static_cast<T>(now.QuadPart * 1000 / freq.QuadPart);

#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<T>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
#endif
        }

    template<typename T>
    T now_ns() {

#if defined(_WIN32)
        static LARGE_INTEGER freq = []() {LARGE_INTEGER f;QueryPerformanceFrequency(&f); return f; }();
        LARGE_INTEGER now; QueryPerformanceCounter(&now);
        return static_cast<T>(1000000000ull * now.QuadPart / freq.QuadPart);
#else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<T>(ts.tv_sec) * 1000000000ull + ts.tv_nsec;
#endif
    }




}


