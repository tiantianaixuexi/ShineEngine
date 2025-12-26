#pragma once

// 平台检测（确保定义已设置）
#if defined(_WIN32) && !defined(_WIN64)
    #ifndef SHINE_PLATFORM_WIN32
    #define SHINE_PLATFORM_WIN32 1
    #endif
    #ifndef SHINE_PLATFORM_WIN
    #define SHINE_PLATFORM_WIN 1
    #endif
#elif defined(_WIN64)
    #ifndef SHINE_PLATFORM_WIN64
    #define SHINE_PLATFORM_WIN64 1
    #endif
    #ifndef SHINE_PLATFORM_WIN
    #define SHINE_PLATFORM_WIN 1
    #endif
#elif defined(__linux__) && !defined(__LP64__)
    #ifndef SHINE_PLATFORM_LINUX32
    #define SHINE_PLATFORM_LINUX32 1
    #endif
#elif defined(__linux__) && defined(__LP64__)
    #ifndef SHINE_PLATFORM_LINUX64
    #define SHINE_PLATFORM_LINUX64 1
    #endif
#elif defined(__EMSCRIPTEN__)
    #ifndef SHINE_PLATFORM_WASM
    #define SHINE_PLATFORM_WASM 1
    #endif
#endif

// 编译器特定属性
#ifdef __has_cpp_attribute
    #if __has_cpp_attribute(assume)
        #define SHINE_ASSUME(expr) [[assume(expr)]]
    #else
        #define SHINE_ASSUME(expr)
    #endif
#endif

// CPU架构宏
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
    #define PLATFORM_CPU_ARM_FAMILY 1
    #define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 1
    #define PLATFORM_ENABLE_VECTORINTRINSICS 1
#elif (defined(_M_IX86) || defined(_M_X64))
    #define PLATFORM_CPU_X86_FAMILY 1
    #define PLATFORM_ENABLE_VECTORINTRINSICS 1
#endif

// API导出/导入宏
#if defined(SHINE_ENGINE_EXPORTS)
    #define SHINE_API __declspec(dllexport)
#else
    #define SHINE_API __declspec(dllimport)
#endif

// 功能检测
// WASM不支持string_view（C++17特性不完全可用）
#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
    #ifndef SHINE_HAS_STRING_VIEW
    #define SHINE_HAS_STRING_VIEW 0
    #endif
#else
    #ifndef SHINE_HAS_STRING_VIEW
    #define SHINE_HAS_STRING_VIEW 1
    #endif
#endif

// 字符串类型头文件
#include <string>
#include <type_traits>
#if SHINE_HAS_STRING_VIEW
    #include <string_view>
#endif

// 类型定义
using s8 = signed char;
using u8 = unsigned char;
using s16 = short;
using u16 = unsigned short;
using s32 = int;
using u32 = unsigned int;
using s64 = long long ;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;




// 路径分隔符
#ifdef _WIN32
    constexpr char SEPARATOR = '\\';
    constexpr char OTHER_SEP = '/';
#else
    constexpr char SEPARATOR = '/';
    constexpr char OTHER_SEP = '\\';
#endif

// Windows特定头文件
#ifdef SHINE_PLATFORM_WIN
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
	#include <shellapi.h>
#endif




static_assert(sizeof(s8) == 1);
static_assert(sizeof(s16) == 2);
static_assert(sizeof(s32) == 4);
static_assert(sizeof(s64) == 8);

static_assert(sizeof(u8) == 1);
static_assert(sizeof(u16) == 2);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(u64) == 8);

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

#define S8_MIN    (-128)
#define S8_max    (127)

#define S16_MIN   (-32768)
#define S16_MAX   (32767)

#define S32_MIN   (-2147483647 - 1)
#define S32_MAX   (2147483647)

#define S64_MIN   (-9223372036854775807LL - 1)
#define S64_MAX   (9223372036854775807LL)

/* unsigned */
#define U8_MAX    (255u)
#define U16_MAX   (65535u)
#define U32_MAX   (4294967295u)
#define U64_MAX   (18446744073709551615ULL)




#if SHINE_HAS_STRING_VIEW
    using SString = std::string_view;           // 只读字符串视图，零开销
#else
    using SString = std::string;                // WASM 回退到 string
#endif



