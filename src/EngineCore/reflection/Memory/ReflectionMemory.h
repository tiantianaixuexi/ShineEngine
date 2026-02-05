#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include <mutex>


#include "../Core/ReflectionModernTypes.h"
#include "util/EnumFlags.h"
#include "memory/memory.ixx"

namespace shine::reflection {

    /**
     * @brief 反射系统内存分配标签
     * 用于区分不同类型的反射内存分配
     */
    enum class MemoryTag : uint32_t {
        None = 0,
        TypeInfo = 1 << 0,
        FieldInfo = 1 << 1,
        MethodInfo = 1 << 2,
        StringStorage = 1 << 3,
        HashTables = 1 << 4,
        TemporaryBuffers = 1 << 5,
        CacheData = 1 << 6
    };

    ENABLE_ENUM_FLAGS(MemoryTag)

    /**
     * @brief 内存分配统计信息
     */
    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t peakUsage = 0;
        size_t allocationCount = 0;
        size_t deallocationCount = 0;
        
        // 按标签统计
        size_t typeInfoBytes = 0;
        size_t fieldInfoBytes = 0;
        size_t stringBytes = 0;
        size_t cacheBytes = 0;
        
        double hitRate = 0.0; // 池化命中率
        double fragmentation = 0.0; // 内存碎片率
    };

    /**
     * @brief 现代化的反射内存管理器
     * 提供池化分配、标签追踪、统计监控等功能
     */
    class ReflectionMemoryManager {
    public:
        static ReflectionMemoryManager& GetInstance() {
            static ReflectionMemoryManager instance;
            return instance;
        }

        // 删除拷贝构造和赋值
        ReflectionMemoryManager(const ReflectionMemoryManager&) = delete;
        ReflectionMemoryManager& operator=(const ReflectionMemoryManager&) = delete;

        /**
         * @brief 分配带标签的内存
         */
        template<typename T = void>
        T* Allocate(size_t size, MemoryTag tag = MemoryTag::None, 
                   size_t alignment = alignof(std::max_align_t)) {
            return static_cast<T*>(AllocateImpl(size, tag, alignment));
        }

        /**
         * @brief 释放内存
         */
        void Deallocate(void* ptr, size_t size, MemoryTag tag = MemoryTag::None) {
            DeallocateImpl(ptr, size, tag);
        }

        /**
         * @brief 重置临时分配（每帧调用）
         */
        void ResetFrame() {
            std::lock_guard<std::mutex> lock(frameMutex_);
            
            // 重置临时缓冲区
            temporaryAllocator_.Reset();
            
            // 更新统计
            UpdateStatistics();
        }

        /**
         * @brief 获取内存统计信息
         */
        MemoryStats GetStatistics() const {
            std::lock_guard<std::mutex> lock(statsMutex_);
            return stats_;
        }

        /**
         * @brief 强制清理所有内存
         */
        void ForceCleanup() {
            temporaryAllocator_.Clear();
            ClearPools();
            
            std::lock_guard<std::mutex> lock(statsMutex_);
            stats_ = {};
        }

    private:
        ReflectionMemoryManager() = default;
        ~ReflectionMemoryManager() = default;

        void* AllocateImpl(size_t size, MemoryTag tag, size_t alignment);
        void DeallocateImpl(void* ptr, size_t size, MemoryTag tag);
        void UpdateStatistics();
        void ClearPools();

        // 临时分配器（帧生命周期）
        class TemporaryAllocator {
        public:
            struct Block {
                void* ptr;
                size_t size;
                MemoryTag tag;
            };

            void* Allocate(size_t size, MemoryTag tag, size_t alignment);
            void Reset(); // 重置但不释放内存
            void Clear(); // 完全清理

        private:
            std::vector<Block> blocks_;
            std::mutex mutex_;
        };

        TemporaryAllocator temporaryAllocator_;
        
        // 统计数据
        mutable std::mutex statsMutex_;
        mutable std::mutex frameMutex_;
        MemoryStats stats_;
        
        // 内存池相关
        static constexpr size_t SMALL_OBJECT_THRESHOLD = 512;
    };

    /**
     * @brief RAII内存分配包装器
     */
    template<typename T>
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

        // 禁止拷贝
        MemoryGuard(const MemoryGuard&) = delete;
        MemoryGuard& operator=(const MemoryGuard&) = delete;

        // 支持移动
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

        T* Get() const { return ptr_; }
        T* operator->() const { return ptr_; }
        T& operator*() const { return *ptr_; }
        explicit operator bool() const { return ptr_ != nullptr; }

    private:
        T* ptr_;
        MemoryTag tag_;
    };

    /**
     * @brief 字符串内存管理器
     * 专门处理反射系统中的字符串存储
     */
    class StringMemoryManager {
    public:
        static StringMemoryManager& GetInstance() {
            static StringMemoryManager instance;
            return instance;
        }

        // 字符串分配和管理
        const char* StoreString(std::string_view str);
        void ClearStrings();
        size_t GetStringCount() const;

    private:
        StringMemoryManager() = default;
        ~StringMemoryManager();

        class StringPool {
        public:
            const char* Allocate(std::string_view str);
            void Clear();
            size_t GetCount() const;

        private:
            struct StringBlock {
                std::unique_ptr<char[]> buffer;
                size_t capacity;
                size_t used;
            };

            std::vector<StringBlock> blocks_;
            static constexpr size_t INITIAL_BLOCK_SIZE = 4096;
            mutable std::mutex mutex_;
        };

        StringPool stringPool_;
        mutable std::mutex mutex_;
    };

} // namespace shine::reflection