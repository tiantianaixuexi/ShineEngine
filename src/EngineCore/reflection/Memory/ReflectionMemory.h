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
    // Reflection-specific memory tags (fine-grained, bitmask)
    // =========================================================================

    enum class MemoryTag : uint32_t {
        None             = 0,
        TypeInfo         = 1 << 0,
        FieldInfo        = 1 << 1,
        MethodInfo       = 1 << 2,
        StringStorage    = 1 << 3,
        HashTables       = 1 << 4,
        TemporaryBuffers = 1 << 5,
        CacheData        = 1 << 6
    };

} // namespace shine::reflection  (close for ENABLE_ENUM_FLAGS)

ENABLE_ENUM_FLAGS(shine::reflection::MemoryTag)

namespace shine::reflection {

    // =========================================================================
    // MemoryStats — atomic, lock-free statistics
    // =========================================================================

    struct MemoryStats {
        size_t totalAllocated   = 0;
        size_t peakUsage        = 0;
        size_t allocationCount  = 0;
        size_t deallocationCount = 0;

        // Per-tag byte counters
        size_t typeInfoBytes  = 0;
        size_t fieldInfoBytes = 0;
        size_t stringBytes    = 0;
        size_t cacheBytes     = 0;

        double hitRate       = 0.0;  // pool hit rate
        double fragmentation = 0.0;  // memory fragmentation estimate
    };

    // =========================================================================
    // LinearAllocator — lightweight arena with Reset support
    // =========================================================================
    //
    // Similar to ArenaAllocator but designed for general-purpose use cases
    // like per-frame rendering data. Uses engine Memory allocator for blocks.
    //

    class LinearAllocator {
    public:
        explicit LinearAllocator(size_t pageSize = 64 * 1024) 
            : pageSize_(pageSize) {}

