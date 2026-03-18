#include <string>
#include <string_view>
#include <type_traits>

#include "../../src/string/shine_string.h"
#include "../common/test_benchmark_framework.h"
#include "fmt/base.h"
#include "fmt/format.h"

using shine::SString;
using shine::STextView;

static shine::test::TestContext g_ctx;

#define CHECK(name, cond) SHINE_TEST_CHECK(g_ctx, name, cond)

// =========================================================
// Acceptor helpers — each takes its parameter by value via
// copy-initialization, which exercises implicit conversion.
// =========================================================

static STextView      accept_textview(STextView tv)       { return tv; }
static SString        accept_sstring (SString   s)        { return s;  }
static std::string_view accept_sv    (std::string_view sv){ return sv; }

// =========================================================
// Compile-time conversion matrix (static_assert)
//
// std::is_convertible_v<From,To> models copy-initialization:
//   To obj = std::declval<From>();
// Only implicit conversions pass.  Explicit constructors do NOT.
// =========================================================

// ---- STextView as destination ----
static_assert( std::is_convertible_v<const char*,      STextView>,
    "const char* -> STextView must be implicit");
static_assert( std::is_convertible_v<std::string_view, STextView>,
    "std::string_view -> STextView must be implicit");
static_assert( std::is_convertible_v<SString,          STextView>,
    "SString -> STextView must be implicit (operator STextView())");
// std::string has no direct STextView(const std::string&) ctor;
// the chain std::string->string_view->STextView requires two UDC.
static_assert(!std::is_convertible_v<std::string,      STextView>,
    "std::string -> STextView must NOT be implicit (two UDC chain)");

// ---- STextView as source ----
static_assert( std::is_convertible_v<STextView, std::string_view>,
    "STextView -> std::string_view must be implicit (operator std::string_view())");
static_assert(!std::is_convertible_v<STextView, std::string>,
    "STextView -> std::string must NOT be implicit (no implicit ctor)");
static_assert(!std::is_convertible_v<STextView, SString>,
    "STextView -> SString must NOT be implicit (explicit SString(STextView))");

// ---- SString as destination ----
static_assert( std::is_convertible_v<const char*, SString>,
    "const char* -> SString must be implicit");
static_assert(!std::is_convertible_v<STextView,          SString>,
    "STextView -> SString must NOT be implicit (explicit ctor)");
static_assert(!std::is_convertible_v<std::string_view,  SString>,
    "std::string_view -> SString must NOT be implicit (explicit ctor)");
static_assert(!std::is_convertible_v<std::string,       SString>,
    "std::string -> SString must NOT be implicit (explicit ctor)");

// ---- SString as source ----
static_assert( std::is_convertible_v<SString, STextView>,
    "SString -> STextView must be implicit (operator STextView())");
// Chaining SString->STextView->string_view needs two user-defined conversions.
static_assert(!std::is_convertible_v<SString, std::string_view>,
    "SString -> std::string_view must NOT be implicit (two-UDC chain blocked by standard)");
static_assert(!std::is_convertible_v<SString, std::string>,
    "SString -> std::string must NOT be implicit");

// ---- Standard-library baselines ----
static_assert( std::is_convertible_v<const char*,      std::string_view>,
    "const char* -> std::string_view: standard implicit");
static_assert( std::is_convertible_v<std::string,      std::string_view>,
    "std::string -> std::string_view: standard implicit (operator string_view)");
static_assert( std::is_convertible_v<const char*,      std::string>,
    "const char* -> std::string: standard implicit");
static_assert(!std::is_convertible_v<std::string_view, std::string>,
    "std::string_view -> std::string: standard explicit ctor");

// =========================================================
// Runtime helpers — print the conversion matrix as a table
// =========================================================

static void print_matrix_header(std::string_view lhs_label, std::string_view rhs_label)
{
    fmt::println(
        "  {:<20} -> {:<20}",
        lhs_label, rhs_label);
}

// =========================================================
// Section 1: implicit conversions that MUST work
// =========================================================

