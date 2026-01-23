#include "wasm_compat.h"

// wasm_runtime.cpp
// Minimal runtime bits for -nostdlib wasm32 builds:
// - malloc/free implementation
// - C++ atexit stubs (avoid importing __cxa_atexit)

using uintptr_t = __UINTPTR_TYPE__;
using size_t = __SIZE_TYPE__;

extern "C" {
extern unsigned char __heap_base;
unsigned long __builtin_wasm_memory_size(int) noexcept;
unsigned long __builtin_wasm_memory_grow(int, unsigned long) noexcept;
}

static inline uintptr_t align_up(uintptr_t x, uintptr_t a) { return (x + (a - 1)) & ~(a - 1); }
static constexpr uintptr_t kAlign = 16;
static constexpr uintptr_t kPageSize = 64u * 1024u;

struct Block {
  size_t size;
  Block* next;
};

static uintptr_t g_heap_ptr = 0;
static Block* g_free_list = nullptr;
static uintptr_t g_heap_base = 0;
static unsigned int g_alloc_count = 0;
static unsigned int g_free_count = 0;
static unsigned int g_alloc_fail_count = 0;

static void ensure_heap_inited() {
  if (g_heap_ptr == 0) {
    g_heap_base = align_up(reinterpret_cast<uintptr_t>(&__heap_base), kAlign);
    g_heap_ptr = g_heap_base;
  }
}

static bool ensure_capacity(uintptr_t need_end) {
  unsigned long pages_now = __builtin_wasm_memory_size(0);
  uintptr_t bytes_now = static_cast<uintptr_t>(pages_now) * kPageSize;
  if (need_end <= bytes_now) return true;
  uintptr_t need_bytes = need_end - bytes_now;
  unsigned long need_pages = static_cast<unsigned long>((need_bytes + kPageSize - 1) / kPageSize);
  unsigned long old = __builtin_wasm_memory_grow(0, need_pages);
  return old != 0xFFFFFFFFul;
}

extern "C" void* malloc(size_t size) {
  if (size == 0) return nullptr;
  ensure_heap_inited();
  size = static_cast<size_t>(align_up(static_cast<uintptr_t>(size), kAlign));

  Block** prevp = &g_free_list;
  for (Block* b = g_free_list; b; b = b->next) {
    if (b->size >= size) {
      *prevp = b->next;

      uintptr_t b_addr = reinterpret_cast<uintptr_t>(b);
      uintptr_t payload_addr = b_addr + sizeof(Block);
      uintptr_t new_payload_addr = payload_addr + size;
      uintptr_t new_block_addr = align_up(new_payload_addr, kAlign);
      uintptr_t b_end = payload_addr + b->size;

      if (new_block_addr + sizeof(Block) + kAlign <= b_end) {
        Block* nb = reinterpret_cast<Block*>(new_block_addr);
        uintptr_t nb_payload = new_block_addr + sizeof(Block);
        nb->size = static_cast<size_t>(b_end - nb_payload);
        nb->next = g_free_list;
        g_free_list = nb;
        b->size = size;
      }
      ++g_alloc_count;
      return reinterpret_cast<void*>(payload_addr);
    }
    prevp = &b->next;
  }

  uintptr_t block_addr = align_up(g_heap_ptr, kAlign);
  uintptr_t payload_addr = block_addr + sizeof(Block);
  uintptr_t end = payload_addr + size;
  if (!ensure_capacity(end)) {
    ++g_alloc_fail_count;
    return nullptr;
  }

  Block* b = reinterpret_cast<Block*>(block_addr);
  b->size = size;
  b->next = nullptr;
  g_heap_ptr = end;
  ++g_alloc_count;
  return reinterpret_cast<void*>(payload_addr);
}

extern "C" void free(void* p) {
  if (!p) return;
  uintptr_t payload_addr = reinterpret_cast<uintptr_t>(p);
  Block* b = reinterpret_cast<Block*>(payload_addr - sizeof(Block));
  b->next = g_free_list;
  g_free_list = b;
  ++g_free_count;
}

#if defined(DEBUG) && DEBUG
// ---- Heap stats (for monitoring) ----
extern "C" unsigned int wasm_heap_alloc_count() { return g_alloc_count; }
extern "C" unsigned int wasm_heap_free_count() { return g_free_count; }
extern "C" unsigned int wasm_heap_alloc_fail_count() { return g_alloc_fail_count; }
extern "C" unsigned int wasm_heap_used_bytes() {
  ensure_heap_inited();
  if (g_heap_ptr <= g_heap_base) return 0;
  return (unsigned int)(g_heap_ptr - g_heap_base);
}
extern "C" unsigned int wasm_heap_free_list_bytes() {
  unsigned int sum = 0;
  for (Block* b = g_free_list; b; b = b->next) sum += (unsigned int)b->size;
  return sum;
}
extern "C" unsigned int wasm_heap_capacity_bytes() {
  // total linear memory in bytes
  unsigned long pages_now = __builtin_wasm_memory_size(0);
  uintptr_t bytes_now = static_cast<uintptr_t>(pages_now) * kPageSize;
  ensure_heap_inited();
  if (bytes_now <= g_heap_base) return 0;
  return (unsigned int)(bytes_now - g_heap_base);
}

// Addresses (wasm32 offsets) for debugging heap/global overlap.
extern "C" unsigned int wasm_heap_base_addr() {
  ensure_heap_inited();
  return (unsigned int)g_heap_base;
}
extern "C" unsigned int wasm_heap_ptr_addr() {
  ensure_heap_inited();
  return (unsigned int)g_heap_ptr;
}
#endif

// ---- C++ new operators (keep wasm small; no exceptions) ----
// Some code may use `new` even under -fno-exceptions. Provide minimal operators.
void* operator new(size_t n) {
  if (void* p = malloc(n)) return p;
  __builtin_trap();
}
void* operator new[](size_t n) {
  if (void* p = malloc(n)) return p;
  __builtin_trap();
}

// ---- C++ delete operators (avoid importing _ZdlPv / _ZdlPvm) ----
// When using virtual functions, the compiler may emit a deleting destructor which
// references these operators, even if you never call delete yourself.
void operator delete(void* p) noexcept { free(p); }
void operator delete(void* p, size_t) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }
void operator delete[](void* p, size_t) noexcept { free(p); }

// ---- C++ runtime stubs (avoid importing __cxa_atexit under -nostdlib) ----
extern "C" {
void* __dso_handle = (void*)0;
int __cxa_atexit(void (*)(void*), void*, void*) { return 0; }
void __cxa_finalize(void*) {}
void __cxa_pure_virtual() {}

// Standard C library memory functions
void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    unsigned char v = (unsigned char)value;
    for (size_t i = 0; i < num; ++i) {
        p[i] = v;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < num; ++i) {
        d[i] = s[i];
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < num; ++i) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = num; i > 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    for (size_t i = 0; i < num; ++i) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

}

namespace shine::wasm {

// Implementation of shared logic for SVector reserve
void svector_reserve_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int length, unsigned int newCap, unsigned int elemSize) {
    if (newCap <= *cap_ref) return;
    if (elemSize == 0) return;

    unsigned int bytes = newCap * elemSize;
    void* np = raw_malloc((size_t)(bytes));
    if (!np) return;

    void* old_ptr = *pointer_ref;
    if (old_ptr && length != 0) {
        raw_memcpy(np, old_ptr, (size_t)length * (size_t)elemSize);
    }
    if (old_ptr) raw_free(old_ptr);
    *pointer_ref = np;
    *cap_ref = newCap;
}

} // namespace shine::wasm
