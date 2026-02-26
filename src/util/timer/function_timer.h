#pragma once

#include <cstdint>
#include <string>
#include <source_location>

namespace shine::util {

// 定时精度枚举
enum class   TimerPrecision {
    Milliseconds,
    Nanoseconds
};


class  FunctionTimer 
{

public:
    // 构造函数 - 自动获取函数名，默认毫秒精度
    constexpr explicit FunctionTimer(
        TimerPrecision precision = TimerPrecision::Milliseconds,
         std::source_location location = std::source_location::current()
        );

    // 构造函数 - 自定义名称，自动获取函数名，默认毫秒精度
    explicit FunctionTimer(
        std::string name,
        TimerPrecision precision = TimerPrecision::Milliseconds,
         std::source_location location = std::source_location::current()
        );

    // 析构函数 - 自动输出计时结果
	~FunctionTimer();

    // 阻止复制
    FunctionTimer(const FunctionTimer&) = delete;
    FunctionTimer& operator=(const FunctionTimer&) = delete;

private:
    std::string name_; // 定时器名称
    std::string func_; // 函数名称
    float start_ms_; // 开始时间（毫秒，平台相关）
    std::uint64_t start_ns_; // 开始时间（纳秒，平台相关）
    TimerPrecision precision_; // 精度选择
};

}

