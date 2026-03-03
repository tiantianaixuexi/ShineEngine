#ifdef SHINE_USE_MODULE
module shine.memory;
import <algorithm>;
import <cstdio>;
import <cstring>;
import <new>;
#include <atomic>
import <source_location>;
#endif



#ifndef SHINE_USE_MODULE
#include "memory.ixx"
#include "fmt/printf.h"
#endif
#include "memory_backend.h"
#include "util/profiling/shine_profiling.h"

#ifndef SHINE_MEMORY_BACKEND_UE_B2
#define SHINE_MEMORY_BACKEND_UE_B2 0
#endif
namespace shine::co {

    // ============================================================
    // Global Definitions
    // ============================================================

    FrameContext g_frameContext;
    
    // Using an array of atomics for global stats
    MemoryTagStatsAtomic g_tagStats[(size_t)MemoryTag::Count];

    // Thread-local tag definition
    thread_local MemoryTag g_tlsMemoryTag = MemoryTag::Unknown;

    const char* g_memoryTagNames[] = {
        "Unknown",
        "Core",
        "Job",
        "Render",
        "Resource",
        "Physics",
        "AI",
        "Gameplay",
        "Reflection",
        "Script"
    };

    // ============================================================
    // Thread-Local Statistics Buffering
    // ============================================================
    //
    // Each thread accumulates allocation stats locally and flushes
    // to the global atomics only when a threshold is reached.
    // This eliminates most atomic contention on hot allocation paths.
    //

    struct ThreadLocalTagStats {
        int64_t pending_alloc_bytes = 0;
        int64_t pending_alloc_count = 0;
        int64_t pending_free_bytes  = 0;
        int64_t pending_free_count  = 0;
        
        // Thresholds for flushing to global
        static constexpr int64_t BYTES_THRESHOLD = 16 * 1024; // 16 KB
        static constexpr int64_t COUNT_THRESHOLD = 100;        // 100 ops
    };

    thread_local ThreadLocalTagStats g_tlsTagStats[(size_t)MemoryTag::Count];

    // ---- Flush thread-local buffer to global atomics ----

    static void FlushStats(MemoryTag tag, ThreadLocalTagStats& stats) {
        auto& global = g_tagStats[(size_t)tag];
        
        if (stats.pending_alloc_bytes != 0) {
            // Use signed add: pending_alloc_bytes is always positive here,
            // but use fetch_add on size_t carefully.
            global.bytes_current.fetch_add(
                static_cast<size_t>(stats.pending_alloc_bytes),
                std::memory_order_relaxed);

            // Update peak with a proper CAS loop (the old single-attempt
            // compare_exchange_weak could miss updates under contention).
            size_t current = global.bytes_current.load(std::memory_order_relaxed);
            size_t peak    = global.bytes_peak.load(std::memory_order_relaxed);
            while (current > peak) {
                if (global.bytes_peak.compare_exchange_weak(
                        peak, current, std::memory_order_relaxed)) {
                    break;
                }
                // CAS failure reloads 'peak'; if someone set a higher peak
                // we'll exit the loop naturally when current <= peak.
            }
        }
        
        if (stats.pending_alloc_count != 0) {
            global.alloc_count.fetch_add(
                static_cast<uint64_t>(stats.pending_alloc_count),
                std::memory_order_relaxed);
            global.allocs_this_frame.fetch_add(
                static_cast<uint32_t>(stats.pending_alloc_count),
                std::memory_order_relaxed);
            global.last_alloc_frame.store(
                g_frameContext.frame_index, std::memory_order_relaxed);
        }

        if (stats.pending_free_bytes != 0) {
            // Guard against underflow: bytes_current is size_t (unsigned).
            // In normal operation pending_free_bytes <= bytes_current,
            // but due to thread-local batching there can be a transient
            // mismatch.  Clamp to zero rather than wrapping around.
            size_t to_sub = static_cast<size_t>(stats.pending_free_bytes);
            size_t prev   = global.bytes_current.fetch_sub(to_sub, std::memory_order_relaxed);
            if (prev < to_sub) {
                // Underflow occurred — snap back to 0.
                global.bytes_current.store(0, std::memory_order_relaxed);
            }
        }

        if (stats.pending_free_count != 0) {
            global.free_count.fetch_add(
                static_cast<uint64_t>(stats.pending_free_count),
                std::memory_order_relaxed);
            global.frees_this_frame.fetch_add(
                static_cast<uint32_t>(stats.pending_free_count),
                std::memory_order_relaxed);
            global.last_free_frame.store(
                g_frameContext.frame_index, std::memory_order_relaxed);
        }

        // Reset thread-local accumulators
        stats = {};
    }

