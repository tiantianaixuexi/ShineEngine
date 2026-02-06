#include <iostream>
#include <chrono>
#include <vector>
#include <fmt/format.h>

// 简化版本的测试，避免复杂的依赖关系
int main() {
    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║        ShineEngine Simple Performance Test         ║");
    fmt::println("╚════════════════════════════════════════════════════╝");
    fmt::println("");

    // 基本性能测试
    fmt::println("=== 基本性能测试 ===");
    
    // 测试整数运算性能
    auto start = std::chrono::high_resolution_clock::now();
    volatile long long sum = 0;
    for (int i = 0; i < 1000000; ++i) {
        sum += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    fmt::println("整数累加测试 (1M次): {} μs", duration.count());
    fmt::println("计算结果: {}", sum);
    
    // 测试内存分配性能
    fmt::println("\n=== 内存分配测试 ===");
    start = std::chrono::high_resolution_clock::now();
    std::vector<int> vec;
    vec.reserve(10000);
    for (int i = 0; i < 10000; ++i) {
        vec.push_back(i);
    }
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    fmt::println("向量填充测试 (10K元素): {} μs", duration.count());
    fmt::println("向量大小: {}", vec.size());
    
    fmt::println("\n╔════════════════════════════════════════════════════╗");
    fmt::println("║               测试完成                              ║");
    fmt::println("╚════════════════════════════════════════════════════╝");
    
    return 0;
}