        ~LinearAllocator() { Clear(); }

        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t)) {
            size_t aligned = (currentOffset_ + alignment - 1) & ~(alignment - 1);
            if (currentPage_ < pages_.size() && aligned + size <= pages_[currentPage_].capacity) {
                void* ptr = static_cast<char*>(pages_[currentPage_].data) + aligned;
                currentOffset_ = aligned + size;
                return ptr;
            }
            return AllocateNewPage(size, alignment);
        }

        template<typename T, typename... Args>
        T* New(Args&&... args) {
            void* p = Allocate(sizeof(T), alignof(T));
            return new(p) T(std::forward<Args>(args)...);
        }

        void Reset() {
            currentPage_ = 0;
            currentOffset_ = 0;
        }

        void Clear() {
            for (auto& page : pages_) {
                shine::co::Memory::Free(page.data);
            }
            pages_.clear();
            currentPage_ = 0;
            currentOffset_ = 0;
        }

    private:
        struct Page {
            void* data;
            size_t capacity;
        };
        std::vector<Page> pages_;
        size_t pageSize_;
        size_t currentPage_ = 0;
        size_t currentOffset_ = 0;

        void* AllocateNewPage(size_t size, size_t alignment) {
            while (currentPage_ + 1 < pages_.size()) {
                currentPage_++;
                size_t aligned = (alignment - 1) & ~(alignment - 1);
                if (aligned + size <= pages_[currentPage_].capacity) {
                    currentOffset_ = aligned + size;
                    return static_cast<char*>(pages_[currentPage_].data) + aligned;
                }
            }
            size_t needed = std::max(pageSize_, size + alignment);
            void* mem = shine::co::Memory::Alloc(needed);
            pages_.push_back({mem, needed});
            currentPage_ = pages_.size() - 1;
            size_t aligned = (size_t(mem) + alignment - 1) & ~(alignment - 1);
            currentOffset_ = (aligned - size_t(mem)) + size;
            return (void*)aligned;
        }
    };

    // =========================================================================
    // ReflectionMemoryManager
    // =========================================================================

    class ReflectionMemoryManager {
    public:
        static ReflectionMemoryManager& GetInstance() {
            static ReflectionMemoryManager instance;
            return instance;
        }

        ReflectionMemoryManager(const ReflectionMemoryManager&) = delete;
        ReflectionMemoryManager& operator=(const ReflectionMemoryManager&) = delete;

        // ---- Allocation ----

        template <typename T = void>
        [[nodiscard]]
        T* Allocate(size_t size, MemoryTag tag = MemoryTag::None,
                    size_t alignment = alignof(std::max_align_t)) {
            return static_cast<T*>(AllocateImpl(size, tag, alignment));
        }

        void Deallocate(void* ptr, size_t size, MemoryTag tag = MemoryTag::None) {
            DeallocateImpl(ptr, size, tag);
        }

        // ---- Frame lifecycle ----

        /// Reset temporary arena (call once per frame).
        void ResetFrame() {
            arena_.Reset();
            UpdateStatistics();
        }

        // ---- Statistics (lock-free snapshot) ----

        [[nodiscard]]
        MemoryStats GetStatistics() const noexcept {
            MemoryStats s;
            s.totalAllocated   = totalAllocated_.load(std::memory_order_relaxed);
            s.peakUsage        = peakUsage_.load(std::memory_order_relaxed);
            s.allocationCount  = allocationCount_.load(std::memory_order_relaxed);
            s.deallocationCount = deallocationCount_.load(std::memory_order_relaxed);
            s.typeInfoBytes    = typeInfoBytes_.load(std::memory_order_relaxed);
            s.fieldInfoBytes   = fieldInfoBytes_.load(std::memory_order_relaxed);
            s.stringBytes      = stringBytes_.load(std::memory_order_relaxed);
            s.cacheBytes       = cacheBytes_.load(std::memory_order_relaxed);
            s.hitRate          = hitRate_.load(std::memory_order_relaxed);
            s.fragmentation    = fragmentation_.load(std::memory_order_relaxed);
            return s;
        }

        /// Release everything (temporary + pools + stats).
        void ForceCleanup() {
            arena_.Clear();
            totalAllocated_.store(0, std::memory_order_relaxed);
            peakUsage_.store(0, std::memory_order_relaxed);
            allocationCount_.store(0, std::memory_order_relaxed);
            deallocationCount_.store(0, std::memory_order_relaxed);
            typeInfoBytes_.store(0, std::memory_order_relaxed);
            fieldInfoBytes_.store(0, std::memory_order_relaxed);
            stringBytes_.store(0, std::memory_order_relaxed);
            cacheBytes_.store(0, std::memory_order_relaxed);
            hitRate_.store(0.0, std::memory_order_relaxed);
            fragmentation_.store(0.0, std::memory_order_relaxed);
        }

    private:
        ReflectionMemoryManager() = default;
        ~ReflectionMemoryManager() = default;

        void* AllocateImpl(size_t size, MemoryTag tag, size_t alignment);
        void  DeallocateImpl(void* ptr, size_t size, MemoryTag tag);
        void  UpdateStatistics();

        // Temporary arena (frame-lifetime data)
        LinearAllocator arena_;

        // ---- Atomic statistics (no mutex needed) ----
        std::atomic<size_t>   totalAllocated_{0};
        std::atomic<size_t>   peakUsage_{0};
        std::atomic<size_t>   allocationCount_{0};
        std::atomic<size_t>   deallocationCount_{0};
        std::atomic<size_t>   typeInfoBytes_{0};
        std::atomic<size_t>   fieldInfoBytes_{0};
        std::atomic<size_t>   stringBytes_{0};
        std::atomic<size_t>   cacheBytes_{0};
        std::atomic<double>   hitRate_{0.0};
        std::atomic<double>   fragmentation_{0.0};

        static constexpr size_t SMALL_OBJECT_THRESHOLD = 512;
    };

    // =========================================================================
    // MemoryGuard — RAII scoped allocation wrapper
    // =========================================================================

    template <typename T>
    class MemoryGuard {
    public:
        MemoryGuard(MemoryTag tag = MemoryTag::None)
            : ptr_(ReflectionMemoryManager::GetInstance().template Allocate<T>(sizeof(T), tag))
            , tag_(tag) {}

        explicit MemoryGuard(T* ptr, MemoryTag tag = MemoryTag::None)
            : ptr_(ptr), tag_(tag) {}

        ~MemoryGuard() {
            if (ptr_) {
                ReflectionMemoryManager::GetInstance().Deallocate(ptr_, sizeof(T), tag_);
            }
        }

        MemoryGuard(const MemoryGuard&) = delete;
        MemoryGuard& operator=(const MemoryGuard&) = delete;

        MemoryGuard(MemoryGuard&& other) noexcept
            : ptr_(other.ptr_), tag_(other.tag_) {
            other.ptr_ = nullptr;
        }

        MemoryGuard& operator=(MemoryGuard&& other) noexcept {
            if (this != &other) {
                if (ptr_) {
                    ReflectionMemoryManager::GetInstance().Deallocate(ptr_, sizeof(T), tag_);
                }
                ptr_ = other.ptr_;
                tag_ = other.tag_;
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
        MemoryTag tag_;
    };

    // =========================================================================
    // StringMemoryManager — interned string pool with deduplication
    // =========================================================================
    //
    // Uses a hash set to deduplicate strings and a linear arena
    // for storage.  This is critical because reflection registers
    // many duplicate type/field names across translation units.
    //

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
