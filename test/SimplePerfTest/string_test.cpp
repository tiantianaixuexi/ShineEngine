#include <string>
#include <chrono>
#include <numeric>
#include <format>
#include <vector>
#include <unordered_map>
#include <cassert>
#include <type_traits>

#include "../../src/string/shine_string.h"
#include "../../src/string/shine_text.h"
#include "fmt/base.h"
#include "fmt/format.h"

// Prevent compiler from optimizing away a value
template <typename T>
__declspec(noinline) void DoNotOptimize(const T& val) {
    volatile const void* p = &val;
    (void)p;
}

// =========================================================
// Correctness Tests
// =========================================================

static int g_pass = 0, g_fail = 0;

#define CHECK(name, cond) do { \
    if (cond) { fmt::println("  [PASS] {}", name); ++g_pass; } \
    else      { fmt::println("  [FAIL] {}", name); ++g_fail; } \
} while(0)

void test_static_properties() {
    fmt::println("--- Static Properties ---");
    CHECK("sizeof(STextView) == 16", sizeof(shine::STextView) == 16);
    CHECK("sizeof(SString) == 32",   sizeof(shine::SString) == 32);
    CHECK("STextView trivially copyable", std::is_trivially_copyable_v<shine::STextView>);
    CHECK("SString SSO capacity == 30", shine::SString::kSsoMax == 30);

    // Compile-time hash (C++23 constexpr)
    constexpr size_t h = shine::SString::static_hash("entity_position");
    CHECK("static_hash constexpr", h != 0);
    CHECK("static_hash matches runtime",
        h == shine::SString::static_hash("entity_position"));

    fmt::println("");
}

void test_sso() {
    fmt::println("--- SSO Tests ---");

    // Short string fits in SSO
    shine::SString s1("Hello");
    CHECK("SSO: short string size", s1.size() == 5);
    CHECK("SSO: short string content", s1.sv() == "Hello");

    // Max SSO string (30 chars)
    shine::SString s2("123456789012345678901234567890"); // 30 chars
    CHECK("SSO: max SSO string size", s2.size() == 30);
    CHECK("SSO: max SSO content", s2.sv() == "123456789012345678901234567890");
    CHECK("SSO: max SSO capacity", s2.capacity() == 30);

    // Just over SSO limit (31 chars) -> heap
    shine::SString s3("1234567890123456789012345678901"); // 31 chars
    CHECK("Heap: 31-char string size", s3.size() == 31);
    CHECK("Heap: 31-char content", s3.sv() == "1234567890123456789012345678901");

    // Medium string that was heap in old 23-char SSO but now fits SSO
    shine::SString s4("assets/textures/hero.png"); // 24 chars
    CHECK("SSO: 24-char path fits SSO now", s4.size() == 24);
    CHECK("SSO: 24-char path capacity", s4.capacity() == 30);

    // 28-char identifier: was heap before, now SSO
    shine::SString s5("PlayerCharacterAnimCtrl_Idle"); // 28 chars
    CHECK("SSO: 28-char identifier fits SSO", s5.size() == 28);
    CHECK("SSO: 28-char capacity == 30", s5.capacity() == 30);

    fmt::println("");
}

void test_copy_move() {
    fmt::println("--- Copy / Move ---");

    // Copy SSO
    shine::SString orig("Hello SSO");
    shine::SString copy = orig;
    CHECK("Copy SSO: equal", copy.sv() == orig.sv());

    // Copy Heap
    shine::SString big(std::string_view("This is a long string that exceeds SSO capacity limit!"));
    shine::SString big_copy = big;
    CHECK("Copy Heap: equal", big_copy.sv() == big.sv());

    // Move SSO
    shine::SString src("Move SSO");
    shine::SString dst = std::move(src);
    CHECK("Move SSO: dst has data", dst.sv() == "Move SSO");
    CHECK("Move SSO: src is empty", src.empty());

    // Move Heap
    shine::SString src2(std::string_view("This is a very long string for heap move test, it must be over 30!"));
    auto expected = src2.sv();
    shine::SString dst2 = std::move(src2);
    CHECK("Move Heap: dst has data", std::string_view(dst2.sv()) == expected);
    CHECK("Move Heap: src is empty", src2.empty());

    fmt::println("");
}

