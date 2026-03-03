#include "ue_binned2_port.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include "util/profiling/shine_profiling.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <malloc.h>
#else
#include <malloc.h>
#endif


namespace shine::co::ue_binned2_port {
    namespace {
        constexpr size_t kPoolPageSize = 65536;
        constexpr uint32_t kPageMagic = 0x53483242u;
        constexpr uint32_t kAlignMagic = 0x53483241u;
        constexpr uint32_t kLargeMagic = 0x5348324Cu;
        constexpr size_t kMinAlign = 16;
        constexpr size_t kMaxSmallPoolSize = 32768 - 16;
        constexpr std::array<uint16_t, 51> kSmallBinSizes = {
            16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208,
            256, 288, 320, 384, 448, 512, 560, 624, 720, 816, 912, 1008,
            1168, 1392, 1520, 1680, 1872, 2032, 2256, 2608, 2976, 3264, 3632,
            4080, 4368, 4672, 5040, 5456, 5952, 6544, 7280, 8176, 9360, 10912,
            13104, 16368, 21840, 32752
        };

        struct FreeNode {
            FreeNode* Next;
        };

        struct PoolPage {
            uint32_t Magic;
            uint16_t PoolIndex;
            uint16_t BinSize;
            uint16_t Capacity;
            uint16_t FreeCount;
            uint8_t InPartialList;
            uint8_t Reserved[7];
            FreeNode* FreeList;
            PoolPage* NextPartial;
            PoolPage* PrevPartial;
        };

        struct AlignHeader {
            uint32_t Magic;
            uint32_t Reserved;
            void* RawPtr;
            size_t RequestedSize;
        };

        struct LargeAllocHeader {
            uint32_t Magic;
            uint32_t Cookie;
            void* BasePtr;
            size_t Size;
        };

        constexpr uint16_t kTlsCacheLimit = 96;
        constexpr size_t kPoolCount = kSmallBinSizes.size();
        constexpr size_t kPoolHeaderBytes = sizeof(PoolPage);
        constexpr size_t kPoolUsableBytes = kPoolPageSize - kPoolHeaderBytes;

        class Allocator {
        public:
            Allocator() {
                BuildSizeMap();
            }

            void* Alloc(size_t size, size_t align) noexcept {
                if (size == 0) {
                    return nullptr;
                }
                if (align < kMinAlign) {
                    align = kMinAlign;
                }
                if ((align & (align - 1)) != 0) {
                    return nullptr;
                }
                if (size > (std::numeric_limits<size_t>::max)() - align - sizeof(AlignHeader)) {
                    return nullptr;
                }
                const size_t rawSize = size + align + sizeof(AlignHeader);
                void* raw = AllocRaw(rawSize);
                if (!raw) {
                    return nullptr;
                }
                uintptr_t begin = reinterpret_cast<uintptr_t>(raw) + sizeof(AlignHeader);
                uintptr_t aligned = (begin + (align - 1)) & ~(align - 1);
                auto* meta = reinterpret_cast<AlignHeader*>(aligned - sizeof(AlignHeader));
                meta->Magic = kAlignMagic;
                meta->RawPtr = raw;
                meta->RequestedSize = size;
                return reinterpret_cast<void*>(aligned);
            }

            void Free(void* p) noexcept {
                if (!p) {
                    return;
                }
                auto* meta = reinterpret_cast<AlignHeader*>(
                    reinterpret_cast<uintptr_t>(p) - sizeof(AlignHeader));
                if (meta->Magic != kAlignMagic) {
                    SHINE_PROFILE_CPU_MESSAGE_L("[UEB2] Free invalid pointer");
                    return;
                }
                void* raw = meta->RawPtr;
                meta->Magic = 0;
                FreeRaw(raw);
            }

