#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <format>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../src/string/shine_string.h"
#include "../common/test_benchmark_framework.h"
#include "fmt/base.h"
#include "fmt/format.h"

static shine::test::TestContext g_test_ctx;

using shine::test::BenchmarkStats;
using shine::test::ComparisonResult;
using shine::test::DoNotOptimize;
using shine::test::build_summary;
using shine::test::measure_benchmark;
using shine::test::print_comparison;
using shine::test::print_group_summary;
using shine::test::print_summary;
using shine::test::run_compare;

#define CHECK(name, cond) SHINE_TEST_CHECK(g_test_ctx, name, cond)

// std::formatter<SString> and std::formatter<STextView> are defined in
// shine_string.h / shine_text_view.h (guarded by __cpp_lib_format).

// =========================================================
// Dataset helpers
// =========================================================

struct Corpus {
    std::vector<std::string> tiny;
    std::vector<std::string> small;
    std::vector<std::string> medium;
    std::vector<std::string> large;
    std::vector<std::string> utf8;
    std::vector<std::string> trim_texts;
    std::vector<std::string> paths;
    std::vector<std::string> kv_keys;
};

static Corpus make_corpus() {
    Corpus c;

    c.tiny = {
        "", "a", "ab", "abc", "id", "hp", "mp", "x1", "AB", "!"
    };

    c.small = {
        "Hello",
        "World",
        "Player",
        "Enemy_01",
        "Transform",
        "Position2D",
        "MeshNode",
        "ScriptCore",
        "AssetPath",
        "GameState"
    };

    c.medium = {
        "PlayerCharacterAnimCtrl_Idle",
        "assets/textures/hero_diffuse.png",
        "scene/main_city/camera_controller",
        "AnimationStateMachine_UpperBody_Run",
        "entity_component_transform_position"
    };

    c.large = {
        std::string(64, 'A'),
        std::string(96, 'B'),
        std::string(128, 'C'),
        std::string(192, 'D'),
        std::string(256, 'E')
    };

    c.utf8 = {
        "你好世界",
        "こんにちは世界",
        "안녕하세요 월드",
        "Привет мир",
        "😀😃😄😁😆",
        "数据驱动引擎系统"
    };

    c.trim_texts = {
        "   hello world   ",
        "\t\tconfig/path/to/file.txt\t",
        "\n  player_name  \r",
        "   keyword keyword keyword   ",
        "     1234567890     "
    };

    c.paths = {
        "assets/textures/player.png",
        "assets/shaders/pbr/lighting_pass.vert",
        "scene/world/terrain/chunk_001",
        "save/profile_01/slot_03/data.bin",
        "scripts/gameplay/player_controller.lua"
    };

    c.kv_keys.reserve(2000);
    for (int i = 0; i < 2000; ++i) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "entity_%04d_component_%02d", i, i % 17);
        c.kv_keys.emplace_back(buf);
    }

    return c;
}

// =========================================================
// Correctness Tests
// =========================================================

static const Corpus& corpus() {
    static Corpus c = make_corpus();
    return c;
}

static inline std::size_t bench_index(int i, std::size_t n) noexcept {
    return static_cast<std::size_t>(i) % n;
}

// =========================================================
// Correctness Tests
// =========================================================

void test_static_properties() {
    fmt::println("--- Static Properties ---");
    CHECK("sizeof(STextView) == sizeof(std::string_view)",
        sizeof(shine::STextView) == sizeof(std::string_view));
    CHECK("sizeof(STextView) == 16",
        sizeof(shine::STextView) == 16);
    CHECK("sizeof(SString) == 32",
        sizeof(shine::SString) == 32);
    CHECK("STextView trivially copyable",
        std::is_trivially_copyable_v<shine::STextView>);
    CHECK("SString noexcept move constructible",
        std::is_nothrow_move_constructible_v<shine::SString>);
    CHECK("SString noexcept move assignable",
        std::is_nothrow_move_assignable_v<shine::SString>);

    constexpr std::size_t h = shine::SString::static_hash("entity_position");
    CHECK("static_hash constexpr non-zero", h != 0);
    CHECK("static_hash stable",
        h == shine::SString::static_hash("entity_position"));

    fmt::println("");
}

void test_sstring_basic() {
    fmt::println("--- SString Basic ---");

    shine::SString s1;
    CHECK("default empty", s1.empty());
    CHECK("default size 0", s1.size() == 0);

    shine::SString s2("Hello");
    CHECK("construct from cstr", s2.sv() == "Hello");
    CHECK("size()", s2.size() == 5);
    CHECK("length()", s2.length() == 5);
    CHECK("data/c_str()", std::string_view(s2.c_str()) == "Hello");

    shine::SString s3(std::string_view("World"));
    CHECK("construct from string_view", s3.sv() == "World");

    shine::SString s4(100);
    CHECK("reserve ctor capacity", s4.capacity() >= 100);

    s4 = "abc";
    CHECK("assign const char*", s4.sv() == "abc");
    s4 = std::string_view("defg");
    CHECK("assign string_view", s4.sv() == "defg");
    s4 = shine::SString(shine::STextView::from_cstring("xyz"));
    CHECK("assign STextView via SString", s4.sv() == "xyz");

    fmt::println("");
}

void test_sstring_sso() {
    fmt::println("--- SString SSO ---");

    shine::SString s1("123456789012345678901234567890");
    CHECK("30 chars still SSO-capacity", s1.size() == 30 && s1.capacity() == 30);

    shine::SString s2("1234567890123456789012345678901");
    CHECK("31 chars heap", s2.size() == 31 && s2.capacity() >= 31);

    shine::SString s3("assets/textures/hero.png");
    CHECK("24 chars in SSO", s3.capacity() == 30);

    shine::SString s4("PlayerCharacterAnimCtrl_Idle");
    CHECK("28 chars in SSO", s4.capacity() == 30);

    fmt::println("");
}

void test_sstring_view_and_factories() {
    fmt::println("--- SString View / Factories ---");

    shine::SString s("Hello View");
    CHECK("view()", s.view() == "Hello View");
    CHECK("as_view()", s.as_view() == "Hello View");

    auto v1 = s.view();
    auto v2 = s.as_view();
    CHECK("view data matches", v1.data() == s.data());
    CHECK("as_view data matches", v2.data() == s.data());

    auto from_view = shine::SString::from_view(shine::STextView::from_cstring("FromView"));
    CHECK("from_view", from_view == "FromView");

    std::string utf8_src = "FromUtf8String";
    auto from_utf8_std = shine::SString::from_utf8(utf8_src);
    CHECK("from_utf8(std::string)", from_utf8_std == "FromUtf8String");

    fmt::println("");
}

