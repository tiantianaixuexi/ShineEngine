#ifdef SHINE_USE_MODULE

export module shine.memory;

import <cstddef>;
import <cstdint>;
import <atomic>;
import <algorithm>;
import <cstdio>;
import <cstring>;
import <new>;
import <source_location>;
import <type_traits>;
import <utility>;

#else

#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <source_location>
#include <type_traits>
#include <utility>

#endif



namespace shine::co {

    // ============================================================
    // Configuration
    // ============================================================

    /// Debug memory validation (header corruption, double-free detection).
    /// Enable in debug builds for safety; disable in release for speed.
#if !defined(SHINE_MEMORY_DEBUG)
  #if defined(_DEBUG) || defined(DEBUG)
    #define SHINE_MEMORY_DEBUG 1
  #else
    #define SHINE_MEMORY_DEBUG 0
  #endif
#endif

#if !defined(SHINE_MEMORY_BACKEND_UE_B2)
  #define SHINE_MEMORY_BACKEND_UE_B2 0
#endif

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
        Gameplay,
        Reflection,
        Script,
        Count
    };

    SHINE_MODULE_EXPORT extern const char* g_memoryTagNames[];

    // ============================================================
    // MemoryTagStatsAtomic
    // ============================================================

    struct MemoryTagStatsAtomic {
        // ---- Long-term stats ----
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
    // Allocation Header  (16 bytes on 64-bit)
    // ============================================================
    //
    // Layout in memory:
    //   [raw_ptr ... padding ... | AllocationHeader | user_ptr →]
    //
    // 'offset' stores the distance from raw_ptr to user_ptr so
    // that Free can recover the original mimalloc pointer.
    //
    // 'size' is now size_t (was uint32_t) to support allocations
    // beyond 4 GB without silent truncation.
    //
    // 'magic' is a sentinel value checked in debug builds to
    // detect header corruption, double frees, and wild pointers.
    //

    inline constexpr uint32_t kHeaderMagicAlive = 0x5348'4E45u;  // "SHNE"
    inline constexpr uint32_t kHeaderMagicDead  = 0xDEAD'BEEFu;

    struct AllocationHeader {
        size_t   size;      // 8  bytes — allocation size (user payload)
        uint32_t magic;     // 4  bytes — debug sentinel
        uint16_t tag;       // 2  bytes — MemoryTag
        uint16_t offset;    // 2  bytes — distance from raw → user ptr
    };

    static_assert(sizeof(AllocationHeader) == 16,
        "AllocationHeader must be exactly 16 bytes for cache-line efficiency");
    static_assert(alignof(AllocationHeader) <= alignof(std::max_align_t),
        "AllocationHeader alignment must not exceed max_align_t");

    // ============================================================
    // Declarations
    // ============================================================

    SHINE_MODULE_EXPORT extern MemoryTagStatsAtomic g_tagStats[(size_t)MemoryTag::Count];
    SHINE_MODULE_EXPORT extern thread_local MemoryTag g_tlsMemoryTag;

    // ============================================================
    // MemoryScope (RAII Tag)
    // ============================================================

    SHINE_MODULE_EXPORT class MemoryScope {
    public:
        explicit MemoryScope(MemoryTag tag) noexcept
            : prev_(g_tlsMemoryTag) {
            g_tlsMemoryTag = tag;
        }

        ~MemoryScope() noexcept {
            g_tlsMemoryTag = prev_;
        }

        // Non-copyable, non-movable
        MemoryScope(const MemoryScope&) = delete;
        MemoryScope& operator=(const MemoryScope&) = delete;
        MemoryScope(MemoryScope&&) = delete;
        MemoryScope& operator=(MemoryScope&&) = delete;

        [[nodiscard]] MemoryTag PreviousTag() const noexcept { return prev_; }

    private:
        MemoryTag prev_;
    };

    // ============================================================
    // Memory  — static interface to the allocator
    // ============================================================

    SHINE_MODULE_EXPORT class Memory {
    public:
        // --------------------------------------------------------
        // Alloc
        // --------------------------------------------------------
        [[nodiscard]]
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
        // Realloc  (tries in-place via mi_realloc_aligned)
        // --------------------------------------------------------
        [[nodiscard]]
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
        static void TrimAllocator() noexcept;
        static bool ValidateHeap() noexcept;
        static void DumpAllocatorStats() noexcept;

        /// Flush all thread-local stat buffers to global atomics.
        /// Call before reading stats for accurate numbers.
        static void FlushAllThreadStats() noexcept;

        // --------------------------------------------------------
        // Debug helpers
        // --------------------------------------------------------

        /// Validate that a pointer was allocated by this system.
        /// Returns false and optionally logs if the header is corrupt.
        static bool ValidatePointer(const void* p) noexcept;

    private:
        // Internal: recover the header from a user pointer.
        static AllocationHeader* GetHeader(void* p) noexcept {
            return reinterpret_cast<AllocationHeader*>(
                static_cast<char*>(p) - sizeof(AllocationHeader));
        }
        static const AllocationHeader* GetHeader(const void* p) noexcept {
            return reinterpret_cast<const AllocationHeader*>(
                static_cast<const char*>(p) - sizeof(AllocationHeader));
        }
    };

    // ============================================================
    // TaggedAllocator — STL-compatible allocator routed through
    //                   the engine Memory system with a fixed tag
    // ============================================================

    SHINE_MODULE_EXPORT
    template <typename T, MemoryTag Tag = MemoryTag::Unknown>
    class TaggedAllocator {
    public:
        using value_type = T;
        using size_type  = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;

        constexpr TaggedAllocator() noexcept = default;

        template <typename U, MemoryTag OtherTag>
        constexpr TaggedAllocator(const TaggedAllocator<U, OtherTag>&) noexcept {}

        [[nodiscard]] T* allocate(size_type n) {
            MemoryScope scope(Tag);
            void* p = Memory::Alloc(n * sizeof(T), alignof(T));
            if (!p) throw std::bad_alloc();
            return static_cast<T*>(p);
        }

        void deallocate(T* p, [[maybe_unused]] size_type n) noexcept {
            Memory::Free(p);
        }

        template <typename U, MemoryTag UTag>
        bool operator==(const TaggedAllocator<U, UTag>&) const noexcept {
            return Tag == UTag;
        }
    };

} // namespace shine::co