            void* Realloc(void* p, size_t size, size_t align) noexcept {
                if (!p) {
                    return Alloc(size, align);
                }
                if (size == 0) {
                    Free(p);
                    return nullptr;
                }
                auto* meta = reinterpret_cast<AlignHeader*>(
                    reinterpret_cast<uintptr_t>(p) - sizeof(AlignHeader));
                if (meta->Magic != kAlignMagic) {
                    SHINE_PROFILE_CPU_MESSAGE_L("[UEB2] Realloc invalid pointer");
                    return nullptr;
                }
                size_t oldSize = meta->RequestedSize;
                void* np = Alloc(size, align);
                if (!np) {
                    return nullptr;
                }
                std::memcpy(np, p, (std::min)(oldSize, size));
                Free(p);
                return np;
            }

            bool GetAllocationSize(void* p, size_t& outSize) noexcept {
                outSize = 0;
                if (!p) {
                    return false;
                }
                auto* meta = reinterpret_cast<AlignHeader*>(
                    reinterpret_cast<uintptr_t>(p) - sizeof(AlignHeader));
                if (meta->Magic != kAlignMagic) {
                    return false;
                }
                outSize = meta->RequestedSize;
                return true;
            }

            void Trim() noexcept {
                SHINE_PROFILE_CPU_SCOPE_N("UEB2::Trim");
                for (uint8_t i = 0; i < static_cast<uint8_t>(kPoolCount); ++i) {
                    FlushThreadCacheToCentral(i, 0);
                }
            }

            bool ValidateHeap() noexcept {
                SHINE_PROFILE_CPU_SCOPE_N("UEB2::ValidateHeap");
                bool ok = true;
                for (uint8_t poolIndex = 0; poolIndex < static_cast<uint8_t>(kPoolCount); ++poolIndex) {
                    std::lock_guard<std::mutex> lock(PoolMutexes[poolIndex]);
                    for (PoolPage* page = PartialLists[poolIndex]; page; page = page->NextPartial) {
                        if (page->Magic != kPageMagic) {
                            ok = false;
                            continue;
                        }
                        if (page->PoolIndex != poolIndex) {
                            ok = false;
                        }
                        if (page->BinSize != kSmallBinSizes[poolIndex]) {
                            ok = false;
                        }
                        if (page->Capacity == 0 || page->Capacity > (kPoolUsableBytes / page->BinSize)) {
                            ok = false;
                        }
                        if (page->FreeCount > page->Capacity) {
                            ok = false;
                        }
                    }
                }
                if (!ok) {
                    SHINE_PROFILE_CPU_MESSAGE_L("[UEB2] ValidateHeap FAILED");
                }
                return ok;
            }

            void DumpAllocatorStats() noexcept {
                SHINE_PROFILE_CPU_SCOPE_N("UEB2::DumpAllocatorStats");
                size_t totalPages = 0;
                size_t totalFree = 0;
                size_t totalCapacity = 0;
                for (uint8_t poolIndex = 0; poolIndex < static_cast<uint8_t>(kPoolCount); ++poolIndex) {
                    std::lock_guard<std::mutex> lock(PoolMutexes[poolIndex]);
                    size_t pages = 0;
                    size_t freeBins = 0;
                    size_t capBins = 0;
                    for (PoolPage* page = PartialLists[poolIndex]; page; page = page->NextPartial) {
                        pages++;
                        freeBins += page->FreeCount;
                        capBins += page->Capacity;
                    }
                    if (pages != 0) {
                        std::printf("[UEB2][bin=%u] pages=%zu free=%zu cap=%zu\n",
                            static_cast<unsigned>(kSmallBinSizes[poolIndex]),
                            pages, freeBins, capBins);
                        SHINE_PROFILE_CPU_PLOT("UEB2.BinPages", static_cast<double>(pages));
                        SHINE_PROFILE_CPU_PLOT("UEB2.BinFreeBins", static_cast<double>(freeBins));
                    }
                    totalPages += pages;
                    totalFree += freeBins;
                    totalCapacity += capBins;
                }
                std::printf("[UEB2] totalPages=%zu totalFree=%zu totalCap=%zu\n",
                    totalPages, totalFree, totalCapacity);
                SHINE_PROFILE_CPU_PLOT("UEB2.TotalPages", static_cast<double>(totalPages));
                SHINE_PROFILE_CPU_PLOT("UEB2.TotalFreeBins", static_cast<double>(totalFree));
                SHINE_PROFILE_CPU_PLOT("UEB2.TotalCapacityBins", static_cast<double>(totalCapacity));
            }

