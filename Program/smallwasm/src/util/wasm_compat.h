#pragma once

// wasm_compat.h
// Small helpers for wasm builds compiled with -nostdlib (no libc, no S
// TL).
// Put all the "boilerplate" here so other headers can stay clean.


namespace shine::wasm {

// Builtin types from Clang (work under -nostdlib).
using size_t = __SIZE_TYPE__;
using uintptr_t = __UINTPTR_TYPE__;

// The module provides malloc/free (we implement them in gl_ctx.cpp for this demo).
extern "C" void* malloc(size_t);
extern "C" void free(void*);

static inline void* raw_malloc(size_t n) noexcept { return shine::wasm::malloc(n); }
static inline void raw_free(void* p) noexcept { shine::wasm::free(p); }

static inline void raw_memset(void* p, unsigned char v, size_t n) noexcept {
  unsigned char* b = (unsigned char*)p;
  for (size_t i = 0; i < n; ++i) b[i] = v;
}

static inline void raw_memcpy(void* dst, const void* src, size_t n) noexcept {
  unsigned char* d = (unsigned char*)dst;
  const unsigned char* s = (const unsigned char*)src;
  for (size_t i = 0; i < n; ++i) d[i] = s[i];
}

static inline int raw_memcmp(const void* s1, const void* s2, size_t n) noexcept {
  const unsigned char* p1 = (const unsigned char*)s1;
  const unsigned char* p2 = (const unsigned char*)s2;
  for (size_t i = 0; i < n; ++i) {
    if (p1[i] != p2[i]) return p1[i] - p2[i];
  }
  return 0;
}

static inline int raw_strlen(const char* s) noexcept {
  if (!s) return 0;
  int n = 0;
  while (s[n]) ++n;
  return n;
}

#define ptr_i32(p) static_cast<unsigned int>(reinterpret_cast<uintptr_t>(p))


#define f2i(f) __builtin_bit_cast(int, f)

void svector_reserve_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int length, unsigned int newCap, unsigned int elemSize);

} // namespace shine::wasm

