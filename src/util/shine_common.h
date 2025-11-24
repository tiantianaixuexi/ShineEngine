#pragma once


#if defined(_WIN32) && !defined(_WIN64)
#define SHINE_PLATFORM_WIN32 1
#define SHINE_PLATFORM_WIN   1
#elif defined(_WIN64)
#define SHINE_PLATFORM_WIN64 1
#elif defined(__linux__) && !defined(__LP64__)
#define SHINE_PLATFORM_LINUX32 1
#elif defined(__linux__) && defined(__LP64__)
#define SHINE_PLATFORM_LINUX64 1
#elif defined(__EMSCRIPTEN__)
#define SHINE_PLATFORM_WASM 1
#endif

// Compiler-Specific Attributes
#ifdef __has_cpp_attribute
#if __has_cpp_attribute(assume)
#define SHINE_ASSUME(expr) [[assume(expr)]]
#else
#define SHINE_ASSUME(expr)
#endif
#endif

// CPU Architecture Macros
#if defined(_M_ARM) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define PLATFORM_CPU_ARM_FAMILY 1
#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON 1
#define PLATFORM_ENABLE_VECTORINTRINSICS 1
#elif (defined(_M_IX86) || defined(_M_X64))
#define PLATFORM_CPU_X86_FAMILY 1
#define PLATFORM_ENABLE_VECTORINTRINSICS 1
#endif




#if defined(SHINE_ENGINE_EXPORTS)

#define SHINE_API __declspec(dllexport)
#else

#define SHINE_API __declspec(dllimport)
#endif



//#define SHINE_IMPORT_STD