    static void UpdateAllocStats(MemoryTag tag, size_t size) {
        auto& stats = g_tlsTagStats[static_cast<size_t>(tag)];
        stats.pending_alloc_bytes += static_cast<int64_t>(size);
        stats.pending_alloc_count++;

        if (stats.pending_alloc_bytes >= ThreadLocalTagStats::BYTES_THRESHOLD || 
            stats.pending_alloc_count >= ThreadLocalTagStats::COUNT_THRESHOLD) {
            FlushStats(tag, stats);
        }
    }

    static void UpdateFreeStats(MemoryTag tag, size_t size) {
        auto& stats = g_tlsTagStats[static_cast<size_t>(tag)];
        stats.pending_free_bytes += static_cast<int64_t>(size);
        stats.pending_free_count++;

        if (stats.pending_free_bytes >= ThreadLocalTagStats::BYTES_THRESHOLD || 
            stats.pending_free_count >= ThreadLocalTagStats::COUNT_THRESHOLD) {
            FlushStats(tag, stats);
        }
    }

    // ============================================================
    // Debug Helpers
    // ============================================================

#if SHINE_MEMORY_DEBUG
    static void MarkHeaderAlive(AllocationHeader* h) noexcept {
        h->magic = kHeaderMagicAlive;
    }
    static void MarkHeaderDead(AllocationHeader* h) noexcept {
        h->magic = kHeaderMagicDead;
    }
    static bool IsHeaderAlive(const AllocationHeader* h) noexcept {
        return h->magic == kHeaderMagicAlive;
    }
#else
    static void MarkHeaderAlive(AllocationHeader*) noexcept {}
    static void MarkHeaderDead(AllocationHeader*)  noexcept {}
    static bool IsHeaderAlive(const AllocationHeader*) noexcept { return true; }
#endif

    // ============================================================
    // Memory Implementation
    // ============================================================

    void* Memory::Alloc(size_t size, size_t align, const std::source_location& loc) noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::Alloc");
        if (size == 0) [[unlikely]] return nullptr;

        // ----------------------------------------------------------
        // Memory layout:
        //
        //   raw (aligned to allocAlign)
        //    │
        //    ├── ... possible padding ...
        //    ├── AllocationHeader  (16 bytes)
        //    └── userPtr  (aligned to 'align')
        //
        // 'offset' is the distance from raw → userPtr, always a
        // multiple of 'align' so that userPtr stays aligned.
        // ----------------------------------------------------------

        const size_t headerSize = sizeof(AllocationHeader);
        const size_t offset     = (headerSize + align - 1) & ~(align - 1);
        const size_t allocAlign = std::max(align, alignof(AllocationHeader));

        void* raw = memory_backend::Alloc(size + offset, allocAlign);
        if (!raw) [[unlikely]] return nullptr;

        void* userPtr = static_cast<char*>(raw) + offset;
        
        auto* header   = GetHeader(userPtr);
        header->size   = size;
        header->tag    = static_cast<uint16_t>(g_tlsMemoryTag);
        header->offset = static_cast<uint16_t>(offset);
        MarkHeaderAlive(header);

        UpdateAllocStats(static_cast<MemoryTag>(header->tag), size);
        const char* tagName = g_memoryTagNames[static_cast<size_t>(header->tag)];
        SHINE_PROFILE_CPU_VALUE(static_cast<uint64_t>(size));
        SHINE_PROFILE_CPU_TEXT(tagName, std::strlen(tagName));
        SHINE_PROFILE_MEM_ALLOC_N(userPtr, size, tagName);
        
        (void)loc; // Reserved for future source-location tracking
        return userPtr;
    }

    void Memory::Free(void* p) noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::Free");
        if (!p) [[unlikely]] return;

        auto* header = GetHeader(p);

#if SHINE_MEMORY_DEBUG
        if (!IsHeaderAlive(header)) [[unlikely]] {
            // Corrupted or double-freed pointer — do NOT touch mimalloc.
            fmt::println("[Memory] ERROR: Free called on invalid/double-freed pointer %p "
                         "(magic=0x%08X, expected 0x%08X)",
                         p, header->magic, kHeaderMagicAlive);
            return;
        }
#endif

        const size_t  size = header->size;
        const MemoryTag tag = static_cast<MemoryTag>(header->tag);
        const uint16_t storedOffset = header->offset;

        const char* tagName = g_memoryTagNames[static_cast<size_t>(tag)];
        SHINE_PROFILE_CPU_VALUE(static_cast<uint64_t>(size));
        SHINE_PROFILE_CPU_TEXT(tagName, std::strlen(tagName));
        SHINE_PROFILE_MEM_FREE_N(p, tagName);
        MarkHeaderDead(header);
        UpdateFreeStats(tag, size);

        void* raw = static_cast<char*>(p) - storedOffset;
        memory_backend::Free(raw);
    }

    void* Memory::Realloc(void* p, size_t newSize, size_t align, const std::source_location& loc) noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::Realloc");
        if (!p) return Alloc(newSize, align, loc);
        if (newSize == 0) {
            Free(p);
            return nullptr;
        }

        auto* header = GetHeader(p);

