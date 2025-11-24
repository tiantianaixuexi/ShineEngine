#pragma once


#include "shine_common.h"

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


#ifdef _WIN32
    constexpr char SEPARATOR = '\\';
    constexpr char OTHER_SEP = '/';
#else
    constexpr char SEPARATOR = '/';
    constexpr char OTHER_SEP = '\\';
#endif


#ifndef SHINE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#endif