#pragma once

// wasm_compat.h
// Small helpers for wasm builds compiled with -nostdlib (no libc, no S
// TL).
// Put all the "boilerplate" here so other headers can stay clean.

// Builtin types from Clang (work under -nostdlib).



namespace shine::wasm {

using size_t = __SIZE_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;


// The module provides malloc/free (we implement them in wasm_runtime.cpp).
extern "C" void* malloc(size_t);
extern "C" void free(void*);

static inline void* raw_malloc(size_t n) noexcept { return shine::wasm::malloc(n); }
static inline void raw_free(void* p) noexcept { shine::wasm::free(p); }

#define raw_memset(p, v, n)  __builtin_memset(p,v,n)
#define raw_memcpy(dst, src, n)  __builtin_memcpy(dst, src, n)
#define raw_memmove(dst, src, n) __builtin_memmove(dst, src, n)
#define raw_memcmp(dst, src, n) __builtin_memcmp(dst,src,n)


//#define raw_strlen(s) __builtin_strlen(s)

extern "C" shine::wasm::size_t my_strlen(const char* s);

#define raw_strlen(s) shine::wasm::my_strlen(s)

#define ptr_i32(p) static_cast<unsigned int>(reinterpret_cast<shine::wasm::uintptr_t>(p))


#if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#define SHINE_CONSTINIT constinit
#else
#define SHINE_CONSTINIT
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#define SHINE_INLINE_VAR inline
#else
#define SHINE_INLINE_VAR static
#endif


#define f2i(f) __builtin_bit_cast(int, f)

#define unreachable() __builtin_unreachable()
#define assume(cond) __builtin_assume(cond)
#define expect(cond, likely) __builtin_expect((cond), (likely))
#define likely(cond) __builtin_expect(!!(cond), 1)
#define unlikely(cond) __builtin_expect(!!(cond), 0)

void* raw_realloc(void* p, size_t oldSize, size_t newSize) noexcept;
void svector_reserve_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int length, unsigned int newCap, unsigned int elemSize);
void svector_grow_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int length, unsigned int needCap, unsigned int elemSize);
void svector_push_back_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int* length_ref, const void* elem_data, unsigned int elemSize);
bool svector_erase_unordered_at_impl(void* pointer, unsigned int* length_ref, unsigned int idx, unsigned int elemSize);
bool svector_erase_first_unordered_impl(void* pointer, unsigned int* length_ref, const void* elem_data, unsigned int elemSize);

} // namespace shine::wasm

// Minimal placement-new definitions for -nostdlib builds.
inline void* operator new(shine::wasm::size_t, void* p) noexcept { return p; }
inline void operator delete(void*, void*) noexcept {}

