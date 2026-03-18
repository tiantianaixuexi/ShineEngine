#include "memory_backend.h"
#include "ue_binned2_port.h"

#ifndef SHINE_MEMORY_BACKEND_UE_B2
#define SHINE_MEMORY_BACKEND_UE_B2 0
#endif


namespace shine::co::memory_backend {

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
}
