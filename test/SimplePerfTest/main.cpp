#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <fmt/format.h>

#include "benchmark_framework.h"

// 测试用例声明
void test_string_performance();
void test_memory_allocation();
void test_hash_performance();
void test_reflection_performance();

int main() {
    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║        ShineEngine Simple Performance Test         ║");
    fmt::println("║              简单性能测试套件                       ║");
    fmt::println("╚════════════════════════════════════════════════════╝");
    fmt::println("");

#ifdef TEST_BUILD
    fmt::println("[构建模式] TEST_BUILD 宏已定义");
#endif

    try {
        // 运行各项测试
        test_string_performance();
        test_memory_allocation();
        test_hash_performance();
        test_reflection_performance();

        fmt::println("\n╔════════════════════════════════════════════════════╗");
        fmt::println("║               所有测试完成                          ║");
        fmt::println("╚════════════════════════════════════════════════════╝");

    } catch (const std::exception& e) {
        fmt::println("❌ 测试过程中发生错误: {}", e.what());
        return 1;
    }

    return 0;
}

void test_string_performance() {
    fmt::println("=== 字符串性能测试 ===");
    
    using namespace shine::benchmark;
    
    // 测试 SString vs std::string
    auto result1 = run_benchmark("SString 创建", []() {
        shine::SString s = shine::SString::from_utf8("Hello World Performance Test");
        volatile auto dummy = s.length(); // 防止优化
    }, 10000, 1000);
    
    auto result2 = run_benchmark("std::string 创建", []() {
        std::string s = "Hello World Performance Test";
        volatile auto dummy = s.length(); // 防止优化
    }, 10000, 1000);
    
    fmt::println("");
}

void test_memory_allocation() {
    fmt::println("=== 内存分配性能测试 ===");
    
    using namespace shine::benchmark;
    
    // 测试小对象分配
    auto result1 = run_benchmark("小对象分配 (64字节)", []() {
        auto* ptr = shine::co::Memory::Alloc(64, 8);
        volatile auto dummy = ptr; // 防止优化
        shine::co::Memory::Free(ptr);
    }, 5000, 500);
    
    // 测试大对象分配
    auto result2 = run_benchmark("大对象分配 (1KB)", []() {
        auto* ptr = shine::co::Memory::Alloc(1024, 8);
        volatile auto dummy = ptr; // 防止优化
        shine::co::Memory::Free(ptr);
    }, 1000, 100);
    
    fmt::println("");
}

void test_hash_performance() {
    fmt::println("=== 哈希性能测试 ===");
    
    using namespace shine::benchmark;
    
    // 测试编译期哈希
    constexpr auto compile_time_hash = shine::reflection::ConstexprHash("TestString");
    
    auto result1 = run_benchmark("编译期哈希计算", []() {
        constexpr auto hash = shine::reflection::ConstexprHash("PerformanceTestString");
        volatile auto dummy = hash; // 防止优化
    }, 100000, 10000);
    
    // 测试运行时哈希
    auto result2 = run_benchmark("运行时哈希计算", []() {
        auto hash = shine::reflection::FastHash("RuntimePerformanceTestString");
        volatile auto dummy = hash; // 防止优化
    }, 100000, 10000);
    
    fmt::println("编译期哈希结果: 0x{:08X}", compile_time_hash);
    fmt::println("");
}

void test_reflection_performance() {
    fmt::println("=== 反射系统性能测试 ===");
    
    using namespace shine::benchmark;
    
    // 测试类型ID获取
    auto result1 = run_benchmark("类型ID获取", []() {
        auto type_id = shine::reflection::GetTypeId<int>();
        volatile auto dummy = type_id; // 防止优化
    }, 50000, 5000);
    
    // 测试字符串存储
    auto result2 = run_benchmark("字符串存储", []() {
        const char* stored = shine::reflection::StringMemoryManager::GetInstance().StoreString("ReflectionPerformanceTest");
        volatile auto dummy = stored; // 防止优化
    }, 10000, 1000);
    
    fmt::println("");
}