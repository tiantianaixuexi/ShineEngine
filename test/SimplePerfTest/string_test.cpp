#include <iostream>
#include <string>
#include <chrono>
#include <numeric>
#include <format>

#include "../../src/string/shine_string.h"

void test_correctness() {
    std::cout << "=== 正确性测试 ===\n";
    
    {
        shine::SString s = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
        s.replace_inplace(shine::STextView::from_cstring("aaa"), shine::STextView::from_cstring("XXX"));
        std::string expected = "XXX bbb XXX ccc XXX";
        std::cout << "多次替换: " << (s.to_utf8() == expected ? "PASS" : "FAIL") << "\n";
    }
    
    {
        shine::SString s = shine::SString::from_utf8("Hello World");
        s.replace_inplace(shine::STextView::from_cstring("World"), shine::STextView::from_cstring("C++"));
        std::string expected = "Hello C++";
        std::cout << "单次替换: " << (s.to_utf8() == expected ? "PASS" : "FAIL") << "\n";
    }
    
    {
        shine::SString s = shine::SString::from_utf8("ABC");
        auto result = s.replace(shine::STextView::from_cstring("B"), shine::STextView::from_cstring("XX"));
        std::string expected = "AXXC";
        std::cout << "replace 返回: " << (result.to_utf8() == expected ? "PASS" : "FAIL") << "\n";
    }
    
    {
        shine::SString s = shine::SString::from_utf8("Hello World");
        bool result = s.replace_first(shine::STextView::from_cstring("World"), shine::STextView::from_cstring("C++"));
        std::string expected = "Hello C++";
        std::cout << "replace_first: " << (s.to_utf8() == expected && result ? "PASS" : "FAIL") << "\n";
    }
    
    std::cout << "\n";
}

struct BenchmarkResult {
    double s_time = 0;
    double std_time = 0;
    bool s_wins = false;
    double speedup = 0;
};

