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
#if defined(SHINE_BUILD_SHARED)
#if defined(SHINE_EXPORTS)
	#define SHINE_API __declspec(dllexport)
#else
	#define SHINE_API __declspec(dllimport)
#endif
#else
    // 构建静态库
#define SHINE_API
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
using TChar = char16_t;



// 路径分隔符
#ifdef _WIN32
    constexpr char SEPARATOR = '\\';
    constexpr char OTHER_SEP = '/';
#else
    constexpr char SEPARATOR = '/';
    constexpr char OTHER_SEP = '\\';
#endif

//// Windows特定头文件
//#ifdef SHINE_PLATFORM_WIN
//    #ifndef WIN32_LEAN_AND_MEAN
//    #define WIN32_LEAN_AND_MEAN
//    #endif
//    #include <Windows.h>
//	#include <shellapi.h>
//#endif
//


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




#define SHINE_NAMESPACE(name) namespace shine name {
#define SHINE_NAMESPACE_END   };


#ifdef SHINE_BUILD_MODULE
#ifndef SHINE_USE_MODULE
#define SHINE_USE_MODULE 1
#endif
#else
#ifndef SHINE_USE_MODULE
#define SHINE_USE_MODULE 0
#endif
#endif

//#ifdef SHINE_USE_MODULE
//
//#define SHINE_IMPORT_STD import std;
//#define SHINE_MODULE_EXPORT(name) export module shine##name;
//
//#else
//
//#define SHINE_MODULE_EXPORT
//#define SHINE_MODULE_END
//#define SHINE_IMPORT_STD
//
//#endif

