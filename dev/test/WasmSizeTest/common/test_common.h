// WASM Size Test - 公共头文件
// 所有 test 都会自动包含这个目录

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

// 定义 size_t（如果没有的话）
#ifndef _SIZE_T_DEFINED
    typedef __SIZE_TYPE__ size_t;
    #define _SIZE_T_DEFINED
#endif

// 定义 NULL
#ifndef NULL
    #define NULL ((void*)0)
#endif

// 简单的断言宏（wasm 环境下没有标准库）
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            __builtin_trap(); \
        } \
    } while(0)

// 防止函数被优化掉的宏
#define TEST_NO_OPTIMIZE __attribute__((used))

// 强制内联
#define TEST_INLINE __attribute__((always_inline)) inline

#endif // TEST_COMMON_H
