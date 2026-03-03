#include "memory_backend.h"
#include "ue_binned2_port.h"

#ifndef SHINE_MEMORY_BACKEND_UE_B2
#define SHINE_MEMORY_BACKEND_UE_B2 0
#endif

#if !defined(SHINE_USE_MODULE) && !SHINE_MEMORY_BACKEND_UE_B2
#define MI_MALLOC_IMPLEMENTATION
#include "../third/mimalloc/mimalloc.h"
#endif

namespace shine::co::memory_backend {
#if SHINE_MEMORY_BACKEND_UE_B2
    void* Alloc(size_t size, size_t align) noexcept {
        return ue_binned2_port::Alloc(size, align);
    }

    void Free(void* p) noexcept {
        ue_binned2_port::Free(p);
    }

    void* Realloc(void* p, size_t size, size_t align) noexcept {
        return ue_binned2_port::Realloc(p, size, align);
    }

    bool GetAllocationSize(void* p, size_t& outSize) noexcept {
        return ue_binned2_port::GetAllocationSize(p, outSize);
    }

    void Trim() noexcept {
        ue_binned2_port::Trim();
    }

    bool ValidateHeap() noexcept {
        return ue_binned2_port::ValidateHeap();
    }

    void DumpAllocatorStats() noexcept {
        ue_binned2_port::DumpAllocatorStats();
    }
#else
    void* Alloc(size_t size, size_t align) noexcept {
        return mi_malloc_aligned(size, align);
    }

    void Free(void* p) noexcept {
        mi_free(p);
    }

    void* Realloc(void* p, size_t size, size_t align) noexcept {
        return mi_realloc_aligned(p, size, align);
    }

    bool GetAllocationSize(void* p, size_t& outSize) noexcept {
        if (!p) {
            outSize = 0;
            return false;
        }
        outSize = mi_usable_size(p);
        return true;
    }

    void Trim() noexcept {}

    bool ValidateHeap() noexcept {
        return true;
    }

    void DumpAllocatorStats() noexcept {}
#endif
}