        private:
            struct ThreadCache {
                std::array<FreeNode*, kPoolCount> Heads{};
                std::array<uint16_t, kPoolCount> Counts{};
            };

            static thread_local ThreadCache TLCache;
            std::array<uint8_t, (kMaxSmallPoolSize / 16) + 2> SizeToPoolIndex{};
            std::array<std::mutex, kPoolCount> PoolMutexes;
            std::array<PoolPage*, kPoolCount> PartialLists{};

            void BuildSizeMap() {
                for (size_t i = 0; i < SizeToPoolIndex.size(); ++i) {
                    size_t s = i * 16;
                    uint8_t idx = static_cast<uint8_t>(kPoolCount - 1);
                    for (size_t p = 0; p < kPoolCount; ++p) {
                        if (s <= kSmallBinSizes[p]) {
                            idx = static_cast<uint8_t>(p);
                            break;
                        }
                    }
                    SizeToPoolIndex[i] = idx;
                }
            }

            uint8_t GetPoolIndex(size_t size) const {
                size_t idx = (size + 15) >> 4;
                if (idx >= SizeToPoolIndex.size()) {
                    idx = SizeToPoolIndex.size() - 1;
                }
                return SizeToPoolIndex[idx];
            }

            bool IsSmallSize(size_t size) const {
                return size <= kMaxSmallPoolSize;
            }

            static ThreadCache& GetThreadCache() {
                return TLCache;
            }

