#include "ReflectionMemory.h"
#include <algorithm>
#include <cstring>

namespace shine::reflection {

    void* ReflectionMemoryManager::AllocateImpl(size_t size, MemoryTag tag, size_t alignment) {
        // 更新统计数据
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.allocationCount++;
            stats_.totalAllocated += size;
            stats_.peakUsage = std::max(stats_.peakUsage, stats_.totalAllocated);

            // 按标签分类统计
            if (HasFlag(tag, MemoryTag::TypeInfo)) {
                stats_.typeInfoBytes += size;
            }
            if (HasFlag(tag, MemoryTag::FieldInfo)) {
                stats_.fieldInfoBytes += size;
            }
            if (HasFlag(tag, MemoryTag::StringStorage)) {
                stats_.stringBytes += size;
            }
            if (HasFlag(tag, MemoryTag::CacheData)) {
                stats_.cacheBytes += size;
            }
        }

        // 对于临时分配，使用帧分配器
        if (HasFlag(tag, MemoryTag::TemporaryBuffers)) {
            return temporaryAllocator_.Allocate(size, tag, alignment);
        }

        // 小对象使用shine内存系统
        if (size <= SMALL_OBJECT_THRESHOLD) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
            return shine::co::Memory::Alloc(size, alignment);
        }

        // 大对象直接分配
        return std::aligned_alloc(alignment, size);
    }

    void ReflectionMemoryManager::DeallocateImpl(void* ptr, size_t size, MemoryTag tag) {
        if (!ptr) return;

        // 更新统计数据
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_.deallocationCount++;
            stats_.totalAllocated = (stats_.totalAllocated > size) ? 
                                   stats_.totalAllocated - size : 0;

            // 按标签分类统计
            if (HasFlag(tag, MemoryTag::TypeInfo) && stats_.typeInfoBytes >= size) {
                stats_.typeInfoBytes -= size;
            }
            if (HasFlag(tag, MemoryTag::FieldInfo) && stats_.fieldInfoBytes >= size) {
                stats_.fieldInfoBytes -= size;
            }
            if (HasFlag(tag, MemoryTag::StringStorage) && stats_.stringBytes >= size) {
                stats_.stringBytes -= size;
            }
            if (HasFlag(tag, MemoryTag::CacheData) && stats_.cacheBytes >= size) {
                stats_.cacheBytes -= size;
            }
        }

        // 临时分配不需要显式释放
        if (HasFlag(tag, MemoryTag::TemporaryBuffers)) {
            return;
        }

        // 小对象使用shine内存系统释放
        if (size <= SMALL_OBJECT_THRESHOLD) {
            shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
            shine::co::Memory::Free(ptr);
            return;
        }

        // 大对象直接释放
        std::free(ptr);
    }

    void ReflectionMemoryManager::UpdateStatistics() {
        std::lock_guard<std::mutex> lock(statsMutex_);
        
        // 计算命中率
        if (stats_.allocationCount > 0) {
            stats_.hitRate = (static_cast<double>(stats_.allocationCount - stats_.deallocationCount) / 
                             static_cast<double>(stats_.allocationCount)) * 100.0;
        }

        // 简单的碎片率估算
        stats_.fragmentation = (stats_.peakUsage > 0) ? 
                              (1.0 - static_cast<double>(stats_.totalAllocated) / 
                               static_cast<double>(stats_.peakUsage)) * 100.0 : 0.0;
    }

    void ReflectionMemoryManager::ClearPools() {
        // 清理shine内存池
        shine::co::MemoryScope scope(shine::co::MemoryTag::Reflection);
        // 注意：这里可能需要shine内存系统的特定清理接口
    }

    // TemporaryAllocator 实现
    void* ReflectionMemoryManager::TemporaryAllocator::Allocate(size_t size, MemoryTag tag, size_t alignment) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // 简单实现：直接分配内存
        void* ptr = std::aligned_alloc(alignment, size);
        if (ptr) {
            blocks_.push_back({ptr, size, tag});
        }
        return ptr;
    }

    void ReflectionMemoryManager::TemporaryAllocator::Reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        // 重置但保留内存块以供下次使用
        blocks_.clear();
    }

    void ReflectionMemoryManager::TemporaryAllocator::Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& block : blocks_) {
            std::free(block.ptr);
        }
        blocks_.clear();
    }

    // StringMemoryManager 实现
    StringMemoryManager::~StringMemoryManager() {
        ClearStrings();
    }

    const char* StringMemoryManager::StoreString(std::string_view str) {
        std::lock_guard<std::mutex> lock(mutex_);
        return stringPool_.Allocate(str);
    }

    void StringMemoryManager::ClearStrings() {
        std::lock_guard<std::mutex> lock(mutex_);
        stringPool_.Clear();
    }

    size_t StringMemoryManager::GetStringCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stringPool_.GetCount();
    }

    // StringPool 实现
    const char* StringMemoryManager::StringPool::Allocate(std::string_view str) {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t required_size = str.length() + 1; // +1 for null terminator

        // 查找合适的块
        for (auto& block : blocks_) {
            if (block.capacity - block.used >= required_size) {
                char* result = block.buffer.get() + block.used;
                std::memcpy(result, str.data(), str.length());
                result[str.length()] = '\0';
                block.used += required_size;
                return result;
            }
        }

        // 创建新块
        size_t new_block_size = std::max(INITIAL_BLOCK_SIZE, required_size);
        StringBlock new_block;
        new_block.buffer = std::make_unique<char[]>(new_block_size);
        new_block.capacity = new_block_size;
        new_block.used = 0;

        char* result = new_block.buffer.get();
        std::memcpy(result, str.data(), str.length());
        result[str.length()] = '\0';
        new_block.used = required_size;

        blocks_.push_back(std::move(new_block));
        return result;
    }

    void StringMemoryManager::StringPool::Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        blocks_.clear();
    }

    size_t StringMemoryManager::StringPool::GetCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t count = 0;
        for (const auto& block : blocks_) {
            count += block.used; // 这里简单估算，实际应该统计字符串数量
        }
        return count;
    }

} // namespace shine::reflection