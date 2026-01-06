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
        "AI"
    };

    // ============================================================
    // Thread Local Statistics Buffering
    // ============================================================

    struct ThreadLocalTagStats {
        int64_t pending_alloc_bytes = 0;
        int64_t pending_alloc_count = 0;
        int64_t pending_free_bytes = 0;
        int64_t pending_free_count = 0;
        
        // Thresholds for flushing to global
        static constexpr int64_t BYTES_THRESHOLD = 16 * 1024; // 16KB
        static constexpr int64_t COUNT_THRESHOLD = 100;       // 100 ops
    };

    thread_local ThreadLocalTagStats g_tlsTagStats[(size_t)MemoryTag::Count];

    static void FlushStats(MemoryTag tag, ThreadLocalTagStats& stats) {
        auto& global = g_tagStats[(size_t)tag];
        
        if (stats.pending_alloc_bytes > 0) {
            global.bytes_current.fetch_add(stats.pending_alloc_bytes, std::memory_order_relaxed);
            // Update peak (approximate for performance)
            size_t current = global.bytes_current.load(std::memory_order_relaxed);
            size_t peak = global.bytes_peak.load(std::memory_order_relaxed);
            if (current > peak) {
                // Relaxed CAS, if it fails, someone else updated it, which is fine
                global.bytes_peak.compare_exchange_weak(peak, current, std::memory_order_relaxed);
            }
        }
        
        if (stats.pending_alloc_count > 0) {
            global.alloc_count.fetch_add(stats.pending_alloc_count, std::memory_order_relaxed);
            global.allocs_this_frame.fetch_add((uint32_t)stats.pending_alloc_count, std::memory_order_relaxed);
            global.last_alloc_frame.store(g_frameContext.frame_index, std::memory_order_relaxed);
        }

        if (stats.pending_free_bytes > 0) {
            global.bytes_current.fetch_sub(stats.pending_free_bytes, std::memory_order_relaxed);
        }

        if (stats.pending_free_count > 0) {
            global.free_count.fetch_add(stats.pending_free_count, std::memory_order_relaxed);
            global.frees_this_frame.fetch_add((uint32_t)stats.pending_free_count, std::memory_order_relaxed);
            global.last_free_frame.store(g_frameContext.frame_index, std::memory_order_relaxed);
        }

        // Reset
        stats.pending_alloc_bytes = 0;
        stats.pending_alloc_count = 0;
        stats.pending_free_bytes = 0;
        stats.pending_free_count = 0;
    }

    static void UpdateAllocStats(MemoryTag tag, size_t size) {
        auto& stats = g_tlsTagStats[(size_t)tag];
        stats.pending_alloc_bytes += size;
        stats.pending_alloc_count++;

        if (stats.pending_alloc_bytes >= ThreadLocalTagStats::BYTES_THRESHOLD || 
            stats.pending_alloc_count >= ThreadLocalTagStats::COUNT_THRESHOLD) {
            FlushStats(tag, stats);
        }
    }

    static void UpdateFreeStats(MemoryTag tag, size_t size) {
        auto& stats = g_tlsTagStats[(size_t)tag];
        stats.pending_free_bytes += size;
        stats.pending_free_count++;

        if (stats.pending_free_bytes >= ThreadLocalTagStats::BYTES_THRESHOLD || 
            stats.pending_free_count >= ThreadLocalTagStats::COUNT_THRESHOLD) {
            FlushStats(tag, stats);
        }
    }

    // ============================================================
    // Memory Implementation
    // ============================================================

    void* Memory::Alloc(size_t size, size_t align, const std::source_location& loc) noexcept {
        if (size == 0) return nullptr;

        // Ensure alignment for AllocationHeader
        // We put the header immediately before the user pointer.
        // User pointer must be aligned to 'align'.
        // So we need (UserPtr - sizeof(Header)) to be a valid address.
        // We allocate extra space to ensure we can satisfy alignment.
        
        // Calculate the aligned header size. 
        // We need 'alignedHeaderSize' such that 'alignedHeaderSize' is a multiple of 'align'
        // ONLY IF we start at an aligned address.
        // mimalloc aligns the block start to 'align'.
        
        // Strategy:
        // Block Start (Aligned to align) | ... padding ... | Header | UserPtr (Aligned to align)
        // Since Block Start is aligned, UserPtr is aligned if (padding + sizeof(Header)) is multiple of align.
        // Also UserPtr = BlockStart + padding + sizeof(Header).
        // To minimize waste, we want smallest padding.
        // But we can just force the offset to be a multiple of align.
        
        size_t headerSize = sizeof(AllocationHeader);
        size_t offset = (headerSize + align - 1) & ~(align - 1); // Align up header size
        
        // Ensure the allocation is also aligned to the header requirements
        size_t allocAlign = std::max(align, alignof(AllocationHeader));

        void* p = mi_malloc_aligned(size + offset, allocAlign);
        if (!p) return nullptr;

        // p is aligned to align.
        // userPtr = p + offset.
        // Since offset is multiple of align, userPtr is aligned to align.
        
        void* userPtr = static_cast<char*>(p) + offset;
        
        auto* header = reinterpret_cast<AllocationHeader*>(static_cast<char*>(userPtr) - sizeof(AllocationHeader));
        header->tag = (uint16_t)g_tlsMemoryTag;
        
        // Note: We don't store size anymore if we trust mi_usable_size, 
        // but mi_usable_size returns the full block size (including offset?).
        // mi_usable_size(p) returns the usable size of the block pointed to by p.
        // The block allocated was (size + offset).
        // So mi_usable_size(p) >= size + offset.
        // When freeing, we get p by (userPtr - offset).
        // Wait, to get p, we need to know offset.
        // Offset depends on align. We don't know align at Free time!
        // PROBLEM: Free(void* p) doesn't take alignment.
        // So we CANNOT calculate offset dynamically during Free without storing it.
        // We MUST store the offset or the original pointer.
        // Or ensure the offset is constant/calculable.
        // Since 'align' varies per Alloc, offset varies.
        // So we MUST store the size or the offset.
        
        // Let's store the size. It's safer and simpler.
        // Current Header has size (uint32_t).
        header->size = (uint32_t)size;
        header->offset = (uint16_t)offset;
        // header->pad is not needed to be set.

        UpdateAllocStats((MemoryTag)header->tag, size);
        
        (void)loc; 
        return userPtr;
    }

    void Memory::Free(void* p) noexcept {
        if (!p) return;

        // We need to recover the header.
        // But wait, where is the header?
        // In Alloc, we did: userPtr = p_raw + offset.
        // Header is at userPtr - sizeof(Header).
        // This is ALWAYS true regardless of alignment padding.
        // The padding is BEFORE the header.
        
        auto* header = reinterpret_cast<AllocationHeader*>(static_cast<char*>(p) - sizeof(AllocationHeader));
        
        size_t size = header->size;
        MemoryTag tag = (MemoryTag)header->tag;
        
        UpdateFreeStats(tag, size);

        // Now we need to free the RAW pointer.
        // raw = userPtr - offset.
        // But we don't know offset because we don't know align!
        // mi_free expects the pointer returned by mi_malloc.
        // If we pass an interior pointer, mimalloc MIGHT handle it if compiled with checks, but standard mi_free requires the start.
        
        // Solution:
        // 1. Store the offset in the header.
        // 2. Or Store the raw pointer? (8 bytes)
        // 3. Or rely on mi_free accepting interior pointers? (Not safe).
        
        // We should add 'offset' or 'alignment' to the header.
        // AllocationHeader:
        // uint32_t size;
        // uint16_t tag;
        // uint16_t _pad;
        
        // We can reuse _pad to store 'offset' / alignment factor?
        // offset = (headerSize + align - 1) & ~(align - 1);
        // If align is large, offset is large.
        // However, standard alignment is usually small.
        // If align <= 256, we can store it in uint8_t.
        // If align is huge, we might have issues.
        
        // Better: AllocationHeader changes.
        // struct AllocationHeader {
        //    uint32_t size;
        //    uint16_t tag;
        //    uint16_t offset; // Offset from raw start to userPtr? 
        // };
        
        // If we change Header definition, we need to update it in memory.ixx too.
        // Let's update the Header to include offset.
        
        // Re-reading the header struct in my mind:
        // struct AllocationHeader { uint32_t size; uint16_t tag; uint16_t _pad; };
        // We can use _pad for offset.
        // offset is at least sizeof(Header) = 8.
        // If align=8, offset=8.
        // If align=16, offset=16.
        // If align=64, offset=64.
        // Max offset fits in uint16_t (65535). 
        // So we support alignment up to ~64KB. This is plenty for a game engine (usually 16, 256, 4096).
        // 4096 fits in uint16.
        
        uint16_t storedOffset = header->offset;
        void* raw = static_cast<char*>(p) - storedOffset;
        mi_free(raw);
    }

    void* Memory::Realloc(void* p, size_t newSize, size_t align, const std::source_location& loc) noexcept {
        if (!p) return Alloc(newSize, align, loc);
        if (newSize == 0) {
            Free(p);
            return nullptr;
        }

        auto* header = reinterpret_cast<AllocationHeader*>(static_cast<char*>(p) - sizeof(AllocationHeader));
        size_t oldSize = header->size;
        
        // Optimization: if newSize <= oldSize, just return p?
        // Or shrink?
        // Usually Realloc allows growing in place.
        // But we have a custom header layout and alignment padding.
        // mi_realloc might move the block, changing alignment properties?
        // mi_realloc_aligned exists?
        // mi_realloc_aligned(p, newsize, align).
        
        // Since we wrap the pointer, we can't pass 'p' to mi_realloc directly.
        // We need to pass 'raw'.
        // And if we realloc 'raw', the offset might change if we want to maintain alignment?
        // Actually, if we use mi_realloc, it might move data.
        // The simplest robust implementation for Realloc with custom headers/alignment is Alloc + Copy + Free.
        // Unless we are sure about in-place growth.
        
        // Let's do Alloc + Copy + Free for safety and simplicity, as Realloc is rarer.
        // And it guarantees alignment.
        
        void* newPtr = Alloc(newSize, align, loc);
        if (!newPtr) return nullptr;
        
        std::memcpy(newPtr, p, std::min(oldSize, newSize));
        Free(p);
        
        return newPtr;
    }

    MemoryTagStats Memory::GetTagStats(MemoryTag tag) noexcept {
        auto& s = g_tagStats[(size_t)tag];
        return {
            s.bytes_current.load(std::memory_order_relaxed),
            s.bytes_peak.load(std::memory_order_relaxed),
            s.alloc_count.load(std::memory_order_relaxed),
            s.free_count.load(std::memory_order_relaxed)
        };
    }

    void Memory::PrintTagStats(MemoryTag tag) noexcept {
        auto s = GetTagStats(tag);
        std::printf(
            "[Memory][%s] allocs=%llu frees=%llu current=%.2fMB peak=%.2fMB\n",
            g_memoryTagNames[(size_t)tag],
            (unsigned long long)s.alloc_count,
            (unsigned long long)s.free_count,
            double(s.bytes_current) / (1024.0 * 1024.0),
            double(s.bytes_peak) / (1024.0 * 1024.0)
        );
    }

    void Memory::DumpAllTags() noexcept {
        std::printf("[Memory] ===== Dump All Tags =====\n");

        for (size_t i = 0; i < (size_t)MemoryTag::Count; ++i) {
            auto& s = g_tagStats[i];

            size_t current = s.bytes_current.load(std::memory_order_relaxed);
            size_t peak    = s.bytes_peak.load(std::memory_order_relaxed);
            uint64_t allocs = s.alloc_count.load(std::memory_order_relaxed);
            uint64_t frees  = s.free_count.load(std::memory_order_relaxed);

            if (allocs == 0 && frees == 0 && current == 0)
                continue;

            std::printf(
                "[Memory][%-9s] allocs=%llu frees=%llu current=%.2fMB peak=%.2fMB\n",
                g_memoryTagNames[i],
                (unsigned long long)allocs,
                (unsigned long long)frees,
                double(current) / (1024.0 * 1024.0),
                double(peak) / (1024.0 * 1024.0)
            );
        }

        std::printf("[Memory] =========================\n");
    }

    void Memory::DumpFrameSpikes(uint32_t allocThreshold) noexcept {
        for (size_t i = 0; i < (size_t)MemoryTag::Count; ++i) {
            auto& s = g_tagStats[i];

            uint32_t allocs =
                s.allocs_this_frame.exchange(0, std::memory_order_relaxed);
            s.frees_this_frame.exchange(0, std::memory_order_relaxed);

            if (allocs < allocThreshold)
                continue;

            std::printf(
                "[Memory][Frame %llu][%s] allocs=%u current=%.2fMB\n",
                (unsigned long long)g_frameContext.frame_index,
                g_memoryTagNames[i],
                allocs,
                double(s.bytes_current.load()) / (1024.0 * 1024.0)
            );
        }
    }

} // namespace shine::co
