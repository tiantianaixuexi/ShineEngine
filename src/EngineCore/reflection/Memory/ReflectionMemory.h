#pragma once

// =============================================================================
// ReflectionMemory.h — Reflection-specific memory management
// =============================================================================
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
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <vector>
#include <unordered_set>
#include <functional>
#include <cstring>

#include "string/shine_text_view.h"
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
            // Reflection metadata objects should not be merged with temp/script traffic.
            shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionMeta);
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
            auto engineStats = shine::co::Memory::GetTagStats(shine::co::MemoryTag::ReflectionMeta);
            
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

    template <typename T>
    class ReflectionArena {
    public:
        static constexpr size_t kTargetPageBytes = 16 * 1024;

        ReflectionArena() = default;
        ~ReflectionArena() {
            Clear();
        }

        ReflectionArena(const ReflectionArena&) = delete;
        ReflectionArena& operator=(const ReflectionArena&) = delete;

        template <typename... TArgs>
        [[nodiscard]] T* Create(TArgs&&... args) {
            void* slotMemory = nullptr;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (activePageIndex_ >= pages_.size() || pages_[activePageIndex_].used >= pages_[activePageIndex_].capacity) {
                    AllocatePageLocked();
                }

                if (activePageIndex_ >= pages_.size()) {
                    return nullptr;
                }

                auto& page = pages_[activePageIndex_];
                slotMemory = page.memory + (page.used * kSlotStride);
                ++page.used;
                ++liveCount_;
            }

            return std::construct_at(static_cast<T*>(slotMemory), std::forward<TArgs>(args)...);
        }

        void Clear() noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& page : pages_) {
                for (size_t slotIndex = 0; slotIndex < page.used; ++slotIndex) {
                    auto* object = std::launder(reinterpret_cast<T*>(page.memory + (slotIndex * kSlotStride)));
                    std::destroy_at(object);
                }
                shine::co::Memory::Free(page.memory);
            }
            pages_.clear();
            activePageIndex_ = kInvalidPageIndex;
            liveCount_ = 0;
        }

        [[nodiscard]] size_t PageCount() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return pages_.size();
        }

        [[nodiscard]] size_t LiveCount() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return liveCount_;
        }

        [[nodiscard]] size_t SlotsPerPage() const noexcept {
            return kSlotsPerPage;
        }

        [[nodiscard]] size_t PageBytes() const noexcept {
            return kPageBytes;
        }

        [[nodiscard]] size_t PageIndexOf(const T* ptr) const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return FindPageIndexForSlotLocked(ptr);
        }

    private:
        struct Page {
            std::byte* memory = nullptr;
            size_t capacity = 0;
            size_t used = 0;
        };

        static constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        void AllocatePageLocked() {
            shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionMeta);
            void* pageMemory = shine::co::Memory::Alloc(kPageBytes, kSlotAlignment);
            if (!pageMemory) {
                return;
            }

            Page page{};
            page.memory = static_cast<std::byte*>(pageMemory);
            page.capacity = kSlotsPerPage;
            page.used = 0;
            pages_.push_back(page);
            activePageIndex_ = pages_.size() - 1;
        }

        [[nodiscard]] size_t FindPageIndexForSlotLocked(const void* slotMemory) const noexcept {
            if (!slotMemory) {
                return kInvalidPageIndex;
            }

            const auto slotAddress = reinterpret_cast<std::uintptr_t>(slotMemory);
            for (size_t pageIndex = 0; pageIndex < pages_.size(); ++pageIndex) {
                const auto& page = pages_[pageIndex];
                const auto pageBegin = reinterpret_cast<std::uintptr_t>(page.memory);
                const auto pageEnd = pageBegin + kPageBytes;
                if (slotAddress >= pageBegin && slotAddress < pageEnd) {
                    return pageIndex;
                }
            }
            return kInvalidPageIndex;
        }

        static constexpr size_t kSlotAlignment = alignof(T);
        static constexpr size_t kSlotStride = AlignUp(sizeof(T), kSlotAlignment);
        static constexpr size_t kSlotsPerPage = (std::max)(size_t{16}, kTargetPageBytes / kSlotStride);
        static constexpr size_t kPageBytes = kSlotStride * kSlotsPerPage;
        static constexpr size_t kInvalidPageIndex = static_cast<size_t>(-1);

        mutable std::mutex mutex_;
        std::vector<Page> pages_;
        size_t activePageIndex_ = kInvalidPageIndex;
        size_t liveCount_ = 0;
    };

    template <typename T>
    class ReflectionColdPool;

    template <typename T>
    struct ReflectionColdDeleter {
        void operator()(T* ptr) const noexcept {
            ReflectionColdPool<T>::Get().Destroy(ptr);
        }
    };

    template <typename T>
    using ReflectionColdPtr = std::unique_ptr<T, ReflectionColdDeleter<T>>;

    template <typename T>
    class ReflectionColdVector {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using iterator = T*;
        using const_iterator = const T*;

        ReflectionColdVector() = default;

        ~ReflectionColdVector() {
            Release();
        }

        ReflectionColdVector(const ReflectionColdVector&) = delete;
        ReflectionColdVector& operator=(const ReflectionColdVector&) = delete;

        ReflectionColdVector(ReflectionColdVector&& other) noexcept
            : data_(other.data_)
            , size_(other.size_)
            , capacity_(other.capacity_) {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        ReflectionColdVector& operator=(ReflectionColdVector&& other) noexcept {
            if (this != &other) {
                Release();
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        void reserve(size_type newCapacity) {
            if (newCapacity <= capacity_) {
                return;
            }

            shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionCold);
            auto* newData = static_cast<T*>(shine::co::Memory::Alloc(sizeof(T) * newCapacity, alignof(T)));
            assert(newData && "ReflectionColdVector::reserve failed");
            if (!newData) {
                return;
            }

            for (size_type index = 0; index < size_; ++index) {
                std::construct_at(newData + index, std::move(data_[index]));
                std::destroy_at(data_ + index);
            }

            if (data_ != nullptr) {
                shine::co::Memory::Free(data_);
            }

            data_ = newData;
            capacity_ = newCapacity;
        }

        void resize(size_type newSize) {
            if (newSize > capacity_) {
                reserve(newSize);
            }

            if (newSize > size_) {
                for (size_type index = size_; index < newSize; ++index) {
                    std::construct_at(data_ + index);
                }
            } else {
                for (size_type index = newSize; index < size_; ++index) {
                    std::destroy_at(data_ + index);
                }
            }

            size_ = newSize;
        }

        template <typename... TArgs>
        T& emplace_back(TArgs&&... args) {
            if (size_ == capacity_) {
                reserve(capacity_ == 0 ? 4 : capacity_ * 2);
            }

            std::construct_at(data_ + size_, std::forward<TArgs>(args)...);
            return data_[size_++];
        }

        void push_back(const T& value) {
            (void)emplace_back(value);
        }

        void push_back(T&& value) {
            (void)emplace_back(std::move(value));
        }

        void clear() noexcept {
            for (size_type index = 0; index < size_; ++index) {
                std::destroy_at(data_ + index);
            }
            size_ = 0;
        }

        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

        [[nodiscard]] T* data() noexcept { return data_; }
        [[nodiscard]] const T* data() const noexcept { return data_; }

        [[nodiscard]] T& operator[](size_type index) noexcept { return data_[index]; }
        [[nodiscard]] const T& operator[](size_type index) const noexcept { return data_[index]; }

        [[nodiscard]] T& back() noexcept { return data_[size_ - 1]; }
        [[nodiscard]] const T& back() const noexcept { return data_[size_ - 1]; }

        [[nodiscard]] iterator begin() noexcept { return data_; }
        [[nodiscard]] iterator end() noexcept { return data_ + size_; }
        [[nodiscard]] const_iterator begin() const noexcept { return data_; }
        [[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }
        [[nodiscard]] const_iterator cbegin() const noexcept { return data_; }
        [[nodiscard]] const_iterator cend() const noexcept { return data_ + size_; }

    private:
        void Release() noexcept {
            clear();
            if (data_ != nullptr) {
                shine::co::Memory::Free(data_);
                data_ = nullptr;
            }
            capacity_ = 0;
        }

        T* data_ = nullptr;
        size_type size_ = 0;
        size_type capacity_ = 0;
    };

    template <typename T>
    class ReflectionColdPool {
    public:
        static constexpr size_t kTargetPageBytes = 16 * 1024;

        class ContiguousBatch {
        public:
            ContiguousBatch() = default;

            ContiguousBatch(ReflectionColdPool* owner, size_t expectedCount) noexcept
                : owner_(owner), remaining_(expectedCount) {}

            ContiguousBatch(const ContiguousBatch&) = delete;
            ContiguousBatch& operator=(const ContiguousBatch&) = delete;

            ContiguousBatch(ContiguousBatch&& other) noexcept
                : owner_(other.owner_)
                , pageIndex_(other.pageIndex_)
                , cursor_(other.cursor_)
                , reservedBegin_(other.reservedBegin_)
                , reservedEnd_(other.reservedEnd_)
                , remaining_(other.remaining_)
                , issuedCount_(other.issuedCount_) {
                other.owner_ = nullptr;
                other.pageIndex_ = kInvalidPageIndex;
                other.cursor_ = 0;
                other.reservedBegin_ = 0;
                other.reservedEnd_ = 0;
                other.remaining_ = 0;
                other.issuedCount_ = 0;
            }

            ContiguousBatch& operator=(ContiguousBatch&& other) noexcept {
                if (this != &other) {
                    Reset();
                    owner_ = other.owner_;
                    pageIndex_ = other.pageIndex_;
                    cursor_ = other.cursor_;
                    reservedBegin_ = other.reservedBegin_;
                    reservedEnd_ = other.reservedEnd_;
                    remaining_ = other.remaining_;
                    issuedCount_ = other.issuedCount_;
                    other.owner_ = nullptr;
                    other.pageIndex_ = kInvalidPageIndex;
                    other.cursor_ = 0;
                    other.reservedBegin_ = 0;
                    other.reservedEnd_ = 0;
                    other.remaining_ = 0;
                    other.issuedCount_ = 0;
                }
                return *this;
            }

            ~ContiguousBatch() {
                Reset();
            }

            void Reset() noexcept {
                owner_ = nullptr;
                pageIndex_ = kInvalidPageIndex;
                cursor_ = 0;
                reservedBegin_ = 0;
                reservedEnd_ = 0;
                remaining_ = 0;
                issuedCount_ = 0;
            }

            [[nodiscard]] explicit operator bool() const noexcept {
                return owner_ != nullptr;
            }

            [[nodiscard]] size_t ReservedPageIndex() const noexcept {
                return pageIndex_;
            }

            [[nodiscard]] size_t ReservedBeginSlot() const noexcept {
                return reservedBegin_;
            }

            [[nodiscard]] size_t ReservedEndSlot() const noexcept {
                return reservedEnd_;
            }

            [[nodiscard]] size_t Remaining() const noexcept {
                return remaining_;
            }

            [[nodiscard]] size_t IssuedCount() const noexcept {
                return issuedCount_;
            }

            template <typename... TArgs>
            [[nodiscard]] ReflectionColdPtr<T> Create(TArgs&&... args) {
                if (!owner_) {
                    return {};
                }

                T* ptr = owner_->CreateReserved(*this, std::forward<TArgs>(args)...);
                assert(ptr && "ReflectionColdPool::ContiguousBatch::Create failed");
                return ReflectionColdPtr<T>(ptr);
            }

        private:
            friend class ReflectionColdPool;

            ReflectionColdPool* owner_ = nullptr;
            size_t pageIndex_ = kInvalidPageIndex;
            size_t cursor_ = 0;
            size_t reservedBegin_ = 0;
            size_t reservedEnd_ = 0;
            size_t remaining_ = 0;
            size_t issuedCount_ = 0;
        };

        static ReflectionColdPool& Get() {
            static ReflectionColdPool pool;
            return pool;
        }

        ReflectionColdPool(const ReflectionColdPool&) = delete;
        ReflectionColdPool& operator=(const ReflectionColdPool&) = delete;

        template <typename... TArgs>
        [[nodiscard]] T* Create(TArgs&&... args) {
            void* slotMemory = nullptr;
            Page* page = nullptr;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (freeList_) {
                    auto* recycledSlot = freeList_;
                    freeList_ = freeList_->next;
                    slotMemory = recycledSlot;
                    page = FindPageForSlotLocked(slotMemory);
                } else {
                    if (activePageIndex_ >= pages_.size() || pages_[activePageIndex_].nextUnused >= pages_[activePageIndex_].capacity) {
                        AllocatePageLocked();
                    }
                    if (activePageIndex_ < pages_.size()) {
                        page = &pages_[activePageIndex_];
                        slotMemory = page->memory + (page->nextUnused * kSlotStride);
                        ++page->nextUnused;
                    }
                }

                if (!slotMemory || !page) {
                    return nullptr;
                }
                ++page->liveCount;
                ++liveCount_;
            }

            return std::construct_at(static_cast<T*>(slotMemory), std::forward<TArgs>(args)...);
        }

        void Destroy(T* ptr) noexcept {
            if (!ptr) {
                return;
            }

            std::destroy_at(ptr);

            std::lock_guard<std::mutex> lock(mutex_);
            Page* page = FindPageForSlotLocked(ptr);
            auto* slot = static_cast<Slot*>(static_cast<void*>(ptr));
            slot->next = freeList_;
            freeList_ = slot;
            if (page && page->liveCount > 0) {
                --page->liveCount;
            }
            if (liveCount_ > 0) {
                --liveCount_;
            }
        }

        [[nodiscard]] ContiguousBatch BeginContiguousBatch(size_t expectedCount) {
            if (expectedCount == 0) {
                return {};
            }

            std::lock_guard<std::mutex> lock(mutex_);
            ContiguousBatch batch(this, expectedCount);
            if (!ReserveBatchChunkLocked(batch)) {
                return {};
            }
            return batch;
        }

        [[nodiscard]] size_t BlockCount() const noexcept {
            return PageCount();
        }

        [[nodiscard]] size_t PageCount() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return pages_.size();
        }

        [[nodiscard]] size_t LiveCount() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return liveCount_;
        }

        [[nodiscard]] size_t LivePageCount() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            size_t livePages = 0;
            for (const auto& page : pages_) {
                if (page.liveCount != 0) {
                    ++livePages;
                }
            }
            return livePages;
        }

        [[nodiscard]] size_t SlotsPerPage() const noexcept {
            return kSlotsPerPage;
        }

        [[nodiscard]] size_t PageBytes() const noexcept {
            return kPageBytes;
        }

        [[nodiscard]] size_t LastPageCommittedSlots() const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            if (pages_.empty()) {
                return 0;
            }
            return pages_.back().nextUnused;
        }

        [[nodiscard]] size_t PageIndexOf(const T* ptr) const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            return FindPageIndexForSlotLocked(ptr);
        }

        [[nodiscard]] size_t SlotIndexInPageOf(const T* ptr) const noexcept {
            std::lock_guard<std::mutex> lock(mutex_);
            const size_t pageIndex = FindPageIndexForSlotLocked(ptr);
            if (pageIndex == kInvalidPageIndex) {
                return kInvalidPageIndex;
            }

            const auto& page = pages_[pageIndex];
            const auto pageBegin = reinterpret_cast<std::uintptr_t>(page.memory);
            const auto slotAddress = reinterpret_cast<std::uintptr_t>(ptr);
            return (slotAddress - pageBegin) / kSlotStride;
        }

    private:
        ReflectionColdPool() = default;
        ~ReflectionColdPool() {
            for (const auto& page : pages_) {
                shine::co::Memory::Free(page.memory);
            }
        }

        struct Slot {
            Slot* next;
        };

        struct Page {
            std::byte* memory = nullptr;
            size_t capacity = 0;
            size_t nextUnused = 0;
            size_t liveCount = 0;
        };

        static constexpr size_t AlignUp(size_t value, size_t alignment) noexcept {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        [[nodiscard]] static constexpr size_t RemainingSlotsLocked(const Page& page) noexcept {
            return page.capacity - page.nextUnused;
        }

        void AllocatePageLocked() {
            shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionCold);
            void* pageMemory = shine::co::Memory::Alloc(kPageBytes, kSlotAlignment);
            if (!pageMemory) {
                return;
            }

            Page page{};
            page.memory = static_cast<std::byte*>(pageMemory);
            page.capacity = kSlotsPerPage;
            page.nextUnused = 0;
            page.liveCount = 0;
            pages_.push_back(page);
            activePageIndex_ = pages_.size() - 1;
        }

        [[nodiscard]] bool ReserveBatchChunkLocked(ContiguousBatch& batch) {
            if (batch.remaining_ == 0) {
                return true;
            }

            const size_t minContiguousSlots = (std::min)(batch.remaining_, kSlotsPerPage);
            if (activePageIndex_ >= pages_.size() || RemainingSlotsLocked(pages_[activePageIndex_]) < minContiguousSlots) {
                AllocatePageLocked();
            }

            if (activePageIndex_ >= pages_.size()) {
                return false;
            }

            auto& page = pages_[activePageIndex_];
            const size_t reserveCount = (std::min)(batch.remaining_, RemainingSlotsLocked(page));
            if (reserveCount == 0) {
                return false;
            }

            batch.pageIndex_ = activePageIndex_;
            batch.cursor_ = page.nextUnused;
            batch.reservedBegin_ = page.nextUnused;
            batch.reservedEnd_ = page.nextUnused + reserveCount;
            page.nextUnused += reserveCount;
            return true;
        }

        template <typename... TArgs>
        [[nodiscard]] T* CreateReserved(ContiguousBatch& batch, TArgs&&... args) {
            void* slotMemory = nullptr;
            Page* page = nullptr;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (batch.owner_ != this || batch.remaining_ == 0) {
                    return nullptr;
                }

                if (batch.cursor_ >= batch.reservedEnd_) {
                    if (!ReserveBatchChunkLocked(batch)) {
                        return nullptr;
                    }
                }

                if (batch.pageIndex_ >= pages_.size()) {
                    return nullptr;
                }

                page = &pages_[batch.pageIndex_];
                slotMemory = page->memory + (batch.cursor_ * kSlotStride);
                ++batch.cursor_;
                --batch.remaining_;
                ++batch.issuedCount_;
                ++page->liveCount;
                ++liveCount_;
            }

            return std::construct_at(static_cast<T*>(slotMemory), std::forward<TArgs>(args)...);
        }

        Page* FindPageForSlotLocked(const void* slotMemory) {
            const size_t pageIndex = FindPageIndexForSlotLocked(slotMemory);
            if (pageIndex == kInvalidPageIndex) {
                return nullptr;
            }
            return &pages_[pageIndex];
        }

        [[nodiscard]] size_t FindPageIndexForSlotLocked(const void* slotMemory) const noexcept {
            if (!slotMemory) {
                return kInvalidPageIndex;
            }

            const auto slotAddress = reinterpret_cast<std::uintptr_t>(slotMemory);
            for (size_t pageIndex = 0; pageIndex < pages_.size(); ++pageIndex) {
                const auto& page = pages_[pageIndex];
                const auto pageBegin = reinterpret_cast<std::uintptr_t>(page.memory);
                const auto pageEnd = pageBegin + kPageBytes;
                if (slotAddress >= pageBegin && slotAddress < pageEnd) {
                    return pageIndex;
                }
            }
            return kInvalidPageIndex;
        }

        static constexpr size_t kSlotAlignment = (std::max)(alignof(T), alignof(Slot));
        static constexpr size_t kSlotStride = AlignUp((std::max)(sizeof(T), sizeof(Slot)), kSlotAlignment);
        static constexpr size_t kSlotsPerPage = (std::max)(size_t{32}, kTargetPageBytes / kSlotStride);
        static constexpr size_t kPageBytes = kSlotStride * kSlotsPerPage;
        static constexpr size_t kInvalidPageIndex = static_cast<size_t>(-1);

        mutable std::mutex mutex_;
        Slot* freeList_ = nullptr;
        std::vector<Page> pages_;
        size_t activePageIndex_ = kInvalidPageIndex;
        size_t liveCount_ = 0;
    };

    template <typename T, typename... TArgs>
    [[nodiscard]] ReflectionColdPtr<T> MakeReflectionColdData(TArgs&&... args) {
        T* ptr = ReflectionColdPool<T>::Get().Create(std::forward<TArgs>(args)...);
        assert(ptr && "MakeReflectionColdData: pool allocation failed");
        return ReflectionColdPtr<T>(ptr);
    }

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
        const char* StoreString(shine::STextView str) {
            if (auto it = internSet_.find(str); it != internSet_.end()) {
                return it->data();
            }

            const char* stored = arena_.Store(str);
            if (stored) {
                internSet_.emplace(stored, str.size());
                stringCount_.fetch_add(1, std::memory_order_relaxed);
            }
            return stored;
        }

        void ClearStrings() {
            internSet_.clear();
            arena_.Clear();
            stringCount_.store(0, std::memory_order_relaxed);
        }

        [[nodiscard]] size_t GetStringCount() const noexcept {
            return stringCount_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] size_t GetTotalBytes()  const noexcept {
            return arena_.GetTotalStored();
        }

    private:
        StringMemoryManager() = default;
        ~StringMemoryManager() {
            ClearStrings();
        }

        // ---- String arena (linear bump allocator for characters) ----
        class StringArena {
        public:
            static constexpr size_t BLOCK_SIZE = 8192;  // 8 KB per block

            /// Allocate room for 'len+1' bytes, copy str, null-terminate.
            const char* Store(shine::STextView str) {
                const size_t required = str.size() + 1;

                if (!blocks_.empty()) {
                    auto& back = blocks_.back();
                    if (back.used + required <= back.capacity) {
                        char* dest = back.buffer.get() + back.used;
                        std::memcpy(dest, str.data(), str.size());
                        dest[str.size()] = '\0';
                        back.used += required;
                        totalStored_ += required;
                        return dest;
                    }
                }

                size_t blockCap = (std::max)(BLOCK_SIZE, required);
                Block block;
                {
                    shine::co::MemoryScope scope(shine::co::MemoryTag::ReflectionString);
                    block.buffer = std::make_unique<char[]>(blockCap);
                }
                block.capacity = blockCap;
                block.used = 0;

                char* dest = block.buffer.get();
                std::memcpy(dest, str.data(), str.size());
                dest[str.size()] = '\0';
                block.used = required;
                totalStored_ += required;

                blocks_.push_back(std::move(block));
                return dest;
            }

            void Clear() {
                blocks_.clear();
                totalStored_ = 0;
            }

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
            size_t operator()(shine::STextView sv) const noexcept {
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
            bool operator()(shine::STextView a, shine::STextView b) const noexcept {
                return a == b;
            }
        };

        StringArena arena_;
        std::unordered_set<shine::STextView, SVHash, SVEqual> internSet_;
        mutable std::atomic<size_t> stringCount_{0};
    };

    [[nodiscard]] inline shine::STextView InternReflectionText(shine::STextView text) {
        if (text.empty()) {
            return {};
        }

        const char* stored = StringMemoryManager::GetInstance().StoreString(text);
        return stored ? shine::STextView(stored, text.size()) : shine::STextView{};
    }

} // namespace shine::reflection