void test_append_insert_erase() {
    fmt::println("--- Append / Insert / Erase ---");

    shine::SString s("Hello");
    s.append(" World");
    CHECK("Append: result", s.sv() == "Hello World");

    s += "!";
    CHECK("operator+=: result", s.sv() == "Hello World!");

    s.push_back('?');
    CHECK("push_back: result", s.sv() == "Hello World!?");

    shine::SString s2("ABCDEF");
    s2.insert(3, "XY");
    CHECK("Insert: result", s2.sv() == "ABCXYDEF");

    s2.erase(3, 2);
    CHECK("Erase: result", s2.sv() == "ABCDEF");

    s2.erase(4);
    CHECK("Erase to end: result", s2.sv() == "ABCD");

    // Test SSO → heap transition during append
    shine::SString s3("12345678901234567890123456"); // 26 chars, SSO
    CHECK("Pre-append SSO", s3.capacity() == 30);
    s3.append("7890ABCDE"); // total 35 chars, triggers heap
    CHECK("Post-append heap", s3.size() == 35);
    CHECK("Post-append content", s3.sv() == "123456789012345678901234567890ABCDE");

    fmt::println("");
}

void test_find() {
    fmt::println("--- Find ---");

    shine::SString s("The quick brown fox jumps over the lazy dog");
    CHECK("find(pattern): found", s.find(shine::STextView::from_cstring("fox")) == 16);
    CHECK("find(pattern): not found", s.find(shine::STextView::from_cstring("cat")) == shine::SString::npos);
    CHECK("find(char): found", s.find('q') == 4);
    CHECK("find(char): not found", s.find('Z') == shine::SString::npos);
    CHECK("find_first_of", s.find_first_of(shine::STextView::from_cstring("xyz")) == 18); // 'x' in "fox"
    // 'y' is at index 38 in "lazy", which is the last of x/y/z in the string
    CHECK("find_last_of", s.find_last_of(shine::STextView::from_cstring("xyz")) == 38); // 'y' in "lazy"

    // Single-char find_first_of fast path
    CHECK("find_first_of(single)", s.find_first_of(shine::STextView::from_cstring("q")) == 4);

    fmt::println("");
}

void test_replace() {
    fmt::println("--- Replace ---");

    {
        shine::SString s = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
        s.replace_inplace(shine::STextView::from_cstring("aaa"), shine::STextView::from_cstring("XXX"));
        CHECK("replace_inplace (same len)", s.sv() == "XXX bbb XXX ccc XXX");
    }

    {
        shine::SString s = shine::SString::from_utf8("Hello World");
        s.replace_inplace(shine::STextView::from_cstring("World"), shine::STextView::from_cstring("C++"));
        CHECK("replace_inplace (shrink)", s.sv() == "Hello C++");
    }

    {
        shine::SString s = shine::SString::from_utf8("ABC");
        auto result = s.replace(shine::STextView::from_cstring("B"), shine::STextView::from_cstring("XX"));
        CHECK("replace (return new)", result.sv() == "AXXC");
    }

    {
        shine::SString s = shine::SString::from_utf8("Hello World");
        bool ok = s.replace_first(shine::STextView::from_cstring("World"), shine::STextView::from_cstring("C++"));
        CHECK("replace_first", s.sv() == "Hello C++" && ok);
    }

    {
        shine::SString s = shine::SString::from_utf8("aXbXcXd");
        s.replace_inplace(shine::STextView::from_cstring("X"), shine::STextView::from_cstring("YYY"));
        CHECK("replace_inplace (grow)", s.sv() == "aYYYbYYYcYYYd");
    }

    fmt::println("");
}

