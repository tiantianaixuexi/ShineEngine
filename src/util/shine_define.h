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
using s8 = char;
using u8 = unsigned char;
using s16 = short;
using u16 = unsigned short;
using s32 = int;
using u32 = unsigned int;
using f32 = float;
using f64 = double;

#if defined(_WIN64) || defined(_EMSCRIPT_32_)
    using u64 = unsigned long long;
    using s64 = long long;
#elif defined(__EMSCRIPTEN__)
    using s64 = s32;
    using u64 = u32;
    using f64 = float;
#endif


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
#endif





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

// ============================================================================
// WASM 平台限制说明
// ============================================================================
// 以下 C++20/23 特性在 WebAssembly 中无法使用或存在严重限制：
//
// 1. 异常处理 (Exception Handling)
//    - WASM 通常需要编译时使用 -fno-exceptions 选项
//    - 不支持 try-catch 异常处理机制
//    - 使用 SHINE_HAS_EXCEPTIONS 宏检测
//
// 2. 多线程支持 (Thread Support)
//    - std::thread, std::jthread 不支持
//    - std::mutex, std::atomic 等线程同步原语可能受限
//    - WASM 的多线程支持仍处于实验阶段
//    - 使用 SHINE_HAS_STD_JTHREAD 宏检测
//
// 3. std::format (C++20)
//    - WASM 可能不支持或支持不完整
//    - 建议使用 fmt 库作为替代方案
//    - 使用 SHINE_HAS_STD_FORMAT 宏检测
//
// 4. std::ranges (C++20)
//    - WASM 可能支持不完整
//    - 某些范围算法可能无法正常工作
//    - 使用 SHINE_HAS_STD_RANGES 宏检测
//
// 5. std::coroutine (C++20)
//    - WASM 的控制流限制，协程可能无法直接实现
//    - 需要使用特殊的协程实现或避免使用
//    - 使用 SHINE_HAS_STD_COROUTINE 宏检测
//
// 6. std::source_location (C++20)
//    - WASM 可能不支持或支持不完整
//    - 调试信息可能受限
//    - 使用 SHINE_HAS_STD_SOURCE_LOCATION 宏检测
//
// 7. std::expected (C++23)
//    - C++23 特性，WASM 可能不支持
//    - 需要使用替代方案（如自定义实现）
//    - 使用 SHINE_HAS_STD_EXPECTED 宏检测
//
// 8. std::span (C++20)
//    - WASM 可能支持不完整
//    - 基本功能可用，但某些高级特性可能受限
//    - 使用 SHINE_HAS_STD_SPAN 宏检测
//
// 9. C++20 Modules
//    - WASM 可能支持不完整
//    - 建议使用传统头文件方式
//    - 使用 SHINE_HAS_MODULES 宏检测
//
// 10. 动态库加载
//     - WASM 不支持动态库的加载和使用
//     - 所有代码必须静态链接
//
// 11. 网络功能
//     - WASM 当前不支持直接的网络功能
//     - 需要通过 JavaScript 接口或 Web API 实现
//
// 12. 文件系统操作
//     - WASM 的文件系统访问受限
//     - 需要使用虚拟文件系统或通过 JavaScript 接口
//
// 使用建议：
// - 在代码中使用上述宏进行条件编译
// - 为 WASM 平台提供替代实现
// - 避免在 WASM 目标中使用不支持的特性
// ============================================================================