#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
    // C++20 std::format - WASM 可能不支持或支持不完整
    #ifndef SHINE_HAS_STD_FORMAT
    #define SHINE_HAS_STD_FORMAT 0
    #endif

    // C++20 std::ranges - WASM 可能支持不完整
    #ifndef SHINE_HAS_STD_RANGES
    #define SHINE_HAS_STD_RANGES 0
    #endif

    // C++20 std::jthread, std::stop_token - WASM 不支持多线程
    #ifndef SHINE_HAS_STD_JTHREAD
    #define SHINE_HAS_STD_JTHREAD 0
    #endif

    // C++20 std::coroutine - WASM 的控制流限制，可能无法直接实现
    #ifndef SHINE_HAS_STD_COROUTINE
    #define SHINE_HAS_STD_COROUTINE 0
    #endif

    // C++20 std::source_location - WASM 可能不支持或支持不完整
    #ifndef SHINE_HAS_STD_SOURCE_LOCATION
    #define SHINE_HAS_STD_SOURCE_LOCATION 0
    #endif

    // C++23 std::expected - C++23 特性，WASM 可能不支持
    #ifndef SHINE_HAS_STD_EXPECTED
    #define SHINE_HAS_STD_EXPECTED 0
    #endif

    // C++20 std::span - WASM 可能支持不完整
    #ifndef SHINE_HAS_STD_SPAN
    #define SHINE_HAS_STD_SPAN 0
    #endif

    // C++20 modules - WASM 可能支持不完整
    #ifndef SHINE_HAS_MODULES
    #define SHINE_HAS_MODULES 0
    #endif

    // C++ 异常处理 - WASM 通常需要 -fno-exceptions，不支持异常
    #ifndef SHINE_HAS_EXCEPTIONS
    #define SHINE_HAS_EXCEPTIONS 0
    #endif

    // C++20 concepts - 基本支持，但某些高级用法可能有限制
    #ifndef SHINE_HAS_CONCEPTS
    #define SHINE_HAS_CONCEPTS 1  // 基本支持
    #endif

    // C++20 consteval, constinit - 基本支持
    #ifndef SHINE_HAS_CONSTEVAL
    #define SHINE_HAS_CONSTEVAL 1
    #endif

#else
    // 非 WASM 平台，默认支持这些特性（根据编译器版本）
    #ifndef SHINE_HAS_STD_FORMAT
    #define SHINE_HAS_STD_FORMAT 1
    #endif

    #ifndef SHINE_HAS_STD_RANGES
    #define SHINE_HAS_STD_RANGES 1
    #endif

    #ifndef SHINE_HAS_STD_JTHREAD
    #define SHINE_HAS_STD_JTHREAD 1
    #endif

    #ifndef SHINE_HAS_STD_COROUTINE
    #define SHINE_HAS_STD_COROUTINE 1
    #endif

    #ifndef SHINE_HAS_STD_SOURCE_LOCATION
    #define SHINE_HAS_STD_SOURCE_LOCATION 1
    #endif

    #ifndef SHINE_HAS_STD_EXPECTED
    // C++23 特性，需要编译器支持
    #if __cplusplus >= 202302L
        #define SHINE_HAS_STD_EXPECTED 1
    #else
        #define SHINE_HAS_STD_EXPECTED 0
    #endif
    #endif

    #ifndef SHINE_HAS_STD_SPAN
    #define SHINE_HAS_STD_SPAN 1
    #endif

    #ifndef SHINE_HAS_MODULES
    #if __cpp_modules >= 201907L
        #define SHINE_HAS_MODULES 1
    #else
        #define SHINE_HAS_MODULES 0
    #endif
    #endif

    #ifndef SHINE_HAS_EXCEPTIONS
    #ifdef __cpp_exceptions
        #define SHINE_HAS_EXCEPTIONS 1
    #else
        #define SHINE_HAS_EXCEPTIONS 0
    #endif
    #endif

    #ifndef SHINE_HAS_CONCEPTS
    #if __cpp_concepts >= 201907L
        #define SHINE_HAS_CONCEPTS 1
    #else
        #define SHINE_HAS_CONCEPTS 0
    #endif
    #endif

    #ifndef SHINE_HAS_CONSTEVAL
    #if __cpp_consteval >= 201811L
        #define SHINE_HAS_CONSTEVAL 1
    #else
        #define SHINE_HAS_CONSTEVAL 0
    #endif
    #endif

#endif
