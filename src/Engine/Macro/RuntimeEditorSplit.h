#pragma once

#ifndef BUILD_RUNTIME
#define BUILD_RUNTIME 1
#endif

#if BUILD_RUNTIME
#define RUNTIME_DATA(...) __VA_ARGS__
#else
#define RUNTIME_DATA(...)
#endif

#if defined(BUILD_EDITOR) && BUILD_EDITOR
#define EDITOR_DATA(...) __VA_ARGS__
#else
#define EDITOR_DATA(...)
#endif
