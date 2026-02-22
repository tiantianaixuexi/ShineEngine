#pragma once

#if defined(__clang__) || defined(__GNUC__)
    #define FORCEINLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define FORCEINLINE  [[msvc::forceinline]]
#else
    #define FORCEINLINE inline
#endif

// #if defined(__clang__) || defined(__GNUC__)
//     #define ASSUME(condition) __builtin_assume(condition)
// #elif defined(_MSC_VER)
//     #define ASSUME(condition) __assume(condition)
// #else
//     #define ASSUME(condition) ((void)0)
// #endif

#define ASSUME(...) [[assume(__VA_ARGS__)]]

#if defined(__clang__) || defined(__GNUC__)
    #define OFFSETOF(s,m) __builtin_offsetof(s,m)
#elif defined(_MSC_VER)
    #ifdef __cplusplus
        #define OFFSETOF(s,m) ((::size_t)&reinterpret_cast<char const volatile&>((((s*)0)->m)))
    #else
        #define OFFSETOF(s,m) ((size_t)&(((s*)0)->m))
    #endif
#endif