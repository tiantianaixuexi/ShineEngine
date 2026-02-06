#include "ReflectionMemory.h"
#include <algorithm>
#include <cstring>

// =============================================================================
// ReflectionMemoryManager — implementation
// =============================================================================
//
// All allocations now route through shine::co::Memory (mimalloc backend)
// with MemoryScope to tag them as Reflection.  This replaces the previous
// _aligned_malloc / _aligned_free path, giving us:
//   • Unified profiling through the engine's tag system
//   • mimalloc performance (thread-local heaps, size classes)
//   • No per-allocation mutex — stats are atomic
//

namespace shine::reflection {

    // ---- Allocate ----

    void* ReflectionMemoryManager::AllocateImpl(size_t size, MemoryTag tag, size_t alignment) {
        // Tag this allocation as Reflection in the engine's global tracker
        shine::co::MemoryScope engineScope(shine::co::MemoryTag::Reflection);

        // Atomic stat updates — no mutex required
        allocationCount_.fetch_add(1, std::memory_order_relaxed);

        size_t prevTotal = totalAllocated_.fetch_add(size, std::memory_order_relaxed);
        size_t newTotal  = prevTotal + size;

        // Update peak with CAS loop (matches engine pattern)
        size_t peak = peakUsage_.load(std::memory_order_relaxed);
        while (newTotal > peak) {
            if (peakUsage_.compare_exchange_weak(peak, newTotal, std::memory_order_relaxed))
                break;
        }

        // Per-sub-tag byte tracking
        if (HasFlag(tag, MemoryTag::TypeInfo))
            typeInfoBytes_.fetch_add(size, std::memory_order_relaxed);
        if (HasFlag(tag, MemoryTag::FieldInfo))
            fieldInfoBytes_.fetch_add(size, std::memory_order_relaxed);
        if (HasFlag(tag, MemoryTag::StringStorage))
            stringBytes_.fetch_add(size, std::memory_order_relaxed);
        if (HasFlag(tag, MemoryTag::CacheData))
            cacheBytes_.fetch_add(size, std::memory_order_relaxed);

        // Temporary allocations go to the arena (O(1) bump, no syscall)
        if (HasFlag(tag, MemoryTag::TemporaryBuffers)) {
            return arena_.Allocate(size, alignment);
        }

        // Everything else routes through engine Memory (mimalloc)
        return shine::co::Memory::Alloc(size, alignment);
    }

    // ---- Deallocate ----

    void ReflectionMemoryManager::DeallocateImpl(void* ptr, size_t size, MemoryTag tag) {
        if (!ptr) return;

        // Atomic stat updates
        deallocationCount_.fetch_add(1, std::memory_order_relaxed);

        // Clamp-to-zero subtraction on totalAllocated_
        size_t prev = totalAllocated_.load(std::memory_order_relaxed);
        while (true) {
            size_t newVal = (prev > size) ? prev - size : 0;
            if (totalAllocated_.compare_exchange_weak(prev, newVal, std::memory_order_relaxed))
                break;
        }

        // Per-sub-tag byte tracking
        if (HasFlag(tag, MemoryTag::TypeInfo)) {
            size_t p = typeInfoBytes_.load(std::memory_order_relaxed);
            while (p >= size) {
                if (typeInfoBytes_.compare_exchange_weak(p, p - size, std::memory_order_relaxed))
                    break;
            }
        }
        if (HasFlag(tag, MemoryTag::FieldInfo)) {
            size_t p = fieldInfoBytes_.load(std::memory_order_relaxed);
            while (p >= size) {
                if (fieldInfoBytes_.compare_exchange_weak(p, p - size, std::memory_order_relaxed))
                    break;
            }
        }
        if (HasFlag(tag, MemoryTag::StringStorage)) {
            size_t p = stringBytes_.load(std::memory_order_relaxed);
            while (p >= size) {
                if (stringBytes_.compare_exchange_weak(p, p - size, std::memory_order_relaxed))
                    break;
            }
        }
        if (HasFlag(tag, MemoryTag::CacheData)) {
            size_t p = cacheBytes_.load(std::memory_order_relaxed);
            while (p >= size) {
                if (cacheBytes_.compare_exchange_weak(p, p - size, std::memory_order_relaxed))
                    break;
            }
        }

        // Arena allocations are freed in bulk by Reset/Clear — no-op here
        if (HasFlag(tag, MemoryTag::TemporaryBuffers)) {
            return;
        }

        shine::co::Memory::Free(ptr);
    }

