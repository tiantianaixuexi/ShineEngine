#pragma once

// =============================================================================
// ReflectionMemory.h — Reflection-specific memory management
// =============================================================================
//
// All allocations are routed through shine::co::Memory (mimalloc backend)
// and tagged with MemoryTag::Reflection for unified profiling.
//
// Key subsystems:
//   ReflectionMemoryManager — tagged allocator with atomic stats
//   ArenaAllocator          — O(1) bump allocator for temporary / frame data
//   StringMemoryManager     — interned string pool with deduplication
//
// C++23 / MSVC
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>
#include <unordered_set>
#include <functional>

#include "util/EnumFlags.h"
#include "memory/memory.ixx"

namespace shine::reflection {

    // =========================================================================
    // Reflection uses engine's main memory tags
    // Fine-grained tracking is done via engine's MemoryTag::Reflection
    // =========================================================================

} // namespace shine::reflection  (close for ENABLE_ENUM_FLAGS)

// Reflection uses engine's memory tagging system

namespace shine::reflection {

    // =========================================================================
    // MemoryStats — uses engine's unified statistics
    // =========================================================================

    struct MemoryStats {
        size_t totalAllocated   = 0;
        size_t peakUsage        = 0;
        size_t allocationCount  = 0;
        size_t deallocationCount = 0;

        // Use engine's MemoryTag::Reflection stats instead of per-subtype tracking
        double hitRate       = 0.0;  // pool hit rate
        double fragmentation = 0.0;  // memory fragmentation estimate
    };

    // =========================================================================
    // LinearAllocator removed - use engine's memory system directly
    // For temporary/frame data, use stack allocation or engine's facilities
    // =========================================================================

    // =========================================================================
    // ReflectionMemoryManager - simplified interface using engine memory
    // =========================================================================

    class ReflectionMemoryManager {
    public:
        static ReflectionMemoryManager& GetInstance() {
            static ReflectionMemoryManager instance;
            return instance;
        }

        ReflectionMemoryManager(const ReflectionMemoryManager&) = delete;
        ReflectionMemoryManager& operator=(const ReflectionMemoryManager&) = delete;

        // ---- Allocation (delegates to engine memory) ----

        template <typename T = void>
        [[nodiscard]]
        T* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            // Use engine's memory system with Reflection tag
            shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
            return static_cast<T*>(shine::co::Memory::Alloc(size, alignment));
        }

        void Deallocate(void* ptr) {
            shine::co::Memory::Free(ptr);
        }

        // ---- Frame lifecycle ----

        /// Reset temporary data (no-op since we use engine's memory system)
        void ResetFrame() {
            // Engine handles frame-based memory management
            // Just flush stats for accurate reporting
            shine::co::Memory::FlushAllThreadStats();
        }

        // ---- Statistics (delegates to engine) ----

        [[nodiscard]]
        MemoryStats GetStatistics() const noexcept {
            auto engineStats = shine::co::Memory::GetTagStats(shine::co::MemoryTag::Reflection);
            
            MemoryStats s;
            s.totalAllocated   = engineStats.bytes_current;
            s.peakUsage        = engineStats.bytes_peak;
            s.allocationCount  = engineStats.alloc_count;
            s.deallocationCount = engineStats.free_count;
            
            // Calculate derived metrics
            s.hitRate = (s.allocationCount > 0) 
                ? (static_cast<double>(s.allocationCount - s.deallocationCount) / 
                   static_cast<double>(s.allocationCount)) * 100.0
                : 0.0;
            
            s.fragmentation = (s.peakUsage > 0)
                ? (1.0 - static_cast<double>(s.totalAllocated) / 
                   static_cast<double>(s.peakUsage)) * 100.0
                : 0.0;
            
            return s;
        }

        /// Release everything (delegates to engine cleanup)
        void ForceCleanup() {
            // Engine memory system handles cleanup
            // Just ensure all stats are flushed
            shine::co::Memory::FlushAllThreadStats();
        }

    private:
        ReflectionMemoryManager() = default;
        ~ReflectionMemoryManager() = default;
    };

    // =========================================================================
    // MemoryGuard - simplified RAII wrapper
    // =========================================================================

    template <typename T>
    class MemoryGuard {
    public:
        MemoryGuard()
            : ptr_(ReflectionMemoryManager::GetInstance().template Allocate<T>(sizeof(T))) {}

        explicit MemoryGuard(T* ptr)
            : ptr_(ptr) {}

        ~MemoryGuard() {
            if (ptr_) {
                ReflectionMemoryManager::GetInstance().Deallocate(ptr_);
            }
        }

        MemoryGuard(const MemoryGuard&) = delete;
        MemoryGuard& operator=(const MemoryGuard&) = delete;

        MemoryGuard(MemoryGuard&& other) noexcept
            : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        MemoryGuard& operator=(MemoryGuard&& other) noexcept {
            if (this != &other) {
                if (ptr_) {
                    ReflectionMemoryManager::GetInstance().Deallocate(ptr_);
                }
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        [[nodiscard]] T* Get() const noexcept { return ptr_; }
        T* operator->() const noexcept { return ptr_; }
        T& operator*() const noexcept { return *ptr_; }
        explicit operator bool() const noexcept { return ptr_ != nullptr; }

    private:
        T* ptr_;
    };

    // =========================================================================
    // StringMemoryManager - keep existing functionality but simplify
    // =========================================================================
    class StringMemoryManager {
    public:
        static StringMemoryManager& GetInstance() {
            static StringMemoryManager instance;
            return instance;
        }

        /// Store (or find existing) a string.  The returned pointer
        /// is valid for the lifetime of the manager.
        const char* StoreString(std::string_view str);

        void ClearStrings();

        [[nodiscard]] size_t GetStringCount() const noexcept;
        [[nodiscard]] size_t GetTotalBytes()  const noexcept;

    private:
        StringMemoryManager() = default;
        ~StringMemoryManager();

        // ---- String arena (linear bump allocator for characters) ----
        class StringArena {
        public:
            static constexpr size_t BLOCK_SIZE = 8192;  // 8 KB per block

            /// Allocate room for 'len+1' bytes, copy str, null-terminate.
            const char* Store(std::string_view str);

            void Clear();

            [[nodiscard]] size_t GetTotalStored() const noexcept { return totalStored_; }

        private:
            struct Block {
                std::unique_ptr<char[]> buffer;
                size_t capacity = 0;
                size_t used     = 0;
            };
            std::vector<Block> blocks_;
            size_t totalStored_ = 0;
        };

        // ---- Deduplication hash set ----
        //
        // We store string_views that point into the arena.
        // On lookup we hash/compare the *content*, not the pointer.

        struct SVHash {
            using is_transparent = void;
            size_t operator()(std::string_view sv) const noexcept {
                // FNV-1a 64-bit
                size_t h = 14695981039346656037ULL;
                for (char c : sv) {
                    h ^= static_cast<size_t>(static_cast<unsigned char>(c));
                    h *= 1099511628211ULL;
                }
                return h;
            }
        };

        struct SVEqual {
            using is_transparent = void;
            bool operator()(std::string_view a, std::string_view b) const noexcept {
                return a == b;
            }
        };

        StringArena arena_;
        std::unordered_set<std::string_view, SVHash, SVEqual> internSet_;
        mutable std::atomic<size_t> stringCount_{0};
    };

} // namespace shine::reflection
