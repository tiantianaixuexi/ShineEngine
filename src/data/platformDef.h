#pragma once

// Platform definitions for WebAssembly and WebGL
#define SHINE_PLATFORM_WASM

// WebGL version definitions
#define SHINE_WEBGL_VERSION 2
#define SHINE_WEBGL_1_0_SUPPORT
#define SHINE_WEBGL_2_0_SUPPORT

// WebGL 2.0 does not support 64-bit floating point operations
#define SHINE_NO_64BIT_FLOAT_SUPPORT
#define SHINE_WEBGL_NO_DOUBLE_PRECISION



#if defined(SHINE_PLATFORM_WASM) || defined(__EMSCRIPTEN__)
#define f64 = float;
#endif
