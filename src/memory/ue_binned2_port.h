#pragma once

#include <cstddef>

namespace shine::co::ue_binned2_port {
    [[nodiscard]] size_t GetTaggedPointerMinAlign() noexcept;
    [[nodiscard]] size_t GetTaggedPointerUsableLowBits() noexcept;
    [[nodiscard]] bool SupportsTaggedPointerTagBits(size_t tagBits) noexcept;
    void* Alloc(size_t size, size_t align) noexcept;
    void Free(void* p) noexcept;
    void* Realloc(void* p, size_t size, size_t align) noexcept;
    bool GetAllocationSize(void* p, size_t& outSize) noexcept;
    void Trim() noexcept;
    bool ValidateHeap() noexcept;
    void DumpAllocatorStats() noexcept;
}