void test_hash_and_containers() {
    fmt::println("--- Hash & Containers ---");

    shine::SString s1("Hello");
    shine::SString s2("Hello");
    shine::SString s3("World");

    CHECK("Hash: same string same hash", s1.hash() == s2.hash());
    CHECK("Hash: diff string diff hash", s1.hash() != s3.hash());

    // Compile-time hash consistency
    constexpr size_t compile_h = shine::SString::static_hash("Hello");
    CHECK("Hash: constexpr == runtime", compile_h == s1.hash());

    // Test std::unordered_map compatibility
    std::unordered_map<shine::SString, int> map;
    map[shine::SString("key1")] = 1;
    map[shine::SString("key2")] = 2;
    CHECK("unordered_map: insert/find", map[shine::SString("key1")] == 1);
    CHECK("unordered_map: count", map.size() == 2);

    fmt::println("");
}

void test_format() {
    fmt::println("--- std::format ---");

    shine::SString s("Hello Engine");
    std::string formatted = std::format("SString: {}", s);
    CHECK("std::format SString", formatted == "SString: Hello Engine");

    shine::STextView tv = shine::STextView::from_cstring("ViewTest");
    std::string formatted2 = std::format("View: {}", tv);
    CHECK("std::format STextView", formatted2 == "View: ViewTest");

    fmt::println("");
}

void test_trim_starts_ends() {
    fmt::println("--- Trim / StartsWith / EndsWith ---");

    shine::SString s("  Hello World  ");
    auto trimmed = s.trim();
    CHECK("Trim", trimmed == shine::STextView::from_cstring("Hello World"));

    shine::SString s2("path/to/file.txt");
    CHECK("starts_with", s2.starts_with(shine::STextView::from_cstring("path/")));
    CHECK("ends_with",   s2.ends_with(shine::STextView::from_cstring(".txt")));
    CHECK("contains",    s2.contains(shine::STextView::from_cstring("to/")));

    fmt::println("");
}

void test_utf8() {
    fmt::println("--- UTF-8 ---");

    // Chinese: 你好世界 = 4 code points, 12 code units
    shine::SString s = shine::SString::from_utf8("\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c");
    CHECK("UTF-8: code_unit_count", s.code_unit_count() == 12);
    CHECK("UTF-8: code_point_count", s.code_point_count() == 4);

    shine::STextView tv(s.data(), s.size());
    CHECK("UTF-8: valid", shine::STextView::is_valid_utf8(s.sv()));

    fmt::println("");
}

void test_shrink_to_fit() {
    fmt::println("--- shrink_to_fit ---");

    shine::SString s(std::string_view("short"));
    s.reserve(1000);
    CHECK("Before shrink: cap >= 1000", s.capacity() >= 1000);
    s.shrink_to_fit();
    // After shrink, short string should go back to SSO
    CHECK("After shrink: back to SSO cap", s.capacity() == shine::SString::kSsoMax);
    CHECK("After shrink: content intact", s.sv() == "short");

    fmt::println("");
}

void test_correctness() {
    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║               正确性测试                            ║");
    fmt::println("╚════════════════════════════════════════════════════╝\n");

    test_static_properties();
    test_sso();
    test_copy_move();
    test_append_insert_erase();
    test_find();
    test_replace();
    test_hash_and_containers();
    test_format();
    test_trim_starts_ends();
    test_utf8();
    test_shrink_to_fit();

    fmt::println("════════════════════════════════════════════════════");
    fmt::println("  Result: {} passed, {} failed\n", g_pass, g_fail);
}

// =========================================================
// Benchmark Helpers
// =========================================================

struct BenchmarkResult {
    const char* name = "";
    double s_time = 0;
    double std_time = 0;
    bool s_wins = false;
    double ratio = 0;
};

template <typename Fn>
double measure_ns(Fn&& fn, int iterations) {
    std::vector<long long> times;
    times.reserve(iterations);
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        fn();
        auto end = std::chrono::high_resolution_clock::now();
        times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }
    return std::accumulate(times.begin(), times.end(), 0LL) / static_cast<double>(times.size());
}

