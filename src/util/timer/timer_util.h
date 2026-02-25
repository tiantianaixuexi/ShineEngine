#pragma once

#include <ctime>
#include <cstdint>
#include <string>

#include "util/shine_define.h"

#ifdef SHINE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#else

#include <sys/time.h>

#endif


namespace shine::util{

    struct TimeComponents {
        std::tm local_tm;           // 本地时间结构
        std::time_t seconds;        // 秒级时间戳
        int32_t milliseconds;       // 毫秒部分 (0-999)
    };


    TimeComponents now();
    TimeComponents now_truncated();
    std::string format_seconds(const TimeComponents& tc);
    std::string format_milliseconds(const TimeComponents& tc);

    
    template<typename T>
    T get_now_ms_platform();

     template<typename T>
    T now_ns();

    extern  template float get_now_ms_platform<float>();
    extern  template double get_now_ms_platform<double>();
    extern  template unsigned long long get_now_ms_platform<unsigned long long>();

    extern  template float now_ns<float>();
    extern  template double now_ns<double>();
    extern  template unsigned long long now_ns<unsigned long long>();

}