static void test_implicit_positive()
{
    fmt::println("\n=== Implicit conversions that MUST succeed ===\n");

    const char*      cstr  = "hello world";
    std::string      stdstr("hello world");
    std::string_view sv   ("hello world");
    SString          ss   ("hello world");
    STextView        stv  (ss);

    // ---- const char* -> STextView ----
    {
        STextView tv = cstr;
        CHECK("const char* -> STextView  (value)", tv == "hello world");
        CHECK("const char* -> STextView  (size)",  tv.size() == 11);
        // via function call (copy-initialization)
        STextView tv2 = accept_textview(cstr);
        CHECK("const char* -> STextView  (fn arg)", tv2 == "hello world");
    }

    // ---- std::string_view -> STextView ----
    {
        STextView tv = sv;
        CHECK("std::string_view -> STextView  (value)", tv == "hello world");
        STextView tv2 = accept_textview(sv);
        CHECK("std::string_view -> STextView  (fn arg)", tv2 == "hello world");
    }

    // ---- SString -> STextView ----
    {
        STextView tv = ss;
        CHECK("SString -> STextView  (value)", tv == "hello world");
        STextView tv2 = accept_textview(ss);
        CHECK("SString -> STextView  (fn arg)", tv2 == "hello world");
    }

    // ---- STextView -> std::string_view ----
    {
        std::string_view r = stv;
        CHECK("STextView -> std::string_view  (value)", r == "hello world");
        std::string_view r2 = accept_sv(stv);
        CHECK("STextView -> std::string_view  (fn arg)", r2 == "hello world");
    }

    // ---- const char* -> SString ----
    {
        SString s = cstr;
        CHECK("const char* -> SString  (value)", s == "hello world");
        SString s2 = accept_sstring(cstr);
        CHECK("const char* -> SString  (fn arg)", s2 == "hello world");
    }

    // ---- std::string -> std::string_view (stdlib baseline) ----
    {
        std::string_view r = stdstr;
        CHECK("std::string -> std::string_view  (value)", r == "hello world");
    }
}

// =========================================================
// Section 2: explicit-only conversions (constructible but NOT
//            implicitly convertible — must be spelled out)
// =========================================================

static void test_explicit_conversions()
{
    fmt::println("\n=== Explicit-only (non-implicit) conversions ===\n");

    const std::string stdstr("engine");
    const std::string_view sv("engine");
    SString           ss("engine");
    STextView         stv(ss);

    // SString from STextView  (explicit ctor)
    {
        SString s(stv);     // explicit construction — OK
        CHECK("SString(STextView)  explicit construct (value)", s == "engine");
    }

    // SString from std::string_view  (explicit ctor)
    {
        SString s(sv);
        CHECK("SString(string_view)  explicit construct (value)", s == "engine");
    }

    // SString from std::string  (explicit ctor)
    {
        SString s(stdstr);
        CHECK("SString(std::string)  explicit construct (value)", s == "engine");
    }

    // std::string from std::string_view  (explicit ctor)
    {
        std::string s(sv);
        CHECK("std::string(string_view)  explicit construct (value)", s == "engine");
    }

    // STextView from std::string — must use factory
    {
        STextView tv = STextView::from_sv(stdstr);   // via string_view conversion
        CHECK("STextView::from_sv(std::string) (value)", tv == "engine");
    }
}

// =========================================================
// Section 3: equality-operator cross-type checks
// =========================================================

static void test_cross_type_equality()
{
    fmt::println("\n=== Cross-type equality operators ===\n");

    const char*      cstr = "shine";
    std::string      stdstr("shine");
    std::string_view sv("shine");
    SString          ss("shine");
    STextView        stv(ss);

    // STextView comparisons
    CHECK("STextView == STextView",      stv == STextView("shine"));
    CHECK("STextView == const char*",    stv == cstr);
    CHECK("const char* == STextView",    cstr == stv);

    // SString comparisons
    CHECK("SString == SString",          ss == SString("shine"));
    CHECK("SString == STextView",        ss == stv);
    CHECK("STextView == SString",        stv == ss);
    CHECK("SString == std::string_view", ss == sv);
    CHECK("std::string_view == SString", sv == ss);
    CHECK("SString == const char*",      ss == cstr);
    CHECK("const char* == SString",      cstr == ss);

    // STextView -> string_view equality bridging
    {
        std::string_view bridge = stv;   // implicit
        CHECK("STextView->string_view used in == string", bridge == stdstr);
    }
}

// =========================================================
// Section 4: SSO / heap boundary — verify implicit conversion
//            preserves full content for all size classes
// =========================================================

static void test_size_boundaries()
{
    fmt::println("\n=== Size boundary conversion correctness ===\n");

    // SSO range (≤ 30 bytes)
    const std::string sso_str(20, 'x');
    {
        SString   ss  = SString(sso_str.c_str()); // explicit SString(const char*)
        STextView tv  = ss;                        // SString -> STextView
        std::string_view sv = tv;                  // STextView -> string_view
        CHECK("SSO: const char* -> SString -> STextView -> string_view (size)",
            sv.size() == 20);
        CHECK("SSO: content preserved", sv == std::string_view(sso_str));
    }

    // Exactly at SSO boundary (30 bytes)
    {
        const std::string edge30(30, 'y');
        SString   ss  (edge30.c_str());
        STextView tv  = ss;
        CHECK("SSO edge 30: SString -> STextView size", tv.size() == 30);
        CHECK("SSO edge 30: content", tv == STextView(edge30.c_str(), 30));
    }

    // Heap range (> 30 bytes)
    {
        const std::string heap_str(64, 'z');
        SString   ss  (heap_str.c_str());
        STextView tv  = ss;
        std::string_view sv = tv;
        CHECK("Heap: SString -> STextView -> string_view (size)", sv.size() == 64);
        CHECK("Heap: content preserved", sv == std::string_view(heap_str));
    }

    // Empty string
    {
        SString   ss("");
        STextView tv  = ss;
        std::string_view sv = tv;
        CHECK("Empty: SString -> STextView -> string_view (size 0)", sv.size() == 0);
        CHECK("Empty: content", sv.empty());
    }
}