            static void* OsAllocPage() {
#ifdef _WIN32
                return ::VirtualAlloc(nullptr, kPoolPageSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
                return std::aligned_alloc(kPoolPageSize, kPoolPageSize);
#endif
            }

            static void OsFreePage(void* p) {
#ifdef _WIN32
                ::VirtualFree(p, 0, MEM_RELEASE);
#else
                std::free(p);
#endif
            }

            PoolPage* CreatePoolPage(uint8_t poolIndex) {
                void* mem = OsAllocPage();
                if (!mem) {
                    return nullptr;
                }
                auto* page = reinterpret_cast<PoolPage*>(mem);
                page->Magic = kPageMagic;
                page->PoolIndex = poolIndex;
                page->BinSize = kSmallBinSizes[poolIndex];
                page->NextPartial = nullptr;
                page->PrevPartial = nullptr;
                page->InPartialList = 1;
                page->FreeList = nullptr;
                page->Capacity = static_cast<uint16_t>(kPoolUsableBytes / page->BinSize);
                page->FreeCount = page->Capacity;

                uint8_t* cursor = reinterpret_cast<uint8_t*>(page) + kPoolHeaderBytes;
                FreeNode* head = nullptr;
                for (uint16_t i = 0; i < page->Capacity; ++i) {
                    auto* node = reinterpret_cast<FreeNode*>(cursor + (i * page->BinSize));
                    node->Next = head;
                    head = node;
                }
                page->FreeList = head;
                return page;
            }

            static uintptr_t AlignDownToPage(void* p) {
                return reinterpret_cast<uintptr_t>(p) & ~(kPoolPageSize - 1);
            }

            static uint32_t MakeLargeCookie(void* base, size_t size) {
                uintptr_t b = reinterpret_cast<uintptr_t>(base);
                return kLargeMagic ^ static_cast<uint32_t>((b >> 4) ^ (size & 0xffffffffu));
            }

            void LinkPartial(uint8_t poolIndex, PoolPage* page) {
                if (page->InPartialList) {
                    return;
                }
                page->PrevPartial = nullptr;
                page->NextPartial = PartialLists[poolIndex];
                if (page->NextPartial) {
                    page->NextPartial->PrevPartial = page;
                }
                PartialLists[poolIndex] = page;
                page->InPartialList = 1;
            }

            void UnlinkPartial(uint8_t poolIndex, PoolPage* page) {
                if (!page->InPartialList) {
                    return;
                }
                if (page->PrevPartial) {
                    page->PrevPartial->NextPartial = page->NextPartial;
                } else {
                    PartialLists[poolIndex] = page->NextPartial;
                }
                if (page->NextPartial) {
                    page->NextPartial->PrevPartial = page->PrevPartial;
                }
                page->PrevPartial = nullptr;
                page->NextPartial = nullptr;
                page->InPartialList = 0;
            }

            void* AllocSmallCentral(uint8_t poolIndex) {
                std::lock_guard<std::mutex> lock(PoolMutexes[poolIndex]);
                while (true) {
                    PoolPage* page = PartialLists[poolIndex];
                    if (!page) {
                        page = CreatePoolPage(poolIndex);
                        if (!page) {
                            return nullptr;
                        }
                        PartialLists[poolIndex] = page;
                    }
                    FreeNode* node = page->FreeList;
                    if (!node) {
                        UnlinkPartial(poolIndex, page);
                        continue;
                    }
                    page->FreeList = node->Next;
                    page->FreeCount = static_cast<uint16_t>(page->FreeCount - 1);
                    if (page->FreeCount == 0) {
                        UnlinkPartial(poolIndex, page);
                    }
                    return node;
                }
            }

            void FreeSmallCentral(FreeNode* node, uint8_t poolIndex) {
                if (!node) {
                    return;
                }
                auto* page = reinterpret_cast<PoolPage*>(AlignDownToPage(node));
                if (page->Magic != kPageMagic || page->PoolIndex != poolIndex) {
                    return;
                }
                std::lock_guard<std::mutex> lock(PoolMutexes[poolIndex]);
                bool wasFull = (page->FreeCount == 0);
                node->Next = page->FreeList;
                page->FreeList = node;
                page->FreeCount = static_cast<uint16_t>(page->FreeCount + 1);
                if (page->FreeCount == page->Capacity) {
                    UnlinkPartial(poolIndex, page);
                    page->Magic = 0;
                    OsFreePage(page);
                    return;
                }
                if (wasFull) {
                    LinkPartial(poolIndex, page);
                }
            }

            void FlushThreadCacheToCentral(uint8_t poolIndex, uint16_t keep) {
                ThreadCache& cache = GetThreadCache();
                while (cache.Counts[poolIndex] > keep) {
                    FreeNode* node = cache.Heads[poolIndex];
                    if (!node) {
                        cache.Counts[poolIndex] = 0;
                        return;
                    }
                    cache.Heads[poolIndex] = node->Next;
                    cache.Counts[poolIndex] = static_cast<uint16_t>(cache.Counts[poolIndex] - 1);
                    FreeSmallCentral(node, poolIndex);
                }
            }

            void* AllocSmall(size_t size) {
                uint8_t poolIndex = GetPoolIndex(size);
                ThreadCache& cache = GetThreadCache();
                if (cache.Heads[poolIndex]) {
                    FreeNode* node = cache.Heads[poolIndex];
                    cache.Heads[poolIndex] = node->Next;
                    cache.Counts[poolIndex] = static_cast<uint16_t>(cache.Counts[poolIndex] - 1);
                    return node;
                }
                return AllocSmallCentral(poolIndex);
            }

            void FreeSmall(void* p) {
                auto* page = reinterpret_cast<PoolPage*>(AlignDownToPage(p));
                if (page->Magic != kPageMagic) {
                    return;
                }
                uint8_t poolIndex = static_cast<uint8_t>(page->PoolIndex);
                ThreadCache& cache = GetThreadCache();
                auto* node = reinterpret_cast<FreeNode*>(p);
                node->Next = cache.Heads[poolIndex];
                cache.Heads[poolIndex] = node;
                cache.Counts[poolIndex] = static_cast<uint16_t>(cache.Counts[poolIndex] + 1);
                if (cache.Counts[poolIndex] >= kTlsCacheLimit) {
                    FlushThreadCacheToCentral(poolIndex, static_cast<uint16_t>(kTlsCacheLimit / 2));
                }
            }

            static void* AllocLarge(size_t size) {
                if (size > (std::numeric_limits<size_t>::max)() - sizeof(LargeAllocHeader) - kMinAlign) {
                    return nullptr;
                }
                size_t total = size + sizeof(LargeAllocHeader) + kMinAlign;
#ifdef _WIN32
                void* base = _aligned_malloc(total, kMinAlign);
#else
                void* base = nullptr;
                if (posix_memalign(&base, kMinAlign, total) != 0) {
                    return nullptr;
                }
#endif
                if (!base) {
                    return nullptr;
                }
                uintptr_t p0 = reinterpret_cast<uintptr_t>(base) + sizeof(LargeAllocHeader);
                uintptr_t p1 = (p0 + (kMinAlign - 1)) & ~(kMinAlign - 1);
                auto* header = reinterpret_cast<LargeAllocHeader*>(p1 - sizeof(LargeAllocHeader));
                header->Magic = kLargeMagic;
                header->Cookie = MakeLargeCookie(base, size);
                header->BasePtr = base;
                header->Size = size;
                return reinterpret_cast<void*>(p1);
            }

            static bool TryFreeLarge(void* p) {
                if (!p) {
                    return false;
                }
                auto* header = reinterpret_cast<LargeAllocHeader*>(
                    reinterpret_cast<uintptr_t>(p) - sizeof(LargeAllocHeader));
                if (header->Magic != kLargeMagic) {
                    return false;
                }
                if (header->Cookie != MakeLargeCookie(header->BasePtr, header->Size)) {
                    return false;
                }
                uintptr_t base = reinterpret_cast<uintptr_t>(header->BasePtr);
                uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
                if (ptr < base + sizeof(LargeAllocHeader) || ptr > base + sizeof(LargeAllocHeader) + kMinAlign) {
                    return false;
                }
                header->Magic = 0;
#ifdef _WIN32
                _aligned_free(header->BasePtr);
#else
                std::free(header->BasePtr);
#endif
                return true;
            }

            void* AllocRaw(size_t size) {
                if (IsSmallSize(size)) {
                    return AllocSmall(size);
                }
                return AllocLarge(size);
            }

            void FreeRaw(void* p) {
                if (!p) {
                    return;
                }
                if (TryFreeLarge(p)) {
                    return;
                }
                auto* page = reinterpret_cast<PoolPage*>(AlignDownToPage(p));
                if (page->Magic == kPageMagic) {
                    FreeSmall(p);
                    return;
                }
            }

            void FlushAllThreadCaches() {
                for (uint8_t i = 0; i < static_cast<uint8_t>(kPoolCount); ++i) {
                    FlushThreadCacheToCentral(i, 0);
                }
            }
        };

        thread_local Allocator::ThreadCache Allocator::TLCache{};

        Allocator& GetAllocator() {
            static Allocator allocator;
            return allocator;
        }
    }

    void* Alloc(size_t size, size_t align) noexcept {
        return GetAllocator().Alloc(size, align);
    }

    void Free(void* p) noexcept {
        GetAllocator().Free(p);
    }

    void* Realloc(void* p, size_t size, size_t align) noexcept {
        return GetAllocator().Realloc(p, size, align);
    }

    bool GetAllocationSize(void* p, size_t& outSize) noexcept {
        return GetAllocator().GetAllocationSize(p, outSize);
    }

    void Trim() noexcept {
        GetAllocator().Trim();
    }

    bool ValidateHeap() noexcept {
        return GetAllocator().ValidateHeap();
    }

    void DumpAllocatorStats() noexcept {
        GetAllocator().DumpAllocatorStats();
    }
}
