#ifdef SHINE_USE_MODULE

export module shine.memory;

import <cstddef>;
import <cstdint>;
import <atomic>;
import <algorithm>;
import <cstdio>;
import <cstring>;
import <source_location>;

#else

#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <source_location>

#endif

// mimalloc
#include <mimalloc.h>

namespace shine::co {

    // ============================================================
    // Frame Context
    // ============================================================

    struct FrameContext {
        uint64_t frame_index = 0;
    };

    SHINE_MODULE_EXPORT extern FrameContext g_frameContext;

    // ============================================================
    // MemoryTag
    // ============================================================

    SHINE_MODULE_EXPORT enum class MemoryTag : uint16_t {
        Unknown = 0,
        Core,
        Job,
        Render,
        Resource,
        Physics,
        AI,
        Count
    };

    SHINE_MODULE_EXPORT extern const char* g_memoryTagNames[];

    // ============================================================
    // MemoryTagStatsAtomic
    // ============================================================

    struct MemoryTagStatsAtomic {
        // ---- Long term stats ----
        std::atomic<size_t>   bytes_current{0};
        std::atomic<size_t>   bytes_peak{0};
        std::atomic<uint64_t> alloc_count{0};
        std::atomic<uint64_t> free_count{0};

        // ---- Frame stats ----
        std::atomic<uint64_t> last_alloc_frame{0};
        std::atomic<uint64_t> last_free_frame{0};
        std::atomic<uint32_t> allocs_this_frame{0};
        std::atomic<uint32_t> frees_this_frame{0};
    };

    struct MemoryTagStats {
        size_t   bytes_current;
        size_t   bytes_peak;
        uint64_t alloc_count;
        uint64_t free_count;
    };

    // ============================================================
    // Allocation Header
    // ============================================================

    struct AllocationHeader {
        uint32_t size;
        uint16_t tag;
        uint16_t offset; // Offset from raw pointer to user pointer
    };

    // Declarations
    SHINE_MODULE_EXPORT extern MemoryTagStatsAtomic g_tagStats[(size_t)MemoryTag::Count];
    SHINE_MODULE_EXPORT extern thread_local MemoryTag g_tlsMemoryTag;

    // ============================================================
    // MemoryScope (RAII Tag)
    // ============================================================

    SHINE_MODULE_EXPORT class MemoryScope {
    public:
        explicit MemoryScope(MemoryTag tag) noexcept
            : _prev(g_tlsMemoryTag) {
            g_tlsMemoryTag = tag;
        }
        ~MemoryScope() noexcept {
            g_tlsMemoryTag = _prev;
        }
    private:
        MemoryTag _prev;
    };

    // ============================================================
    // Memory
    // ============================================================

    SHINE_MODULE_EXPORT class Memory {
    public:
        // --------------------------------------------------------
        // Alloc
        // --------------------------------------------------------
        static void* Alloc(
            size_t size,
            size_t align = alignof(std::max_align_t),
            const std::source_location& loc =
                std::source_location::current()) noexcept;

        // --------------------------------------------------------
        // Free
        // --------------------------------------------------------
        static void Free(void* p) noexcept;

        // --------------------------------------------------------
        // Realloc
        // --------------------------------------------------------
        static void* Realloc(
            void* p,
            size_t newSize,
            size_t align = alignof(std::max_align_t),
            const std::source_location& loc =
                std::source_location::current()) noexcept;

        // --------------------------------------------------------
        // Stats
        // --------------------------------------------------------
        static MemoryTagStats GetTagStats(MemoryTag tag) noexcept;
        static void PrintTagStats(MemoryTag tag) noexcept;
        static void DumpAllTags() noexcept;
        static void DumpFrameSpikes(uint32_t allocThreshold = 64) noexcept;
    };

} // namespace shine::co