void test_sstring_copy_move() {
    fmt::println("--- SString Copy / Move ---");

    shine::SString orig("Hello SSO");
    shine::SString copy = orig;
    CHECK("copy SSO equal", copy == orig);

    shine::SString big(std::string_view("This is a long string that exceeds the SSO capacity of SString."));
    shine::SString big_copy = big;
    CHECK("copy heap equal", big_copy == big);

    shine::SString src("Move SSO");
    shine::SString dst = std::move(src);
    CHECK("move SSO dst value", dst == "Move SSO");
    CHECK("move SSO src empty", src.empty());

    shine::SString src2(std::string_view("This is a long string for heap move test and it must exceed SSO."));
    auto expected = src2.sv();
    shine::SString dst2 = std::move(src2);
    CHECK("move heap dst value", dst2.sv() == expected);
    CHECK("move heap src empty", src2.empty());

    fmt::println("");
}

void test_sstring_capacity() {
    fmt::println("--- SString Capacity ---");

    shine::SString s("short");
    auto old_cap = s.capacity();
    s.reserve(1000);
    CHECK("reserve grow", s.capacity() >= 1000);
    CHECK("reserve keeps content", s == "short");

    s.resize(10, 'x');
    CHECK("resize grow size", s.size() == 10);
    CHECK("resize grow fill", s.sv() == "shortxxxxx");

    s.resize(3);
    CHECK("resize shrink", s.sv() == "sho");

    s.clear();
    CHECK("clear empty", s.empty());

    s = "short";
    s.reserve(1000);
    s.shrink_to_fit();
    CHECK("shrink_to_fit returns to SSO", s.capacity() == 30);
    CHECK("shrink_to_fit keeps content", s == "short");
    CHECK("old small cap changed after reserve", old_cap == 30);

    fmt::println("");
}

void test_sstring_append_insert_erase() {
    fmt::println("--- SString Append / Insert / Erase ---");

    shine::SString s("Hello");
    s.append(" World");
    CHECK("append cstr", s == "Hello World");

    s.append(std::string_view(" Engine"));
    CHECK("append string_view", s == "Hello World Engine");

    std::string tail = " Core";
    s.append(tail);
    CHECK("append std::string", s == "Hello World Engine Core");

    s += "!";
    CHECK("operator+= cstr", s == "Hello World Engine Core!");

    s += std::string("?");
    CHECK("operator+= std::string", s == "Hello World Engine Core!?");

    s.push_back('@');
    CHECK("push_back", s == "Hello World Engine Core!?@");

    shine::SString s2("ABCDEF");
    s2.insert(3, "XY");
    CHECK("insert cstr middle", s2 == "ABCXYDEF");

    s2.insert(2, std::string("QQ"));
    CHECK("insert std::string middle", s2 == "ABQQCXYDEF");

    s2.erase(2, 2);
    CHECK("erase count", s2 == "ABCXYDEF");

    s2.erase(3, 2);
    CHECK("erase second count", s2 == "ABCDEF");

    s2.erase(4);
    CHECK("erase tail", s2 == "ABCD");

    shine::SString s3("12345678901234567890123456");
    CHECK("append pre-heap SSO", s3.capacity() == 30);
    s3.append("7890ABCDE");
    CHECK("append crosses SSO", s3 == "123456789012345678901234567890ABCDE");

    fmt::println("");
}