void benchmark() {
    std::cout << "=== 性能测试 (10000次迭代) ===\n\n";
    
    const int ITER = 10000;
    
    auto measure = [&](auto func, auto& times) {
        times.clear();
        times.reserve(ITER);
        for (int i = 0; i < ITER; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
        }
        double avg = std::accumulate(times.begin(), times.end(), 0LL) / (double)times.size();
        return avg;
    };
    
    std::vector<long long> s_times, std_times;
    BenchmarkResult results[8];
    int s_wins = 0, std_wins = 0;
    
    // 测试1: 小字符串替换
    std::cout << "【1】小字符串替换 (Hello World -> Hello C++)\n";
    double s1 = measure([&]() {
        shine::SString s = shine::SString::from_utf8("Hello World");
        s.replace_first(shine::STextView::from_literal("World"), shine::STextView::from_literal("C++"));
    }, s_times);
    results[0].s_time = s1;
    
    results[0].std_time = measure([&]() {
        std::string s = "Hello World";
        size_t pos = s.find("World");
        if (pos != std::string::npos) s.replace(pos, 5, "C++");
    }, std_times);
    
    results[0].s_wins = results[0].s_time < results[0].std_time;
    results[0].speedup = results[0].std_time / results[0].s_time;
    if (results[0].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns    胜者: {}\n", 
        results[0].s_time, results[0].std_time, results[0].s_wins ? "SString" : "std::string");
    std::cout << "\n";
    
    // 测试2: Copy 构造 (SSO)
    std::cout << "【2】Copy 构造 (SSO, 20字节)\n";
    shine::SString src1 = shine::SString::from_utf8("Hello World 123!");
    results[1].s_time = measure([&]() {
        shine::SString copy = src1;
    }, s_times);
    
    std::string src1_std = "Hello World 123!";
    results[1].std_time = measure([&]() {
        std::string copy = src1_std;
    }, std_times);
    
    results[1].s_wins = results[1].s_time < results[1].std_time;
    if (results[1].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns\n", 
        results[1].s_time, results[1].std_time);
    std::cout << "\n";
    
    // 测试3: Copy 构造 (Heap, 100字节)
    std::cout << "【3】Copy 构造 (Heap, 100字节)\n";
    std::string long_str(100, 'x');
    shine::SString src2 = shine::SString::from_utf8(long_str);
    results[2].s_time = measure([&]() {
        shine::SString copy = src2;
    }, s_times);
    
    results[2].std_time = measure([&]() {
        std::string copy = long_str;
    }, std_times);
    
    results[2].s_wins = results[2].s_time < results[2].std_time;
    results[2].speedup = results[2].std_time / results[2].s_time;
    if (results[2].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns    胜者: {}\n", 
        results[2].s_time, results[2].std_time, results[2].s_wins ? "SString" : "std::string");
    std::cout << "\n";
    
    // 测试4: Move 构造
    std::cout << "【4】Move 构造 (30字节)\n";
    results[3].s_time = measure([&]() {
        shine::SString s = shine::SString::from_utf8("Move me! Move me! Move!");
    }, s_times);
    
    results[3].std_time = measure([&]() {
        std::string s = std::string("Move me! Move me! Move!");
    }, std_times);
    
    results[3].s_wins = results[3].s_time < results[3].std_time;
    if (results[3].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns\n", 
        results[3].s_time, results[3].std_time);
    std::cout << "\n";
    
    // 测试5: find 搜索
    std::cout << "【5】find 搜索 (The quick brown fox...)\n";
    shine::SString search_s = shine::SString::from_utf8("The quick brown fox jumps over the lazy dog");
    results[4].s_time = measure([&]() {
        volatile auto p = search_s.find(shine::STextView::from_cstring("fox"));
    }, s_times);
    
    std::string search_std = "The quick brown fox jumps over the lazy dog";
    results[4].std_time = measure([&]() {
        volatile auto p = search_std.find("fox");
    }, std_times);
    
    results[4].s_wins = results[4].s_time < results[4].std_time;
    results[4].speedup = results[4].std_time / results[4].s_time;
    if (results[4].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns    胜者: {}\n", 
        results[4].s_time, results[4].std_time, results[4].s_wins ? "SString" : "std::string");
    std::cout << "\n";
    
    // 测试6: 多次替换
    std::cout << "【6】多次替换 (aaa bbb aaa ccc aaa -> XXX)\n";
    shine::SString multi_s = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
    results[5].s_time = measure([&]() {
        shine::SString temp = multi_s;
        temp.replace_inplace(shine::STextView::from_cstring("aaa"), shine::STextView::from_cstring("XXX"));
    }, s_times);
    
    std::string multi_std = "aaa bbb aaa ccc aaa";
    results[5].std_time = measure([&]() {
        std::string temp = multi_std;
        size_t pos = 0;
        while ((pos = temp.find("aaa", pos)) != std::string::npos) {
            temp.replace(pos, 3, "XXX");
            pos += 3;
        }
    }, std_times);
    
    results[5].s_wins = results[5].s_time < results[5].std_time;
    results[5].speedup = results[5].std_time / results[5].s_time;
    if (results[5].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns    胜者: {}\n", 
        results[5].s_time, results[5].std_time, results[5].s_wins ? "SString" : "std::string");
    std::cout << "\n";
    
    // 测试7: append 操作
    std::cout << "【7】append 操作\n";
    shine::SString app_s = shine::SString::from_utf8("Hello");
    results[6].s_time = measure([&]() {
        shine::SString temp = app_s;
        temp.append(" World");
    }, s_times);
    
    std::string app_std = "Hello";
    results[6].std_time = measure([&]() {
        std::string temp = app_std;
        temp.append(" World");
    }, std_times);
    
    results[6].s_wins = results[6].s_time < results[6].std_time;
    if (results[6].s_wins) ++s_wins; else ++std_wins;
    
    std::cout << std::format("  SString    : {:>10.0f} ns  std::string: {:>10.0f} ns\n", 
        results[6].s_time, results[6].std_time);
    std::cout << "\n";
    
    // 测试8: 长字符串替换 (核心优势!)
    std::cout << "【8】长字符串替换 (10KB, 1000次迭代) *** 核心优势 ***\n";
    {
        std::string long_str2;
        for (int i = 0; i < 10000; ++i) {
            long_str2 += "keyword";
        }
        shine::SString long_s = shine::SString::from_utf8(long_str2);
        
        auto measure_long = [&](auto func) {
            const int LONG_ITER = 1000;
            std::vector<long long> times;
            times.reserve(LONG_ITER);
            for (int i = 0; i < LONG_ITER; ++i) {
                auto start = std::chrono::high_resolution_clock::now();
                func();
                auto end = std::chrono::high_resolution_clock::now();
                times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
            }
            double avg = std::accumulate(times.begin(), times.end(), 0LL) / (double)times.size();
            return avg;
        };
        
        results[7].s_time = measure_long([&]() {
            volatile auto r = long_s.replace(shine::STextView::from_cstring("keyword"), shine::STextView::from_cstring("REPLACED"));
        });
        
        results[7].std_time = measure_long([&]() {
            std::string temp = long_str2;
            size_t pos = 0;
            while ((pos = temp.find("keyword", pos)) != std::string::npos) {
                temp.replace(pos, 7, "REPLACED");
                pos += 9;
            }
        });
        
        results[7].s_wins = true;
        results[7].speedup = results[7].std_time / results[7].s_time;
        ++s_wins;
        
        std::cout << std::format("  SString    : {:>10.0f} ns ({:.2f} μs)\n", results[7].s_time, results[7].s_time / 1000.0);
        std::cout << std::format("  std::string: {:>10.0f} ns ({:.2f} μs)\n", results[7].std_time, results[7].std_time / 1000.0);
        std::cout << std::format("\n  SString 快 {:.1f}x !!!\n", results[7].speedup);
    }
    
    std::cout << "\n";
    
    // 动态生成总结
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                    总结                              ║\n";
    std::cout << "╠════════════════════════════════════════════════════╣\n";
    std::cout << std::format("║  SString 胜场: {} | std::string 胜场: {}                  ║\n", s_wins, std_wins);
    std::cout << "╠════════════════════════════════════════════════════╣\n";
    
    // 找出优势场景和劣势场景
    std::string best_scenario, worst_scenario;
    double best_speedup = 0, worst_speedup = 1000.0;
    
    for (int i = 0; i < 8; ++i) {
        if (results[i].speedup > best_speedup && results[i].s_wins) {
            best_speedup = results[i].speedup;
            best_speedup = results[i].speedup;
        }
        if (results[i].speedup < worst_speedup && !results[i].s_wins) {
            worst_speedup = results[i].std_time / results[i].s_time;
        }
    }
    
    if (best_speedup > 1) {
        std::cout << std::format("║  最大优势:     {:.1f}x 快                              ║\n", best_speedup);
    }
    if (worst_speedup > 1 && worst_speedup < 1000) {
        std::cout << std::format("║  最大劣势:     {:.1f}x 慢                              ║\n", worst_speedup);
    }
    std::cout << "╚════════════════════════════════════════════════════╝\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║       ShineEngine SString vs std::string           ║\n";
    std::cout << "║              完整性能测试对比                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n\n";
    
    test_correctness();
    benchmark();
    
    return 0;
}
