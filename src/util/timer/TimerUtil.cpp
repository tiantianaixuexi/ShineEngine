#include "TimerUtil.h"
#include "fmt/format.h"

namespace shine::util {

template <typename T>
T get_now_ms_platform() {
#if defined(_WIN32)

    static LARGE_INTEGER freq = []() {
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

template <typename T>
T now_ns() {

#if defined(_WIN32)
    static LARGE_INTEGER freq = []() {LARGE_INTEGER f;QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER        now;
    QueryPerformanceCounter(&now);
    return static_cast<T>(1000000000ull * now.QuadPart / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<T>(ts.tv_sec) * 1000000000ull + ts.tv_nsec;
#endif
}

TimeComponents now() {
    TimeComponents tc{};

#if defined(_WIN32) || defined(_WIN64)
    // Windows: 使用 GetLocalTime 获取毫秒
    SYSTEMTIME st;
    GetLocalTime(&st);

    // 转换为 time_t
    std::tm tm{};
    tm.tm_year  = st.wYear - 1900;
    tm.tm_mon   = st.wMonth - 1;
    tm.tm_mday  = st.wDay;
    tm.tm_hour  = st.wHour;
    tm.tm_min   = st.wMinute;
    tm.tm_sec   = st.wSecond;
    tm.tm_isdst = -1;

    tc.seconds      = std::mktime(&tm);
    tc.milliseconds = st.wMilliseconds;
    tc.local_tm     = tm;

#else
    // POSIX: 使用 gettimeofday 或 clock_gettime
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    tc.seconds      = tv.tv_sec;
    tc.milliseconds = tv.tv_usec / 1000; // 微秒转毫秒

    // 转换为本地时间
    localtime_r(&tc.seconds, &tc.local_tm);
#endif
    return tc;
}

// 仅获取秒级时间（截断毫秒）
TimeComponents now_truncated() 
{
        TimeComponents tc = now();
        tc.milliseconds = 0;
        return tc;
}

// 格式化为字符串（秒级精度）
std::string format_seconds(const TimeComponents &tc) {
    return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
                       tc.local_tm.tm_year + 1900,
                       tc.local_tm.tm_mon + 1,
                       tc.local_tm.tm_mday,
                       tc.local_tm.tm_hour,
                       tc.local_tm.tm_min,
                       tc.local_tm.tm_sec);
}

// 格式化为字符串（毫秒级精度）
std::string format_milliseconds(const TimeComponents &tc) {
    return fmt::format(FMT_STRING("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}"),
                       tc.local_tm.tm_year + 1900,
                       tc.local_tm.tm_mon + 1,
                       tc.local_tm.tm_mday,
                       tc.local_tm.tm_hour,
                       tc.local_tm.tm_min,
                       tc.local_tm.tm_sec,
                       tc.milliseconds);
}

template float              get_now_ms_platform<float>();
template double             get_now_ms_platform<double>();
template unsigned long long get_now_ms_platform<unsigned long long>();

template float              now_ns<float>();
template double             now_ns<double>();
template unsigned long long now_ns<unsigned long long>();

} // namespace shine::util