void test_sstring_element_access() {
    fmt::println("--- SString Element Access ---");

    shine::SString s("Hello");
    CHECK("operator[]", s[1] == 'e');
    CHECK("front", s.front() == 'H');
    CHECK("back", s.back() == 'o');

    s[0] = 'Y';
    CHECK("mutable operator[]", s == "Yello");

    bool threw = false;
    try {
        (void)s.at(100);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK("at throws", threw);

    fmt::println("");
}

void test_sstring_substr_concat_compare() {
    fmt::println("--- SString Substr / Concat / Compare ---");

    shine::SString s("The quick brown fox");
    auto sub = s.substr(4, 5);
    CHECK("substr", sub == "quick");

    auto view = s.subview(10, 5);
    CHECK("subview", view == "brown");

    shine::SString a("Hello");
    shine::SString b("World");
    auto c1 = a + b;
    auto c2 = a + shine::STextView::from_cstring(" ");
    auto c3 = shine::STextView::from_cstring("Say: ") + a;
    auto c4 = a + shine::STextView::from_cstring(" Engine");
    auto c5 = shine::STextView::from_cstring(">>") + a;
    auto c6 = a + "!";
    auto c7 = "<<" + a;

    CHECK("operator+ SString+SString", c1 == "HelloWorld");
    CHECK("operator+ SString+view", c2 == "Hello ");
    CHECK("operator+ view+SString", c3 == "Say: Hello");
    CHECK("operator+ SString+STextView", c4 == "Hello Engine");
    CHECK("operator+ STextView+SString", c5 == ">>Hello");
    CHECK("operator+ SString+cstr", c6 == "Hello!");
    CHECK("operator+ cstr+SString", c7 == "<<Hello");

    CHECK("operator== SString", a == shine::SString("Hello"));
    CHECK("operator== cstr", a == "Hello");
    CHECK("operator== string_view", a == std::string_view("Hello"));
    CHECK("operator== std::string rhs", a == std::string("Hello"));
    CHECK("operator== std::string lhs", std::string_view("Hello") == a.sv());
    CHECK("operator== STextView rhs", a == shine::STextView::from_cstring("Hello"));
    CHECK("operator== STextView lhs", shine::STextView::from_cstring("Hello") == a);
    CHECK("operator<=>", (a <=> b) == std::strong_ordering::less);

    fmt::println("");
}

void test_sstring_find_related() {
    fmt::println("--- SString Search API ---");

    shine::SString s("The quick brown fox jumps over the lazy dog");
    CHECK("find pattern found", s.find(shine::STextView::from_cstring("fox")) == 16);
    CHECK("find pattern miss", s.find(shine::STextView::from_cstring("cat")) == shine::SString::npos);
    CHECK("find char", s.find('q') == 4);
    CHECK("find char miss", s.find('Z') == shine::SString::npos);
    CHECK("rfind char", s.rfind('o') == 41);
    CHECK("find_first_of", s.find_first_of(shine::STextView::from_cstring("xyz")) == 18);
    CHECK("find_first_not_of", shine::SString("   abc").find_first_not_of(shine::STextView::from_cstring(" ")) == 3);
    CHECK("find_last_of", s.find_last_of(shine::STextView::from_cstring("xyz")) == 38);
    CHECK("contains pattern", s.contains(shine::STextView::from_cstring("lazy")));
    CHECK("contains char", s.contains('z'));
    CHECK("starts_with", s.starts_with(shine::STextView::from_cstring("The")));
    CHECK("ends_with", s.ends_with(shine::STextView::from_cstring("dog")));

    fmt::println("");
}

void test_sstring_trim_replace_hash() {
    fmt::println("--- SString Trim / Replace / Hash ---");

    shine::SString s("  Hello World  ");
    CHECK("trim", s.trim() == "Hello World");
    CHECK("trim_start", s.trim_start() == "Hello World  ");
    CHECK("trim_end", s.trim_end() == "  Hello World");

    {
        shine::SString x = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
        x.replace_inplace("aaa", "XXX");
        CHECK("replace_inplace same len", x == "XXX bbb XXX ccc XXX");
    }

    {
        shine::SString x = shine::SString::from_utf8("Hello World");
        bool ok = x.replace_first("World", "C++");
        CHECK("replace_first", ok && x == "Hello C++");
    }

    {
        shine::SString x = shine::SString::from_utf8("ABC");
        auto y = x.replace("B", "XX");
        CHECK("replace return new", y == "AXXC");
    }

    {
        shine::SString x("left-middle-right");
        x.replace_inplace(std::string("middle"), std::string("CENTER"));
        CHECK("replace_inplace std::string", x == "left-CENTER-right");
    }

    {
        shine::SString x("left-middle-right");
        bool ok = x.replace_first(std::string("middle"), std::string("CENTER"));
        CHECK("replace_first std::string", ok && x == "left-CENTER-right");
    }

    {
        shine::SString x("left-middle-right");
        auto y = x.replace(std::string("middle"), std::string("CENTER"));
        CHECK("replace std::string", y == "left-CENTER-right");
    }

    shine::SString h1("Hello");
    shine::SString h2("Hello");
    shine::SString h3("World");

    CHECK("hash equal strings", h1.hash() == h2.hash());
    CHECK("hash different strings", h1.hash() != h3.hash());

    constexpr std::size_t compile_h = shine::SString::static_hash("Hello");
    CHECK("constexpr static_hash stable", compile_h == shine::SString::static_hash("Hello"));
    CHECK("runtime hash equal strings", h1.hash() == h2.hash());

    std::unordered_map<shine::SString, int> map;
    map[shine::SString("key1")] = 1;
    map[shine::SString("key2")] = 2;
    CHECK("unordered_map support", map[shine::SString("key1")] == 1 && map.size() == 2);

    fmt::println("");
}

void test_stextview_basic() {
    fmt::println("--- STextView Basic ---");

    shine::STextView v1;
    CHECK("default empty", v1.empty());
    CHECK("default valid", v1.is_valid());

    shine::STextView v2("Hello");
    CHECK("construct cstr", v2 == "Hello");
    CHECK("size", v2.size() == 5);
    CHECK("size_bytes", v2.size_bytes() == 5);
    CHECK("code_unit_count", v2.code_unit_count() == 5);

    shine::STextView v3("HelloWorld", 5);
    CHECK("construct ptr+size", v3 == "Hello");

    const char* first = "abcdef";
    shine::STextView v4(first + 1, first + 4);
    CHECK("construct first/last", v4 == "bcd");

    shine::STextView v5(std::string_view("View"));
    CHECK("construct string_view", v5 == "View");

    constexpr auto lit = shine::STextView::from_literal("Literal");
    CHECK("from_literal", lit == "Literal");

    auto cstr = shine::STextView::from_cstring("CString");
    CHECK("from_cstring", cstr == "CString");

    CHECK("to_string", v2.to_string() == std::string("Hello"));
    CHECK("sv()", v2.sv() == std::string_view("Hello"));

    fmt::println("");
}

void test_stextview_access_slice_search() {
    fmt::println("--- STextView Access / Slice / Search ---");

    shine::STextView v("The quick brown fox jumps over the lazy dog");
    CHECK("operator[]", v[1] == 'h');
    CHECK("front", v.front() == 'T');
    CHECK("back", v.back() == 'g');
    CHECK("byte_at", v.byte_at(4) == 'q');

    CHECK("substr", v.substr(4, 5) == "quick");
    CHECK("first", v.first(3) == "The");
    CHECK("last", v.last(3) == "dog");

    CHECK("find pattern", v.find("brown") == 10);
    CHECK("find char", v.find('q') == 4);
    CHECK("rfind char", v.rfind('o') == 41);
    CHECK("find_first_of", v.find_first_of("xyz") == 18);
    CHECK("find_first_not_of", shine::STextView("   abc").find_first_not_of(" ") == 3);
    CHECK("find_last_of", v.find_last_of("xyz") == 38);
    CHECK("contains pattern", v.contains("lazy"));
    CHECK("contains char", v.contains('z'));
    CHECK("starts_with", v.starts_with("The"));
    CHECK("ends_with", v.ends_with("dog"));
    CHECK("equals", v.equals("The quick brown fox jumps over the lazy dog"));

    auto trimmed = shine::STextView(" \t hello \n ").trim();
    CHECK("trim", trimmed == "hello");

    fmt::println("");
}

void test_stextview_utf8() {
    fmt::println("--- STextView UTF-8 ---");

    shine::SString s = shine::SString::from_utf8("你好世界");
    shine::STextView tv(s.data(), s.size());

    CHECK("utf8 byte size", tv.code_unit_count() == 12);
    CHECK("utf8 code point count", tv.code_point_count() == 4);
    CHECK("byte_index_from_code_point 0", tv.byte_index_from_code_point(0) == 0);
    CHECK("byte_index_from_code_point 1", tv.byte_index_from_code_point(1) == 3);
    CHECK("substr_code_points", tv.substr_code_points(1, 2) == shine::STextView::from_cstring("好世"));
    CHECK("contains_code_point", tv.contains_code_point(U'界'));
    CHECK("find_code_point", tv.find_code_point(U'世') == 6);
    CHECK("compare_code_points eq", tv.compare_code_points(shine::STextView::from_cstring("你好世界")) == 0);

    std::u32string cps;
    tv.for_each_code_point([&](char32_t cp) { cps.push_back(cp); });
    CHECK("for_each_code_point count", cps.size() == 4);

    CHECK("compat utf8_index_from_code_point", tv.utf8_index_from_code_point(2) == 6);
    CHECK("compat substr_cp", tv.substr_cp(2, 1) == shine::STextView::from_cstring("世"));
    CHECK("compat find_cp", tv.find_cp(U'界') == 9);
    CHECK("compat compare_cp", tv.compare_cp(shine::STextView::from_cstring("你好世界")) == 0);

    fmt::println("");
}

void test_formatting() {
    fmt::println("--- std::format ---");

    shine::SString s("Hello Engine");
    std::string formatted = std::format("SString: {}", s);
    CHECK("format SString", formatted == "SString: Hello Engine");

    shine::STextView tv = shine::STextView::from_cstring("ViewTest");
    std::string formatted2 = std::format("View: {}", tv);
    CHECK("format STextView", formatted2 == "View: ViewTest");

    fmt::println("");
}

void test_correctness() {
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║                        字符串正确性测试                         ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    test_static_properties();
    test_sstring_basic();
    test_sstring_sso();
    test_sstring_view_and_factories();
    test_sstring_copy_move();
    test_sstring_capacity();
    test_sstring_append_insert_erase();
    test_sstring_element_access();
    test_sstring_substr_concat_compare();
    test_sstring_find_related();
    test_sstring_trim_replace_hash();
    test_stextview_basic();
    test_stextview_access_slice_search();
    test_stextview_utf8();
    test_formatting();

    g_test_ctx.print_summary("Result");
}

// =========================================================
// Benchmark Framework
// =========================================================

// =========================================================
// Benchmark Suites
// =========================================================

void benchmark_sstring_vs_std_string(std::vector<ComparisonResult>& results) {
    const auto& c = corpus();

    constexpr int R = 24;
    constexpr int I = 2000;

    // Construction
    results.push_back(run_compare("1", "构造: tiny cstr",
        R, I,
        [&]() {
            shine::SString s(c.tiny[3].c_str());
            DoNotOptimize(s);
        },
        [&]() {
            std::string s(c.tiny[3].c_str());
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    results.push_back(run_compare("2", "构造: small cstr",
        R, I,
        [&]() {
            shine::SString s(c.small[4].c_str());
            DoNotOptimize(s);
        },
        [&]() {
            std::string s(c.small[4].c_str());
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    results.push_back(run_compare("3", "构造: medium cstr (SString SSO 优势区间)",
        R, I,
        [&]() {
            shine::SString s(c.medium[0].c_str());
            DoNotOptimize(s);
        },
        [&]() {
            std::string s(c.medium[0].c_str());
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    results.push_back(run_compare("4", "构造: large string_view",
        R, I,
        [&]() {
            shine::SString s(std::string_view(c.large[2]));
            DoNotOptimize(s);
        },
        [&]() {
            std::string s(std::string_view(c.large[2]));
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    // Copy
    {
        shine::SString src("Hello World 123!");
        std::string src_std = "Hello World 123!";
        results.push_back(run_compare("5", "拷贝构造: small",
            R, I,
            [&]() {
                shine::SString copy = src;
                DoNotOptimize(copy);
            },
            [&]() {
                std::string copy = src_std;
                DoNotOptimize(copy);
            },
            "shine",
            "std",
            "SString/构造"));
    }

    {
        shine::SString src(std::string_view(c.large[3]));
        std::string src_std = c.large[3];
        results.push_back(run_compare("6", "拷贝构造: large",
            R, I,
            [&]() {
                shine::SString copy = src;
                DoNotOptimize(copy);
            },
            [&]() {
                std::string copy = src_std;
                DoNotOptimize(copy);
            },
            "shine",
            "std",
            "SString/构造"));
    }

    // Move
    {
        std::string seed = c.large[1];
        results.push_back(run_compare("7", "移动构造: heap string",
            R, I,
            [&]() {
                shine::SString s{ std::string_view{seed} };
                shine::SString moved = std::move(s);
                DoNotOptimize(moved);
            },
            [&]() {
                std::string s(seed);
                std::string moved = std::move(s);
                DoNotOptimize(moved);
            },
            "shine",
            "std",
            "SString/构造"));
    }

    // Assignment
    {
        shine::SString s;
        std::string t;
        results.push_back(run_compare("8", "赋值: const char*",
            R, I,
            [&]() {
                s = c.medium[1].c_str();
                DoNotOptimize(s);
            },
            [&]() {
                t = c.medium[1].c_str();
                DoNotOptimize(t);
            },
            "shine",
            "std",
            "SString/构造"));
    }

    // Append / push_back
    results.push_back(run_compare("9", "append: 多段追加",
        R, I,
        [&]() {
            shine::SString s("Hello");
            s.append(" ");
            s.append("World");
            s.append(" Engine");
            s.append(" Test");
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("Hello");
            s.append(" ");
            s.append("World");
            s.append(" Engine");
            s.append(" Test");
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("10", "push_back: 逐字符构建",
        R, I,
        [&]() {
            shine::SString s;
            for (char ch : c.medium[0]) s.push_back(ch);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s;
            for (char ch : c.medium[0]) s.push_back(ch);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    // Insert / erase
    results.push_back(run_compare("11", "insert: 中间插入",
        R, I,
        [&]() {
            shine::SString s("ABCDEF");
            s.insert(3, "XYZ");
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("ABCDEF");
            s.insert(3, "XYZ");
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("12", "erase: 中间删除",
        R, I,
        [&]() {
            shine::SString s("ABCDEFGHIJKLMN");
            s.erase(4, 5);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("ABCDEFGHIJKLMN");
            s.erase(4, 5);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    // Resize / reserve / shrink
    results.push_back(run_compare("13", "resize: grow+shrink",
        R, I,
        [&]() {
            shine::SString s("short");
            s.resize(64, 'x');
            s.resize(12);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("short");
            s.resize(64, 'x');
            s.resize(12);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("14", "reserve + append",
        R, I,
        [&]() {
            shine::SString s;
            s.reserve(128);
            s.append(std::string_view(c.medium[1]));
            s.append(std::string_view(c.medium[2]));
            DoNotOptimize(s);
        },
        [&]() {
            std::string s;
            s.reserve(128);
            s.append(c.medium[1]);
            s.append(c.medium[2]);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("14.1", "append: std::string 输入",
        R, I,
        [&]() {
            static int idx = 0;
            const auto& a = c.small[bench_index(idx, c.small.size())];
            const auto& b = c.small[bench_index(idx + 3, c.small.size())];
            ++idx;

            shine::SString s;
            s.append(a);
            s.append(b);
            DoNotOptimize(s);
        },
        [&]() {
            static int idx = 0;
            const auto& a = c.small[bench_index(idx, c.small.size())];
            const auto& b = c.small[bench_index(idx + 3, c.small.size())];
            ++idx;

            std::string s;
            s.append(a);
            s.append(b);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("14.2", "构造: std::string 输入",
        R, I,
        [&]() {
            static int idx = 0;
            const auto& src = c.large[bench_index(idx, c.large.size())];
            ++idx;

            shine::SString s(src);
            DoNotOptimize(s);
        },
        [&]() {
            static int idx = 0;
            const auto& src = c.large[bench_index(idx, c.large.size())];
            ++idx;

            std::string s(src);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    results.push_back(run_compare("14.3", "赋值: std::string 输入",
        R, I,
        [&]() {
            shine::SString s;
            s = c.medium[3];
            DoNotOptimize(s);
        },
        [&]() {
            std::string s;
            s = c.medium[3];
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/构造"));

    results.push_back(run_compare("14.4", "insert: std::string 输入",
        R, I,
        [&]() {
            shine::SString s("ABCDEF");
            s.insert(2, c.small[2]);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("ABCDEF");
            s.insert(2, c.small[2]);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    results.push_back(run_compare("15", "shrink_to_fit",
        R, I,
        [&]() {
            shine::SString s("short");
            s.reserve(1024);
            s.shrink_to_fit();
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("short");
            s.reserve(1024);
            s.shrink_to_fit();
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/修改"));

    // Element access
    results.push_back(run_compare("16", "元素访问: front/back/[]",
        R, I,
        [&]() {
            shine::SString s("HelloEngine");
            volatile int sum = s.front() + s.back() + s[3];
            DoNotOptimize(sum);
        },
        [&]() {
            std::string s("HelloEngine");
            volatile int sum = s.front() + s.back() + s[3];
            DoNotOptimize(sum);
        },
        "shine",
        "std",
        "SString/访问"));

    // substr
    results.push_back(run_compare("17", "substr: 拷贝子串",
        R, I,
        [&]() {
            shine::SString s(c.medium[1].c_str());
            auto sub = s.substr(7, 8);
            DoNotOptimize(sub);
        },
        [&]() {
            std::string s(c.medium[1].c_str());
            auto sub = s.substr(7, 8);
            DoNotOptimize(sub);
        },
        "shine",
        "std",
        "SString/视图"));

    // Search
    {
        shine::SString ss = shine::SString::from_utf8("The quick brown fox jumps over the lazy dog");
        std::string stds = "The quick brown fox jumps over the lazy dog";

        results.push_back(run_compare("18", "find: substring",
            R, I,
            [&]() {
                auto p = ss.find("lazy");
                DoNotOptimize(p);
            },
            [&]() {
                auto p = stds.find("lazy");
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("19", "find: char",
            R, I,
            [&]() {
                auto p = ss.find('q');
                DoNotOptimize(p);
            },
            [&]() {
                auto p = stds.find('q');
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("20", "rfind: char",
            R, I,
            [&]() {
                auto p = ss.rfind('o');
                DoNotOptimize(p);
            },
            [&]() {
                auto p = stds.rfind('o');
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("21", "find_first_of",
            R, I,
            [&]() {
                auto p = ss.find_first_of("xyz");
                DoNotOptimize(p);
            },
            [&]() {
                auto p = stds.find_first_of("xyz");
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("22", "contains 模拟",
            R, I,
            [&]() {
                auto ok = ss.contains("lazy");
                DoNotOptimize(ok);
            },
            [&]() {
                auto ok = stds.find("lazy") != std::string::npos;
                DoNotOptimize(ok);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("22.1", "find_first_not_of",
            R, I,
            [&]() {
                shine::SString s("   abcdef");
                auto p = s.find_first_not_of(" ");
                DoNotOptimize(p);
            },
            [&]() {
                std::string s("   abcdef");
                auto p = s.find_first_not_of(" ");
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("22.2", "find_last_of",
            R, I,
            [&]() {
                auto p = ss.find_last_of("aeiou");
                DoNotOptimize(p);
            },
            [&]() {
                auto p = stds.find_last_of("aeiou");
                DoNotOptimize(p);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("22.3", "subview/view 等价访问",
            R, I,
            [&]() {
                auto v = ss.subview(4, 5);
                DoNotOptimize(v);
            },
            [&]() {
                auto v = std::string_view(stds).substr(4, 5);
                DoNotOptimize(v);
            },
            "shine",
            "std",
            "SString/视图"));
    }

    // Prefix/suffix
    {
        shine::SString ss(c.paths[0].c_str());
        std::string stds = c.paths[0];
        results.push_back(run_compare("23", "starts_with",
            R, I,
            [&]() {
                auto ok = ss.starts_with("assets/");
                DoNotOptimize(ok);
            },
            [&]() {
                auto ok = stds.starts_with("assets/");
                DoNotOptimize(ok);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("24", "ends_with",
            R, I,
            [&]() {
                auto ok = ss.ends_with(".png");
                DoNotOptimize(ok);
            },
            [&]() {
                auto ok = stds.ends_with(".png");
                DoNotOptimize(ok);
            },
            "shine",
            "std",
            "SString/查找"));
    }

    // Replace
    results.push_back(run_compare("25", "replace_inplace: 多次替换",
        20, 1000,
        [&]() {
            shine::SString s = shine::SString::from_utf8("aaa bbb aaa ccc aaa");
            s.replace_inplace("aaa", "XXX");
            DoNotOptimize(s);
        },
        [&]() {
            std::string s = "aaa bbb aaa ccc aaa";
            std::size_t pos = 0;
            while ((pos = s.find("aaa", pos)) != std::string::npos) {
                s.replace(pos, 3, "XXX");
                pos += 3;
            }
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/替换"));

    results.push_back(run_compare("26", "replace_first",
        R, I,
        [&]() {
            shine::SString s = shine::SString::from_utf8("Hello World Hello");
            bool ok = s.replace_first("World", "Engine");
            DoNotOptimize(ok);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s = "Hello World Hello";
            auto pos = s.find("World");
            bool ok = pos != std::string::npos;
            if (ok) s.replace(pos, 5, "Engine");
            DoNotOptimize(ok);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/替换"));

    results.push_back(run_compare("27", "replace: 返回新字符串",
        20, 1000,
        [&]() {
            shine::SString s = shine::SString::from_utf8("foo bar foo baz foo");
            auto out = s.replace("foo", "qux");
            DoNotOptimize(out);
        },
        [&]() {
            std::string s = "foo bar foo baz foo";
            std::string out = s;
            std::size_t pos = 0;
            while ((pos = out.find("foo", pos)) != std::string::npos) {
                out.replace(pos, 3, "qux");
                pos += 3;
            }
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/替换"));

    results.push_back(run_compare("27.1", "replace_inplace: std::string 输入",
        R, I,
        [&]() {
            shine::SString s("left-middle-right");
            s.replace_inplace(std::string("middle"), std::string("CENTER"));
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("left-middle-right");
            s.replace(s.find("middle"), std::string("middle").size(), std::string("CENTER"));
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/替换"));

    results.push_back(run_compare("27.2", "replace_first: std::string 输入",
        R, I,
        [&]() {
            shine::SString s("left-middle-right");
            auto ok = s.replace_first(std::string("middle"), std::string("CENTER"));
            DoNotOptimize(ok);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("left-middle-right");
            auto pos = s.find("middle");
            bool ok = pos != std::string::npos;
            if (ok) s.replace(pos, std::string("middle").size(), std::string("CENTER"));
            DoNotOptimize(ok);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/替换"));

    results.push_back(run_compare("27.3", "replace: std::string 输入",
        R, I,
        [&]() {
            shine::SString s("left-middle-right");
            auto out = s.replace(std::string("middle"), std::string("CENTER"));
            DoNotOptimize(out);
        },
        [&]() {
            std::string out("left-middle-right");
            auto pos = out.find("middle");
            if (pos != std::string::npos) out.replace(pos, std::string("middle").size(), std::string("CENTER"));
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/替换"));

    // Trim
    results.push_back(run_compare("28", "trim",
        R, I,
        [&]() {
            shine::SString s(c.trim_texts[0].c_str());
            auto out = s.trim();
            DoNotOptimize(out);
        },
        [&]() {
            std::string s = c.trim_texts[0];
            auto first = s.find_first_not_of(" \t\n\r\f\v");
            auto last = s.find_last_not_of(" \t\n\r\f\v");
            std::string_view out = (first == std::string::npos)
                ? std::string_view{}
                : std::string_view(s.data() + first, last - first + 1);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/视图"));

    results.push_back(run_compare("28.1", "trim_start",
        R, I,
        [&]() {
            shine::SString s(c.trim_texts[1].c_str());
            auto out = s.trim_start();
            DoNotOptimize(out);
        },
        [&]() {
            std::string s = c.trim_texts[1];
            auto first = s.find_first_not_of(" \t\n\r\f\v");
            std::string_view out = (first == std::string::npos)
                ? std::string_view{}
                : std::string_view(s.data() + first, s.size() - first);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/视图"));

    results.push_back(run_compare("28.2", "trim_end",
        R, I,
        [&]() {
            shine::SString s(c.trim_texts[2].c_str());
            auto out = s.trim_end();
            DoNotOptimize(out);
        },
        [&]() {
            std::string s = c.trim_texts[2];
            auto last = s.find_last_not_of(" \t\n\r\f\v");
            std::string_view out = (last == std::string::npos)
                ? std::string_view{}
                : std::string_view(s.data(), last + 1);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/视图"));

    // Hash
    {
        shine::SString ss = shine::SString::from_utf8("The quick brown fox jumps over the lazy dog");
        std::string stds = "The quick brown fox jumps over the lazy dog";
        std::hash<std::string> std_hasher;

        results.push_back(run_compare("29", "hash",
            R, I,
            [&]() {
                auto h = ss.hash();
                DoNotOptimize(h);
            },
            [&]() {
                auto h = std_hasher(stds);
                DoNotOptimize(h);
            },
            "shine",
            "std",
            "SString/哈希"));
    }

    // Array iteration
    {
        constexpr int N = 10000;
        std::vector<shine::SString> s_arr;
        std::vector<std::string> std_arr;
        s_arr.reserve(N);
        std_arr.reserve(N);

        for (int i = 0; i < N; ++i) {
            char buf[32];
            int len = std::snprintf(buf, sizeof(buf), "item_%05d", i);
            s_arr.emplace_back(std::string_view(buf, static_cast<std::size_t>(len)));
            std_arr.emplace_back(buf, static_cast<std::size_t>(len));
        }

        results.push_back(run_compare("30", "数组遍历: size 累加",
            16, 200,
            [&]() {
                std::size_t total = 0;
                for (const auto& s : s_arr) total += s.size();
                DoNotOptimize(total);
            },
            [&]() {
                std::size_t total = 0;
                for (const auto& s : std_arr) total += s.size();
                DoNotOptimize(total);
            },
            "shine",
            "std",
            "SString/容器"));
    }

    // Hash map
    {
        constexpr int N = 1000;
        results.push_back(run_compare("31", "unordered_map: 插入 + 查找",
            12, 80,
            [&]() {
                std::unordered_map<shine::SString, int> map;
                map.reserve(N);
                for (int i = 0; i < N; ++i) {
                    map.emplace(shine::SString(c.kv_keys[i]), i);
                }
                int sum = 0;
                for (int i = 0; i < N; ++i) {
                    const shine::SString key(c.kv_keys[i]);
                    auto it = map.find(key);
                    if (it != map.end()) sum += it->second;
                }
                DoNotOptimize(sum);
            },
            [&]() {
                std::unordered_map<std::string, int> map;
                map.reserve(N);
                for (int i = 0; i < N; ++i) {
                    map.emplace(c.kv_keys[i], i);
                }
                int sum = 0;
                for (int i = 0; i < N; ++i) {
                    auto it = map.find(c.kv_keys[i]);
                    if (it != map.end()) sum += it->second;
                }
                DoNotOptimize(sum);
            },
            "shine",
            "std",
            "SString/容器"));
    }

    // Long replace stress
    {
        std::string long_str;
        long_str.reserve(70000);
        for (int i = 0; i < 10000; ++i) long_str += "keyword";

        shine::SString long_s = shine::SString::from_utf8(long_str);

        results.push_back(run_compare("32", "长文本 replace 压测 (70KB)",
            10, 100,
            [&]() {
                auto res = long_s.replace("keyword", "REPLACED");
                DoNotOptimize(res);
            },
            [&]() {
                std::string temp = long_str;
                std::size_t pos = 0;
                while ((pos = temp.find("keyword", pos)) != std::string::npos) {
                    temp.replace(pos, 7, "REPLACED");
                    pos += 8;
                }
                DoNotOptimize(temp);
            },
            "shine",
            "std",
            "SString/替换"));

        results.push_back(run_compare("32.1", "长文本 find",
            12, 400,
            [&]() {
                auto pos = long_s.find("keyword");
                DoNotOptimize(pos);
            },
            [&]() {
                auto pos = long_str.find("keyword");
                DoNotOptimize(pos);
            },
            "shine",
            "std",
            "SString/查找"));

        results.push_back(run_compare("32.2", "长文本 contains",
            12, 400,
            [&]() {
                auto ok = long_s.contains("keyword");
                DoNotOptimize(ok);
            },
            [&]() {
                auto ok = long_str.find("keyword") != std::string::npos;
                DoNotOptimize(ok);
            },
            "shine",
            "std",
            "SString/查找"));
    }

    results.push_back(run_compare("32.3", "from_view 工厂",
        R, I,
        [&]() {
            auto s = shine::SString::from_view(shine::STextView::from_cstring("factory_view_payload"));
            DoNotOptimize(s);
        },
        [&]() {
            std::string s("factory_view_payload");
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/工厂"));

    results.push_back(run_compare("32.4", "from_utf8(std::string)",
        R, I,
        [&]() {
            auto s = shine::SString::from_utf8(c.utf8[5]);
            DoNotOptimize(s);
        },
        [&]() {
            std::string s(c.utf8[5]);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "SString/工厂"));

    results.push_back(run_compare("32.5", "view()/as_view()",
        R, I,
        [&]() {
            shine::SString s("view_payload");
            auto v1 = s.view();
            auto v2 = s.as_view();
            DoNotOptimize(v1);
            DoNotOptimize(v2);
        },
        [&]() {
            std::string s("view_payload");
            auto v1 = std::string_view(s);
            auto v2 = std::string_view(s);
            DoNotOptimize(v1);
            DoNotOptimize(v2);
        },
        "shine",
        "std",
        "SString/视图"));

    results.push_back(run_compare("32.6", "operator+ 混合组合",
        R, I,
        [&]() {
            shine::SString a("Hello");
            shine::SString prefix("<<");
            auto out = prefix + (a + "!");
            DoNotOptimize(out);
        },
        [&]() {
            std::string a("Hello");
            std::string prefix("<<");
            auto out = prefix + (a + "!");
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/运算符"));

    results.push_back(run_compare("32.7", "operator== 混合组合",
        R, I,
        [&]() {
            shine::SString a("Hello");
            bool ok = (a == "Hello") && (a == std::string("Hello")) &&
                      (a == std::string_view("Hello")) &&
                      (a == shine::STextView::from_cstring("Hello"));
            DoNotOptimize(ok);
        },
        [&]() {
            std::string a("Hello");
            bool ok = (a == "Hello") && (a == std::string("Hello")) &&
                      (a == std::string_view("Hello"));
            DoNotOptimize(ok);
        },
        "shine",
        "std",
        "SString/运算符"));

    // format
    results.push_back(run_compare("33", "std::format",
        16, 500,
        [&]() {
            shine::SString s("Hello Engine");
            auto out = std::format("name={} value={}", s, 42);
            DoNotOptimize(out);
        },
        [&]() {
            std::string s("Hello Engine");
            auto out = std::format("name={} value={}", s, 42);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "SString/格式化"));
}

void benchmark_stextview_vs_std_string_view(std::vector<ComparisonResult>& results) {
    constexpr int R = 24;
    constexpr int I = 4000;

    const std::string ascii = "The quick brown fox jumps over the lazy dog";
    const std::string utf8 = "你好世界，欢迎来到 ShineEngine。";

    shine::STextView stv_ascii(ascii.data(), ascii.size());
    std::string_view ssv_ascii(ascii.data(), ascii.size());

    shine::STextView stv_utf8(utf8.data(), utf8.size());
    std::string_view ssv_utf8(utf8.data(), utf8.size());

    results.push_back(run_compare("34", "View 构造: ptr+size",
        R, I,
        [&]() {
            shine::STextView v(ascii.data(), ascii.size());
            DoNotOptimize(v);
        },
        [&]() {
            std::string_view v(ascii.data(), ascii.size());
            DoNotOptimize(v);
        },
        "shine",
        "std",
        "STextView/构造"));

    results.push_back(run_compare("35", "View 拷贝",
        R, I,
        [&]() {
            shine::STextView copy = stv_ascii;
            DoNotOptimize(copy);
        },
        [&]() {
            std::string_view copy = ssv_ascii;
            DoNotOptimize(copy);
        },
        "shine",
        "std",
        "STextView/构造"));

    results.push_back(run_compare("36", "size/data 访问",
        R, I,
        [&]() {
            auto n = stv_ascii.size();
            auto p = stv_ascii.data();
            DoNotOptimize(n);
            DoNotOptimize(p);
        },
        [&]() {
            auto n = ssv_ascii.size();
            auto p = ssv_ascii.data();
            DoNotOptimize(n);
            DoNotOptimize(p);
        },
        "shine",
        "std",
        "STextView/访问"));

    results.push_back(run_compare("36.1", "empty / is_valid 等价",
        R, I,
        [&]() {
            auto empty = stv_ascii.empty();
            auto valid = stv_ascii.is_valid();
            DoNotOptimize(empty);
            DoNotOptimize(valid);
        },
        [&]() {
            auto empty = ssv_ascii.empty();
            auto valid = true;
            DoNotOptimize(empty);
            DoNotOptimize(valid);
        },
        "shine",
        "std",
        "STextView/访问"));

    results.push_back(run_compare("36.2", "front/back/byte_at",
        R, I,
        [&]() {
            auto a = stv_ascii.front();
            auto b = stv_ascii.back();
            auto c = stv_ascii.byte_at(4);
            DoNotOptimize(a);
            DoNotOptimize(b);
            DoNotOptimize(c);
        },
        [&]() {
            auto a = ssv_ascii.front();
            auto b = ssv_ascii.back();
            auto c = ssv_ascii[4];
            DoNotOptimize(a);
            DoNotOptimize(b);
            DoNotOptimize(c);
        },
        "shine",
        "std",
        "STextView/访问"));

    results.push_back(run_compare("37", "substr / remove_prefix 等价",
        R, I,
        [&]() {
            auto sub = stv_ascii.substr(4, 5);
            DoNotOptimize(sub);
        },
        [&]() {
            auto sub = ssv_ascii.substr(4, 5);
            DoNotOptimize(sub);
        },
        "shine",
        "std",
        "STextView/视图"));

    results.push_back(run_compare("38", "find substring",
        R, I,
        [&]() {
            auto pos = stv_ascii.find("lazy");
            DoNotOptimize(pos);
        },
        [&]() {
            auto pos = ssv_ascii.find("lazy");
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("39", "find char",
        R, I,
        [&]() {
            auto pos = stv_ascii.find('q');
            DoNotOptimize(pos);
        },
        [&]() {
            auto pos = ssv_ascii.find('q');
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("40", "rfind char",
        R, I,
        [&]() {
            auto pos = stv_ascii.rfind('o');
            DoNotOptimize(pos);
        },
        [&]() {
            auto pos = ssv_ascii.rfind('o');
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("41", "find_first_of",
        R, I,
        [&]() {
            auto pos = stv_ascii.find_first_of("xyz");
            DoNotOptimize(pos);
        },
        [&]() {
            auto pos = ssv_ascii.find_first_of("xyz");
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("41.1", "find_first_not_of",
        R, I,
        [&]() {
            shine::STextView v("   abcdef");
            auto pos = v.find_first_not_of(" ");
            DoNotOptimize(pos);
        },
        [&]() {
            std::string_view v("   abcdef");
            auto pos = v.find_first_not_of(" ");
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("41.2", "find_last_of",
        R, I,
        [&]() {
            auto pos = stv_ascii.find_last_of("aeiou");
            DoNotOptimize(pos);
        },
        [&]() {
            auto pos = ssv_ascii.find_last_of("aeiou");
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("42", "starts_with",
        R, I,
        [&]() {
            auto ok = stv_ascii.starts_with("The");
            DoNotOptimize(ok);
        },
        [&]() {
            auto ok = ssv_ascii.starts_with("The");
            DoNotOptimize(ok);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("43", "ends_with",
        R, I,
        [&]() {
            auto ok = stv_ascii.ends_with("dog");
            DoNotOptimize(ok);
        },
        [&]() {
            auto ok = ssv_ascii.ends_with("dog");
            DoNotOptimize(ok);
        },
        "shine",
        "std",
        "STextView/查找"));

    results.push_back(run_compare("44", "trim vs 手工 string_view trim",
        R, I,
        [&]() {
            shine::STextView v("   hello world   ");
            auto out = v.trim();
            DoNotOptimize(out);
        },
        [&]() {
            std::string_view v("   hello world   ");
            auto first = v.find_first_not_of(" \t\n\r\f\v");
            auto last = v.find_last_not_of(" \t\n\r\f\v");
            std::string_view out = (first == std::string_view::npos)
                ? std::string_view{}
                : v.substr(first, last - first + 1);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "STextView/视图"));

    results.push_back(run_compare("45", "UTF-8 code_point_count vs 手工统计",
        20, 2000,
        [&]() {
            auto n = stv_utf8.code_point_count();
            DoNotOptimize(n);
        },
        [&]() {
            std::size_t count = 0;
            for (unsigned char ch : ssv_utf8) {
                if ((ch & 0xC0u) != 0x80u) ++count;
            }
            DoNotOptimize(count);
        },
        "shine",
        "std",
        "STextView/UTF8"));

    results.push_back(run_compare("46", "UTF-8 find_code_point vs 手工扫描",
        20, 2000,
        [&]() {
            auto pos = stv_utf8.find_code_point(U'世');
            DoNotOptimize(pos);
        },
        [&]() {
            const std::string_view target = "世";
            auto pos = ssv_utf8.find(target);
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/UTF8"));

    results.push_back(run_compare("47", "for_each_code_point vs 手工 UTF-8 前导字节统计",
        20, 1000,
        [&]() {
            std::uint32_t sum = 0;
            stv_utf8.for_each_code_point([&](char32_t cp) {
                sum += static_cast<std::uint32_t>(cp);
            });
            DoNotOptimize(sum);
        },
        [&]() {
            std::uint32_t sum = 0;
            const unsigned char* p = reinterpret_cast<const unsigned char*>(ssv_utf8.data());
            const unsigned char* e = p + ssv_utf8.size();
            while (p < e) {
                unsigned char b0 = *p;
                if (b0 < 0x80) { sum += b0; ++p; }
                else if (b0 < 0xE0 && (e - p) >= 2) { sum += 1; p += 2; }
                else if (b0 < 0xF0 && (e - p) >= 3) { sum += 1; p += 3; }
                else if ((e - p) >= 4) { sum += 1; p += 4; }
                else { ++p; }
            }
            DoNotOptimize(sum);
        },
        "shine",
        "std",
        "STextView/UTF8"));

    results.push_back(run_compare("47.1", "substr_code_points vs UTF-8 子串定位",
        20, 1000,
        [&]() {
            auto sub = stv_utf8.substr_code_points(1, 2);
            DoNotOptimize(sub);
        },
        [&]() {
            const std::string_view target = "好世界";
            auto pos = ssv_utf8.find(target.substr(0, target.size() - std::string_view("界").size()));
            DoNotOptimize(pos);
        },
        "shine",
        "std",
        "STextView/UTF8"));

    results.push_back(run_compare("47.2", "byte_index_from_code_point vs 手工扫描",
        20, 1000,
        [&]() {
            auto idx = stv_utf8.byte_index_from_code_point(2);
            DoNotOptimize(idx);
        },
        [&]() {
            std::size_t cp_index = 0;
            std::size_t byte_index = 0;
            for (; byte_index < ssv_utf8.size() && cp_index < 2; ++byte_index) {
                unsigned char ch = static_cast<unsigned char>(ssv_utf8[byte_index]);
                if ((ch & 0xC0u) != 0x80u) {
                    ++cp_index;
                }
            }
            DoNotOptimize(byte_index);
        },
        "shine",
        "std",
        "STextView/UTF8"));

    results.push_back(run_compare("48", "to_string vs string(view)",
        R, I,
        [&]() {
            static int idx = 0;
            const auto& src = corpus().large[bench_index(idx, corpus().large.size())];
            ++idx;

            shine::STextView v(src.data(), src.size());
            auto s = v.to_string();
            DoNotOptimize(s);
        },
        [&]() {
            static int idx = 0;
            const auto& src = corpus().large[bench_index(idx, corpus().large.size())];
            ++idx;

            std::string_view v(src.data(), src.size());
            std::string s(v);
            DoNotOptimize(s);
        },
        "shine",
        "std",
        "STextView/视图"));

    results.push_back(run_compare("49", "std::format view",
        16, 500,
        [&]() {
            auto out = std::format("view={}", stv_ascii);
            DoNotOptimize(out);
        },
        [&]() {
            auto out = std::format("view={}", ssv_ascii);
            DoNotOptimize(out);
        },
        "shine",
        "std",
        "STextView/格式化"));
}

// =========================================================
// Reporting
// =========================================================

void benchmark() {
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║                    字符串性能对比测试系统                       ║");
    fmt::println("║      shine::SString   vs   std::string                         ║");
    fmt::println("║      shine::STextView vs   std::string_view                    ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    std::vector<ComparisonResult> results;
    results.reserve(64);

    benchmark_sstring_vs_std_string(results);
    benchmark_stextview_vs_std_string_view(results);

    for (const auto& r : results) {
        print_comparison(r);
    }

    print_summary(results, "shine", "std");

    std::vector<ComparisonResult> sstring_results;
    std::vector<ComparisonResult> stextview_results;
    sstring_results.reserve(results.size());
    stextview_results.reserve(results.size());

    for (const auto& r : results) {
        if (r.group.rfind("SString/", 0) == 0) {
            sstring_results.push_back(r);
        } else if (r.group.rfind("STextView/", 0) == 0) {
            stextview_results.push_back(r);
        }
    }

    if (!sstring_results.empty()) {
        fmt::println("\n================ SString 分组胜率 ================\n");
        print_summary(sstring_results, "shine", "std");
    }

    if (!stextview_results.empty()) {
        fmt::println("\n================ STextView 分组胜率 ================\n");
        print_summary(stextview_results, "shine", "std");
    }

    const auto overall_summary = build_summary(results);
    const auto sstring_summary = build_summary(sstring_results);
    const auto stextview_summary = build_summary(stextview_results);

    fmt::println("\n================ 分类统计（总览） ================\n");
    print_group_summary(overall_summary, "shine", "std");

    if (!sstring_results.empty()) {
        fmt::println("\n================ 分类统计（SString） ================\n");
        print_group_summary(sstring_summary, "shine", "std");
    }

    if (!stextview_results.empty()) {
        fmt::println("\n================ 分类统计（STextView） ================\n");
        print_group_summary(stextview_summary, "shine", "std");
    }
}

// =========================================================
// Main
// =========================================================

int main() {
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║              ShineEngine String Test / Benchmark                ║");
    fmt::println("║  SString(32B, 30-char SSO) vs std::string                       ║");
    fmt::println("║  STextView(16B) vs std::string_view                             ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    test_correctness();
    benchmark();

    if (!g_test_ctx.all_passed()) {
        fmt::println("\n!!! {} tests FAILED !!!", g_test_ctx.fail);
        return 1;
    }

    fmt::println("\nAll {} correctness tests passed.", g_test_ctx.pass);
    return 0;
}