#if SHINE_MEMORY_DEBUG
        if (!IsHeaderAlive(header)) [[unlikely]] {
            fmt::println("[Memory] ERROR: Realloc called on invalid pointer %p "
                         "(magic=0x%08X)", p, header->magic);
            return nullptr;
        }
#endif

        const size_t  oldSize      = header->size;
        const MemoryTag tag        = static_cast<MemoryTag>(header->tag);
        const uint16_t storedOffset = header->offset;
        const size_t  allocAlign   = std::max(align, alignof(AllocationHeader));

        // ---- Optimisation: try in-place realloc via mimalloc ----
        //
        // mi_realloc_aligned may grow/shrink in place and returns
        // the same pointer when it succeeds.  When it must relocate,
        // it copies the old data automatically.  Either way the raw
        // block start stays at (userPtr - offset), so our header
        // layout is preserved.

        void* oldRaw = static_cast<char*>(p) - storedOffset;
        void* newRaw = memory_backend::Realloc(oldRaw, newSize + storedOffset, allocAlign);
        if (!newRaw) [[unlikely]] return nullptr;

        void* newUserPtr = static_cast<char*>(newRaw) + storedOffset;
        auto* newHeader  = GetHeader(newUserPtr);

        // Rewrite the header (safe even if pointer didn't move —
        // mi_realloc_aligned guarantees the first min(old,new) bytes
        // are preserved, which includes our header region).
        newHeader->size   = newSize;
        newHeader->tag    = static_cast<uint16_t>(tag);
        newHeader->offset = storedOffset;
        MarkHeaderAlive(newHeader);

        // Update delta stats
        if (newSize > oldSize) {
            UpdateAllocStats(tag, newSize - oldSize);
        } else if (newSize < oldSize) {
            UpdateFreeStats(tag, oldSize - newSize);
        }
        const char* tagName = g_memoryTagNames[static_cast<size_t>(tag)];
        SHINE_PROFILE_CPU_VALUE(static_cast<uint64_t>(newSize));
        SHINE_PROFILE_CPU_TEXT(tagName, std::strlen(tagName));
        SHINE_PROFILE_MEM_FREE_N(p, tagName);
        SHINE_PROFILE_MEM_ALLOC_N(newUserPtr, newSize, tagName);

        return newUserPtr;
    }

    // ============================================================
    // Stats
    // ============================================================

    MemoryTagStats Memory::GetTagStats(MemoryTag tag) noexcept {
        auto& s = g_tagStats[static_cast<size_t>(tag)];
        return {
            s.bytes_current.load(std::memory_order_relaxed),
            s.bytes_peak.load(std::memory_order_relaxed),
            s.alloc_count.load(std::memory_order_relaxed),
            s.free_count.load(std::memory_order_relaxed)
        };
    }

    void Memory::PrintTagStats(MemoryTag tag) noexcept {
        auto s = GetTagStats(tag);
        fmt::println(
            "[Memory][%s] allocs=%llu frees=%llu current=%.2fMB peak=%.2fMB",
            g_memoryTagNames[static_cast<size_t>(tag)],
            static_cast<unsigned long long>(s.alloc_count),
            static_cast<unsigned long long>(s.free_count),
            static_cast<double>(s.bytes_current) / (1024.0 * 1024.0),
            static_cast<double>(s.bytes_peak) / (1024.0 * 1024.0)
        );
    }

    void Memory::DumpAllTags() noexcept {
        fmt::println("[Memory] ===== Dump All Tags =====\n");

        for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
            auto& s = g_tagStats[i];

            size_t current   = s.bytes_current.load(std::memory_order_relaxed);
            size_t peak      = s.bytes_peak.load(std::memory_order_relaxed);
            uint64_t allocs  = s.alloc_count.load(std::memory_order_relaxed);
            uint64_t frees   = s.free_count.load(std::memory_order_relaxed);

            if (allocs == 0 && frees == 0 && current == 0)
                continue;

            fmt::println(
                "[Memory][%-9s] allocs=%llu frees=%llu current=%.2fMB peak=%.2fMB",
                g_memoryTagNames[i],
                static_cast<unsigned long long>(allocs),
                static_cast<unsigned long long>(frees),
                static_cast<double>(current) / (1024.0 * 1024.0),
                static_cast<double>(peak) / (1024.0 * 1024.0)
            );
        }

        fmt::println("[Memory] =========================");
    }

    void Memory::DumpFrameSpikes(uint32_t allocThreshold) noexcept {
        for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
            auto& s = g_tagStats[i];

            uint32_t allocs =
                s.allocs_this_frame.exchange(0, std::memory_order_relaxed);
            s.frees_this_frame.exchange(0, std::memory_order_relaxed);

            if (allocs < allocThreshold)
                continue;

            fmt::println(
                "[Memory][Frame %llu][%s] allocs=%u current=%.2fMB",
                static_cast<unsigned long long>(g_frameContext.frame_index),
                g_memoryTagNames[i],
                allocs,
                static_cast<double>(s.bytes_current.load()) / (1024.0 * 1024.0)
            );
        }
    }

    void Memory::TrimAllocator() noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::TrimAllocator");
        memory_backend::Trim();
        SHINE_PROFILE_CPU_MESSAGE_L("[Memory] TrimAllocator");
    }

    bool Memory::ValidateHeap() noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::ValidateHeap");
        const bool ok = memory_backend::ValidateHeap();
        if (!ok) {
            SHINE_PROFILE_CPU_MESSAGE_L("[Memory] ValidateHeap FAILED");
        }
        return ok;
    }

    void Memory::DumpAllocatorStats() noexcept {
        SHINE_PROFILE_CPU_SCOPE_N("Memory::DumpAllocatorStats");
        memory_backend::DumpAllocatorStats();
        for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
            auto s = GetTagStats(static_cast<MemoryTag>(i));
            if (s.bytes_current == 0 && s.bytes_peak == 0 && s.alloc_count == 0 && s.free_count == 0) {
                continue;
            }
            SHINE_PROFILE_CPU_PLOT(g_memoryTagNames[i], static_cast<double>(s.bytes_current));
        }
    }

    bool Memory::ValidatePointer(const void* p) noexcept {
        if (!p) return false;
#if SHINE_MEMORY_DEBUG
        const auto* header = GetHeader(p);
        return IsHeaderAlive(header);
#else
        return true; // No validation in release
#endif
    }

    void Memory::FlushAllThreadStats() noexcept {
        // Flush the calling thread's stats.
        // NOTE: This cannot flush OTHER threads' buffers because
        // thread_local storage is inaccessible cross-thread.
        // A more complete solution would register thread-locals
        // at construction and iterate them here.
        for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
            auto& stats = g_tlsTagStats[i];
            if (stats.pending_alloc_count != 0 || stats.pending_free_count != 0) {
                FlushStats(static_cast<MemoryTag>(i), stats);
            }
        }
    }

} // namespace shine::co