void print_result(const BenchmarkResult& r) {
    const char* winner = r.s_wins ? "SString" : "std::string";
    double speedup = r.s_wins ? r.ratio : (1.0 / r.ratio);
    fmt::println("  SString: {:>10.0f} ns  |  std::string: {:>10.0f} ns  |  {} {:.2f}x faster",
        r.s_time, r.std_time, winner, speedup);
    fmt::println("");
}

// =========================================================
// Benchmarks
// =========================================================

void benchmark() {
    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║          性能测试 (10000 次迭代)                    ║");
    fmt::println("╚════════════════════════════════════════════════════╝\n");

    constexpr int ITER = 10000;
    std::vector<BenchmarkResult> results;

    // 1. Small string construction (SSO)
    {
        fmt::println("【1】小字符串构造 (SSO, 11 bytes)");
        BenchmarkResult r; r.name = "Small Construct";
        r.s_time = measure_ns([&]() {
            shine::SString s("Hello World");
            DoNotOptimize(s);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string s("Hello World");
            DoNotOptimize(s);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 2. Medium string construction (SSO for SString, heap for std::string)
    {
        fmt::println("【2】中等字符串构造 (28 bytes: SString=SSO, std=Heap)  *** SSO优势 ***");
        BenchmarkResult r; r.name = "Medium SSO Advantage";
        r.s_time = measure_ns([&]() {
            shine::SString s("PlayerCharacterAnimCtrl_Idle"); // 28 chars: SSO in SString (<=30)
            DoNotOptimize(s);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string s("PlayerCharacterAnimCtrl_Idle"); // 28 chars: HEAP in std::string (>15)
            DoNotOptimize(s);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 3. Copy construct (SSO)
    {
        fmt::println("【3】Copy 构造 (SSO, 16 bytes)");
        BenchmarkResult r; r.name = "Copy SSO";
        shine::SString src("Hello World 123!");
        r.s_time = measure_ns([&]() {
            shine::SString copy = src;
            DoNotOptimize(copy);
        }, ITER);
        std::string src_std = "Hello World 123!";
        r.std_time = measure_ns([&]() {
            std::string copy = src_std;
            DoNotOptimize(copy);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 4. Copy construct (Heap)
    {
        fmt::println("【4】Copy 构造 (Heap, 100 bytes)");
        BenchmarkResult r; r.name = "Copy Heap";
        std::string long_str(100, 'x');
        shine::SString src{std::string_view{long_str}};
        r.s_time = measure_ns([&]() {
            shine::SString copy = src;
            DoNotOptimize(copy);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string copy = long_str;
            DoNotOptimize(copy);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 5. Move construct
    {
        fmt::println("【5】Move 构造 (Heap, 50 bytes)");
        BenchmarkResult r; r.name = "Move";
        std::string long_src(50, 'M');
        r.s_time = measure_ns([&]() {
            shine::SString s{std::string_view{long_src}};
            shine::SString dst = std::move(s);
            DoNotOptimize(dst);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string s(long_src);
            std::string dst = std::move(s);
            DoNotOptimize(dst);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 6. find
    {
        fmt::println("【6】find 搜索");
        BenchmarkResult r; r.name = "find";
        shine::SString ss = shine::SString::from_utf8("The quick brown fox jumps over the lazy dog");
        std::string stds = "The quick brown fox jumps over the lazy dog";
        r.s_time = measure_ns([&]() {
            auto p = ss.find(shine::STextView::from_cstring("lazy"));
            DoNotOptimize(p);
        }, ITER);
        r.std_time = measure_ns([&]() {
            auto p = stds.find("lazy");
            DoNotOptimize(p);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 7. Multi replace (in-place)
    {
        fmt::println("【7】多次 replace_inplace");
        BenchmarkResult r; r.name = "Multi Replace";
        r.s_time = measure_ns([&]() {
            shine::SString s = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
            s.replace_inplace(shine::STextView::from_cstring("aaa"), shine::STextView::from_cstring("XXX"));
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string s = "aaa bbb aaa ccc aaa";
            size_t pos = 0;
            while ((pos = s.find("aaa", pos)) != std::string::npos) {
                s.replace(pos, 3, "XXX");
                pos += 3;
            }
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 8. Append
    {
        fmt::println("【8】append");
        BenchmarkResult r; r.name = "Append";
        r.s_time = measure_ns([&]() {
            shine::SString s("Hello");
            s.append(" World");
            s.append(" Engine");
            s.append(" Test");
            DoNotOptimize(s);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string s("Hello");
            s.append(" World");
            s.append(" Engine");
            s.append(" Test");
            DoNotOptimize(s);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 9. Hash
    {
        fmt::println("【9】Hash 计算");
        BenchmarkResult r; r.name = "Hash";
        shine::SString ss = shine::SString::from_utf8("The quick brown fox jumps over the lazy dog");
        std::string stds = "The quick brown fox jumps over the lazy dog";
        std::hash<std::string> std_hasher;
        r.s_time = measure_ns([&]() {
            auto h = ss.hash();
            DoNotOptimize(h);
        }, ITER);
        r.std_time = measure_ns([&]() {
            auto h = std_hasher(stds);
            DoNotOptimize(h);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 10. Long string replace (core advantage)
    {
        fmt::println("【10】长字符串替换 (70KB, 1000 次) *** 核心优势 ***");
        BenchmarkResult r; r.name = "Long Replace";
        std::string long_str;
        for (int i = 0; i < 10000; ++i) long_str += "keyword";
        shine::SString long_s = shine::SString::from_utf8(long_str);

        constexpr int LONG_ITER = 1000;
        r.s_time = measure_ns([&]() {
            auto res = long_s.replace(shine::STextView::from_cstring("keyword"), shine::STextView::from_cstring("REPLACED"));
            DoNotOptimize(res);
        }, LONG_ITER);
        r.std_time = measure_ns([&]() {
            std::string temp = long_str;
            size_t pos = 0;
            while ((pos = temp.find("keyword", pos)) != std::string::npos) {
                temp.replace(pos, 7, "REPLACED");
                pos += 8;
            }
        }, LONG_ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time < r.std_time;

        fmt::println("  SString:     {:>12.0f} ns ({:.2f} us)", r.s_time, r.s_time / 1000.0);
        fmt::println("  std::string: {:>12.0f} ns ({:.2f} us)", r.std_time, r.std_time / 1000.0);
        if (r.s_wins) fmt::println("  -> SString {:.1f}x faster", r.ratio);
        else          fmt::println("  -> std::string {:.1f}x faster", 1.0 / r.ratio);
        fmt::println("");
        results.push_back(r);
    }

    // 11. STextView copy vs std::string_view copy
    {
        fmt::println("【11】View 拷贝 (STextView vs string_view)");
        BenchmarkResult r; r.name = "View Copy";
        shine::STextView stv("Hello World View Test!!!", 24);
        std::string_view ssv("Hello World View Test!!!", 24);
        r.s_time = measure_ns([&]() {
            shine::STextView copy = stv;
            DoNotOptimize(copy);
        }, ITER);
        r.std_time = measure_ns([&]() {
            std::string_view copy = ssv;
            DoNotOptimize(copy);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time <= r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 12. Array iteration — cache locality (SString 32B vs std::string 32B)
    {
        fmt::println("【12】数组遍历 (10000 strings, 缓存局部性)");
        BenchmarkResult r; r.name = "Array Iterate";

        constexpr int N = 10000;
        std::vector<shine::SString> s_arr;
        std::vector<std::string> std_arr;
        s_arr.reserve(N);
        std_arr.reserve(N);
        for (int i = 0; i < N; ++i) {
            char buf[32];
            int len = std::snprintf(buf, sizeof(buf), "item_%05d", i);
            s_arr.emplace_back(std::string_view(buf, static_cast<size_t>(len)));
            std_arr.emplace_back(buf, static_cast<size_t>(len));
        }

        r.s_time = measure_ns([&]() {
            size_t total = 0;
            for (const auto& s : s_arr) total += s.size();
            DoNotOptimize(total);
        }, ITER);
        r.std_time = measure_ns([&]() {
            size_t total = 0;
            for (const auto& s : std_arr) total += s.size();
            DoNotOptimize(total);
        }, ITER);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time <= r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // 13. Hash map insert+lookup
    {
        fmt::println("【13】HashMap 插入+查找 (1000 keys)");
        BenchmarkResult r; r.name = "HashMap";

        constexpr int N = 1000;
        std::vector<std::string> keys;
        keys.reserve(N);
        for (int i = 0; i < N; ++i) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "entity_%04d", i);
            keys.emplace_back(buf);
        }

        r.s_time = measure_ns([&]() {
            std::unordered_map<shine::SString, int> map;
            map.reserve(N);
            for (int i = 0; i < N; ++i) {
                map.emplace(shine::SString(keys[i].c_str()), i);
            }
            int sum = 0;
            for (int i = 0; i < N; ++i) {
                auto it = map.find(shine::SString(keys[i].c_str()));
                if (it != map.end()) sum += it->second;
            }
            DoNotOptimize(sum);
        }, 100);
        r.std_time = measure_ns([&]() {
            std::unordered_map<std::string, int> map;
            map.reserve(N);
            for (int i = 0; i < N; ++i) {
                map.emplace(keys[i], i);
            }
            int sum = 0;
            for (int i = 0; i < N; ++i) {
                auto it = map.find(keys[i]);
                if (it != map.end()) sum += it->second;
            }
            DoNotOptimize(sum);
        }, 100);
        r.ratio = r.std_time / r.s_time;
        r.s_wins = r.s_time <= r.std_time;
        print_result(r);
        results.push_back(r);
    }

    // Summary
    int s_wins = 0, std_wins = 0;
    for (auto& r : results) {
        if (r.s_wins) ++s_wins; else ++std_wins;
    }

    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║                    总结                            ║");
    fmt::println("╠════════════════════════════════════════════════════╣");
    fmt::println("║  SString 胜: {}  |  std::string 胜: {}              ║", s_wins, std_wins);
    fmt::println("╠════════════════════════════════════════════════════╣");

    double best = 0;
    const char* best_name = "";
    for (auto& r : results) {
        if (r.s_wins && r.ratio > best) { best = r.ratio; best_name = r.name; }
    }
    if (best > 1) {
        fmt::println("║  最大优势: {} ({:.1f}x faster)       ║", best_name, best);
    }

    double worst = 1e9;
    const char* worst_name = "";
    for (auto& r : results) {
        if (!r.s_wins && r.ratio < worst) { worst = r.ratio; worst_name = r.name; }
    }
    if (worst < 1e9) {
        fmt::println("║  最大劣势: {} ({:.1f}x slower)       ║", worst_name, 1.0 / worst);
    }
    fmt::println("╚════════════════════════════════════════════════════╝");
}

// =========================================================
// Main
// =========================================================

int main() {
    fmt::println("╔════════════════════════════════════════════════════╗");
    fmt::println("║   ShineEngine SString Optimized Test Suite          ║");
    fmt::println("║     SString(32B, 30-char SSO) vs std::string(32B)   ║");
    fmt::println("║     STextView(16B, trivially copyable)              ║");
    fmt::println("╚════════════════════════════════════════════════════╝\n");

    test_correctness();
    benchmark();

    if (g_fail > 0) {
        fmt::println("\n!!! {} tests FAILED !!!", g_fail);
        return 1;
    }

    fmt::println("\nAll {} tests passed.", g_pass);
    return 0;
}
