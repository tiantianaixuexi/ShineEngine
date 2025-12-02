#pragma once

#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <fmt/format.h>

namespace shine::benchmark {

struct BenchmarkResult {
    std::string name;
    double average_time_ns;
    double min_time_ns;
    double max_time_ns;
    double total_time_ms;
    double median_time_ns;
    double std_deviation_ns;
    size_t iterations;
    size_t warmup_iterations;
};

class Benchmark {
public:
    Benchmark(std::string name, size_t iterations = 1000, size_t warmup_iterations = 100)
        : name_(std::move(name)), iterations_(iterations), warmup_iterations_(warmup_iterations) {}

    template<typename Func>
    BenchmarkResult run(Func&& func) {
        // 预热阶段
        for (size_t i = 0; i < warmup_iterations_; ++i) {
            func();
        }

        std::vector<double> times;
        times.reserve(iterations_);

        auto total_start = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < iterations_; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            times.push_back(static_cast<double>(duration.count()));
        }

        auto total_end = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start);

        // 计算统计数据
        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double avg_time = sum / static_cast<double>(iterations_);
        double min_time = *std::min_element(times.begin(), times.end());
        double max_time = *std::max_element(times.begin(), times.end());

        // 计算中位数
        std::vector<double> sorted_times = times;
        std::sort(sorted_times.begin(), sorted_times.end());
        double median_time = sorted_times[iterations_ / 2];

        // 计算标准差
        double sq_sum = std::inner_product(times.begin(), times.end(), times.begin(), 0.0);
        double std_dev = std::sqrt(sq_sum / iterations_ - avg_time * avg_time);

        return BenchmarkResult{
            name_,
            avg_time,
            min_time,
            max_time,
            static_cast<double>(total_duration.count()),
            median_time,
            std_dev,
            iterations_,
            warmup_iterations_
        };
    }

private:
    std::string name_;
    size_t iterations_;
    size_t warmup_iterations_;
};

inline void print_result(const BenchmarkResult& result) {
    double ops_per_sec = (static_cast<double>(result.iterations) / result.total_time_ms) * 1000.0;
    double cv = (result.std_deviation_ns / result.average_time_ns) * 100.0;

    fmt::println(" 测试项目: {}", result.name);
    fmt::println("   迭代次数: {} (预热: {})", result.iterations, result.warmup_iterations);
    fmt::println("   平均时间: {:.2f} ns", result.average_time_ns);
    fmt::println("   中位数时间: {:.2f} ns", result.median_time_ns);
    fmt::println("   最短时间: {:.2f} ns", result.min_time_ns);
    fmt::println("   最长时间: {:.2f} ns", result.max_time_ns);
    fmt::println("   标准差: {:.2f} ns", result.std_deviation_ns);
    fmt::println("   总时间: {:.2f} ms", result.total_time_ms);
    fmt::println("   操作频率: {:.0f} 次/秒", ops_per_sec);
    fmt::println("   性能稳定性: {:.2f}% (变异系数)", cv);
}

template<typename Func>
BenchmarkResult run_benchmark(const std::string& name, Func&& func,
                             size_t iterations = 1000, size_t warmup_iterations = 100) {
    Benchmark bench(name, iterations, warmup_iterations);
    auto result = bench.run(std::forward<Func>(func));
    print_result(result);
    return result;
}

} // namespace shine::benchmark