// =========================================================
// Section 5: UTF-8 content round-trip through implicit paths
// =========================================================

static void test_utf8_roundtrip()
{
    fmt::println("\n=== UTF-8 content through implicit conversion paths ===\n");

    const char* utf8_cstr = "你好，引擎！";        // 18 UTF-8 bytes

    SString ss(utf8_cstr);                         // const char* -> SString (explicit)
    STextView tv = ss;                             // SString -> STextView  (implicit)
    std::string_view sv = tv;                      // STextView -> string_view (implicit)

    CHECK("UTF-8: const char* -> SString size",       ss.size() == std::char_traits<char>::length(utf8_cstr));
    CHECK("UTF-8: SString -> STextView size",         tv.size() == ss.size());
    CHECK("UTF-8: STextView -> string_view equality", sv == std::string_view(utf8_cstr));

    // Explicit SString from std::string_view preserves UTF-8
    {
        SString ss2(sv);
        CHECK("UTF-8: explicit SString(string_view) preserves content", ss2 == ss);
    }
}

// =========================================================
// Section 6: compile-time summary table (printed at runtime)
// =========================================================

static void print_conversion_table()
{
    fmt::println("\n");
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║               Implicit Conversion Matrix Summary                ║");
    fmt::println("╠═══════════════════════╦════════════════════════╦════════════════╣");
    fmt::println("║  From                 ║  To                    ║  Implicit?     ║");
    fmt::println("╠═══════════════════════╬════════════════════════╬════════════════╣");

    auto row = [](std::string_view from, std::string_view to, bool yes)
    {
        fmt::println("║  {:<21} ║  {:<22} ║  {:<14} ║",
            from, to, yes ? "YES" : "NO (explicit)");
    };

    row("const char*",      "STextView",         true);
    row("std::string_view", "STextView",         true);
    row("SString",          "STextView",         true);
    row("std::string",      "STextView",         false);
    fmt::println("╠═══════════════════════╬════════════════════════╬════════════════╣");
    row("STextView",        "std::string_view",  true);
    row("STextView",        "SString",           false);
    row("STextView",        "std::string",       false);
    fmt::println("╠═══════════════════════╬════════════════════════╬════════════════╣");
    row("const char*",      "SString",           false);
    row("STextView",        "SString",           false);
    row("std::string_view", "SString",           false);
    row("std::string",      "SString",           false);
    fmt::println("╠═══════════════════════╬════════════════════════╬════════════════╣");
    row("SString",          "STextView",         true);
    row("SString",          "std::string_view",  false);
    row("SString",          "std::string",       false);
    fmt::println("╠═══════════════════════╬════════════════════════╬════════════════╣");
    row("const char*",      "std::string_view",  true);
    row("std::string",      "std::string_view",  true);
    row("const char*",      "std::string",       true);
    row("std::string_view", "std::string",       false);
    fmt::println("╚═══════════════════════╩════════════════════════╩════════════════╝");
    fmt::println("");
}

// =========================================================
// main
// =========================================================

int main()
{
    fmt::println("╔══════════════════════════════════════════════════════════════════╗");
    fmt::println("║          ShineEngine  StringConversionTest                      ║");
    fmt::println("║  SString / STextView / std::string / std::string_view / char*   ║");
    fmt::println("╚══════════════════════════════════════════════════════════════════╝\n");

    fmt::println("Compile-time static_assert matrix: ALL PASSED (compiled successfully)\n");

    test_implicit_positive();
    test_explicit_conversions();
    test_cross_type_equality();
    test_size_boundaries();
    test_utf8_roundtrip();
    print_conversion_table();

    g_ctx.print_summary("Conversion Correctness");

    if (!g_ctx.all_passed())
    {
        fmt::println("!!! {} test(s) FAILED !!!", g_ctx.fail);
        return 1;
    }

    fmt::println("All {} correctness tests passed.", g_ctx.pass);
    return 0;
}