// ============================================================
// Global New/Delete Overrides
// ============================================================

// We override global new/delete to route all allocations through our Memory system.
// This ensures standard containers (std::vector, std::string) and other 'new' calls
// are tracked by the MemoryProfiler (defaulting to the current thread's tag).

// ---- Throwing overloads ----

void* operator new(size_t size) {
    void* p = shine::co::Memory::Alloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new[](size_t size) {
    void* p = shine::co::Memory::Alloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

// ---- Nothrow overloads (C++11) ----

void* operator new(size_t size, const std::nothrow_t&) noexcept {
    return shine::co::Memory::Alloc(size);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept {
    return shine::co::Memory::Alloc(size);
}

// ---- Delete ----

void operator delete(void* p) noexcept {
    shine::co::Memory::Free(p);
}

void operator delete[](void* p) noexcept {
    shine::co::Memory::Free(p);
}

// Sized delete (C++14) — size parameter is unused because
// our header already stores the allocation size.

void operator delete(void* p, size_t) noexcept {
    shine::co::Memory::Free(p);
}

void operator delete[](void* p, size_t) noexcept {
    shine::co::Memory::Free(p);
}

// ---- Aligned New/Delete (C++17) ----

void* operator new(size_t size, std::align_val_t al) {
    void* p = shine::co::Memory::Alloc(size, static_cast<size_t>(al));
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new[](size_t size, std::align_val_t al) {
    void* p = shine::co::Memory::Alloc(size, static_cast<size_t>(al));
    if (!p) throw std::bad_alloc();
    return p;
}

void* operator new(size_t size, std::align_val_t al, const std::nothrow_t&) noexcept {
    return shine::co::Memory::Alloc(size, static_cast<size_t>(al));
}

void* operator new[](size_t size, std::align_val_t al, const std::nothrow_t&) noexcept {
    return shine::co::Memory::Alloc(size, static_cast<size_t>(al));
}

void operator delete(void* p, std::align_val_t) noexcept {
    shine::co::Memory::Free(p);
}

void operator delete[](void* p, std::align_val_t) noexcept {
    shine::co::Memory::Free(p);
}

void operator delete(void* p, size_t, std::align_val_t) noexcept {
    shine::co::Memory::Free(p);
}

void operator delete[](void* p, size_t, std::align_val_t) noexcept {
    shine::co::Memory::Free(p);
}
