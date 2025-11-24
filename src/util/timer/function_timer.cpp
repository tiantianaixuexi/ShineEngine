#include "function_timer.h"

#include "timer_util.h"

#include "fmt/format.h"

#include <string>
#include <source_location>




namespace shine::util
{
    constexpr  FunctionTimer::FunctionTimer(TimerPrecision precision,  std::source_location location)
        : name_(""),
        func_(location.function_name()),
        precision_(precision)
    {
        if (precision_ == TimerPrecision::Milliseconds) {
            start_ms_ = get_now_ms_platform<float>();
        }
        else {
            start_ns_ = now_ns<float>(); // 需要在timer_util中添加此函数
        }
    }

     FunctionTimer::FunctionTimer(std::string name, TimerPrecision precision,  std::source_location location)
        : name_(std::move(name)),
        func_(location.function_name()),
        precision_(precision)
    {
        if (precision_ == TimerPrecision::Milliseconds) {
            start_ms_ = get_now_ms_platform<float>();
        }
        else {
            start_ns_ = now_ns<float>(); // 需要在timer_util中添加此函数
        }
    }

	FunctionTimer::~FunctionTimer()
    {
        if  (precision_ == TimerPrecision::Milliseconds)
        {
            float end_ms = get_now_ms_platform<float>();
            float duration_ms = end_ms - start_ms_;

            if (!name_.empty()) {
                fmt::println("[Timer] {} {} 执行时间: {} ms", name_, func_, duration_ms);
            }
            else {
                fmt::println("[Timer] {} 执行时间: {} ms", func_, duration_ms);
            }
        }
        else // 纳秒精度
        {
            uint64_t  end_ns = now_ns<uint64_t>();
            uint64_t  duration_ns = end_ns - start_ns_;

            if (!name_.empty()) {
                fmt::println("[Timer] {} {} 执行时间: {} ns {} ms", name_, func_, duration_ns,(double)(duration_ns / 1e6));
            }
            else {
                fmt::println("[Timer] {} 执行时间: {} ns {} ms", func_, duration_ns,(double)(duration_ns / 1e6));
            }
        }
    }
} // namespace shine::util


