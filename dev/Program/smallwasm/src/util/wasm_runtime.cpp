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

struct BlockHeader {
  size_t size_flags; // includes header; LSB = free flag
  BlockHeader* next;
};

static constexpr size_t kFreeFlag = 1;
static constexpr size_t kHeaderSize = (sizeof(BlockHeader) + kAlign - 1) & ~(kAlign - 1);
static constexpr size_t kMinBlockSize = kHeaderSize + kAlign;
static constexpr unsigned int kBinCount = 8;
static constexpr size_t kBinLimits[kBinCount - 1] = {
    64, 128, 256, 512, 1024, 2048, 4096,
};

static uintptr_t g_heap_ptr = 0;
static BlockHeader* g_free_bins[kBinCount] = {};
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

static inline size_t block_size(const BlockHeader* b) { return b->size_flags & ~kFreeFlag; }
static inline bool block_is_free(const BlockHeader* b) { return (b->size_flags & kFreeFlag) != 0; }
static inline unsigned int size_to_bin(size_t size) {
  for (unsigned int i = 0; i < kBinCount - 1; ++i) {
    if (size <= kBinLimits[i]) return i;
  }
  return kBinCount - 1;
}

static inline void remove_free(BlockHeader* b) {
  unsigned int bin = size_to_bin(block_size(b));
  BlockHeader* prev = nullptr;
  for (BlockHeader* cur = g_free_bins[bin]; cur; cur = cur->next) {
    if (cur == b) {
      if (prev) prev->next = cur->next;
      else g_free_bins[bin] = cur->next;
      break;
    }
    prev = cur;
  }
  b->next = nullptr;
}

static inline void insert_free(BlockHeader* b) {
  unsigned int bin = size_to_bin(block_size(b));
  b->next = g_free_bins[bin];
  g_free_bins[bin] = b;
}

static BlockHeader* find_suitable_block(size_t need) {
  unsigned int bin = size_to_bin(need);
  for (unsigned int i = bin; i < kBinCount; ++i) {
    for (BlockHeader* b = g_free_bins[i]; b; b = b->next) {
      if (block_size(b) >= need) {
        remove_free(b);
        return b;
      }
    }
  }
  return nullptr;
}

static BlockHeader* coalesce(BlockHeader* b) {
  uintptr_t b_addr = reinterpret_cast<uintptr_t>(b);
  size_t total = block_size(b);

  uintptr_t next_addr = b_addr + total;
  if (next_addr < g_heap_ptr) {
    BlockHeader* nb = reinterpret_cast<BlockHeader*>(next_addr);
    if (block_is_free(nb)) {
      remove_free(nb);
      total += block_size(nb);
      b->size_flags = total | kFreeFlag;
    }
  }
  return b;
}

extern "C" void* malloc(size_t size) {
  if (expect(size == 0, 0)) return nullptr;
  ensure_heap_inited();
  size = static_cast<size_t>(align_up(static_cast<uintptr_t>(size), kAlign));

  size_t need = size + kHeaderSize;
  need = static_cast<size_t>(align_up(static_cast<uintptr_t>(need), kAlign));
  assume((need & (kAlign - 1)) == 0);

  BlockHeader* b = find_suitable_block(need);
  if (b) {
    size_t b_size = block_size(b);
    size_t remain = b_size - need;
    if (remain >= kMinBlockSize) {
      BlockHeader* nb = reinterpret_cast<BlockHeader*>(reinterpret_cast<uintptr_t>(b) + need);
      nb->size_flags = remain | kFreeFlag;
      insert_free(nb);
      b->size_flags = need;
    } else {
      b->size_flags = b_size;
    }
    ++g_alloc_count;
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(b) + kHeaderSize);
  }

  uintptr_t block_addr = align_up(g_heap_ptr, kAlign);
  uintptr_t end = block_addr + need;
  if (expect(!ensure_capacity(end), 0)) {
    ++g_alloc_fail_count;
    return nullptr;
  }

  b = reinterpret_cast<BlockHeader*>(block_addr);
  b->size_flags = need;
  b->next = nullptr;
  g_heap_ptr = end;
  ++g_alloc_count;
  return reinterpret_cast<void*>(block_addr + kHeaderSize);
}

extern "C" void free(void* p) {
  if (expect(!p, 0)) return;
  uintptr_t payload_addr = reinterpret_cast<uintptr_t>(p);
  BlockHeader* b = reinterpret_cast<BlockHeader*>(payload_addr - kHeaderSize);
  b->size_flags = block_size(b) | kFreeFlag;
  b = coalesce(b);
  insert_free(b);
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
  for (unsigned int i = 0; i < kBinCount; ++i) {
    for (BlockHeader* b = g_free_bins[i]; b; b = b->next) {
      size_t payload = block_size(b);
      if (payload > kHeaderSize) payload -= kHeaderSize;
      sum += (unsigned int)payload;
    }
  }
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

}

namespace shine::wasm {

void* raw_realloc(void* p, size_t oldSize, size_t newSize) noexcept {
    if (!p) return raw_malloc(newSize);
    if (newSize == 0) {
        raw_free(p);
        return nullptr;
    }
    void* np = raw_malloc(newSize);
    if (!np) return nullptr;
    const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
    if (copySize > 0) raw_memcpy(np, p, copySize);
    raw_free(p);
    return np;
}

// Implementation of shared logic for SVector reserve
void svector_reserve_impl(void** pointer_ref, unsigned int* cap_ref, unsigned int length, unsigned int newCap, unsigned int elemSize) {
    if (newCap <= *cap_ref) return;
    if (elemSize == 0) return;

    unsigned int bytes = newCap * elemSize;
    void* old_ptr = *pointer_ref;
    const size_t old_bytes = (size_t)length * (size_t)elemSize;
    void* np = raw_realloc(old_ptr, old_bytes, (size_t)bytes);
    if (!np) return;
    *pointer_ref = np;
    *cap_ref = newCap;
}

} // namespace shine::wasm