    // ---- UpdateStatistics ----

    void ReflectionMemoryManager::UpdateStatistics() {
        size_t allocs = allocationCount_.load(std::memory_order_relaxed);
        size_t deallocs = deallocationCount_.load(std::memory_order_relaxed);
        size_t peak = peakUsage_.load(std::memory_order_relaxed);
        size_t total = totalAllocated_.load(std::memory_order_relaxed);

        double hr = (allocs > 0)
            ? (static_cast<double>(allocs - deallocs) / static_cast<double>(allocs)) * 100.0
            : 0.0;
        hitRate_.store(hr, std::memory_order_relaxed);

        double frag = (peak > 0)
            ? (1.0 - static_cast<double>(total) / static_cast<double>(peak)) * 100.0
            : 0.0;
        fragmentation_.store(frag, std::memory_order_relaxed);
    }

    // =========================================================================
    // StringMemoryManager — implementation
    // =========================================================================

    StringMemoryManager::~StringMemoryManager() {
        ClearStrings();
    }

    const char* StringMemoryManager::StoreString(std::string_view str) {
        // ---- Fast-path: deduplicate via hash set ----
        //
        // If the exact string content was already interned, return
        // the existing pointer.  This avoids allocating duplicate
        // copies of common names like "float", "int", field names, etc.

        if (auto it = internSet_.find(str); it != internSet_.end()) {
            return it->data();
        }

        // ---- Slow-path: store in arena + insert into set ----
        const char* stored = arena_.Store(str);
        if (stored) {
            // Create a string_view pointing into the arena memory
            internSet_.emplace(stored, str.size());
            stringCount_.fetch_add(1, std::memory_order_relaxed);
        }
        return stored;
    }

    void StringMemoryManager::ClearStrings() {
        internSet_.clear();
        arena_.Clear();
        stringCount_.store(0, std::memory_order_relaxed);
    }

    size_t StringMemoryManager::GetStringCount() const noexcept {
        return stringCount_.load(std::memory_order_relaxed);
    }

    size_t StringMemoryManager::GetTotalBytes() const noexcept {
        return arena_.GetTotalStored();
    }

    // =========================================================================
    // StringArena — implementation
    // =========================================================================

    const char* StringMemoryManager::StringArena::Store(std::string_view str) {
        const size_t required = str.length() + 1;  // +1 for null terminator

        // Try to fit in the last block
        if (!blocks_.empty()) {
            auto& back = blocks_.back();
            if (back.used + required <= back.capacity) {
                char* dest = back.buffer.get() + back.used;
                std::memcpy(dest, str.data(), str.length());
                dest[str.length()] = '\0';
                back.used += required;
                totalStored_ += required;
                return dest;
            }
        }

        // Allocate a new block (via operator new → engine Memory)
        size_t blockCap = std::max(BLOCK_SIZE, required);
        Block block;
        block.buffer   = std::make_unique<char[]>(blockCap);
        block.capacity = blockCap;
        block.used     = 0;

        char* dest = block.buffer.get();
        std::memcpy(dest, str.data(), str.length());
        dest[str.length()] = '\0';
        block.used = required;
        totalStored_ += required;

        blocks_.push_back(std::move(block));
        return dest;
    }

    void StringMemoryManager::StringArena::Clear() {
        blocks_.clear();
        totalStored_ = 0;
    }

} // namespace shine::reflection
