#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <atomic>

#include "ReflectionMemory.h"

namespace shine::reflection {

    /**
     * @brief 固定大小对象池
     * 针对特定大小的对象进行高效池化管理
     */
    template<size_t ObjectSize, size_t Alignment = alignof(std::max_align_t)>
    class FixedSizeObjectPool {
    public:
        static constexpr size_t OBJECT_SIZE = ObjectSize;
        static constexpr size_t ALIGNMENT = Alignment;

        FixedSizeObjectPool() : freeList_(nullptr), allocatedBlocks_(0) {}
        ~FixedSizeObjectPool() { Clear(); }

        /**
         * @brief 从池中分配对象
         */
        void* Allocate() {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (freeList_) {
                Node* node = freeList_;
                freeList_ = freeList_->next;
                allocationCount_++;
                return node;
            }

            // 分配新区块
            Node* newNode = AllocateNewNode();
            if (newNode) {
                allocationCount_++;
                return newNode;
            }

            return nullptr;
        }

        /**
         * @brief 将对象归还到池中
         */
        void Deallocate(void* ptr) {
            if (!ptr) return;

            std::lock_guard<std::mutex> lock(mutex_);
            
            Node* node = static_cast<Node*>(ptr);
            node->next = freeList_;
            freeList_ = node;
            deallocationCount_++;
        }

        /**
         * @brief 重置池状态（保留内存）
         */
        void Reset() {
            std::lock_guard<std::mutex> lock(mutex_);
            // 重置统计但保留内存
            allocationCount_ = 0;
            deallocationCount_ = 0;
        }

        /**
         * @brief 完全清理池内存
         */
        void Clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            
            while (freeList_) {
                Node* node = freeList_;
                freeList_ = freeList_->next;
                
                // 使用反射内存管理器释放
                ReflectionMemoryManager::GetInstance().Deallocate(
                    node, sizeof(Node), MemoryTag::TypeInfo);
            }
            
            allocatedBlocks_ = 0;
            allocationCount_ = 0;
            deallocationCount_ = 0;
        }

        /**
         * @brief 获取池统计信息
         */
        struct PoolStats {
            size_t allocatedBlocks = 0;
            size_t freeObjects = 0;
            size_t totalAllocations = 0;
            size_t totalDeallocations = 0;
            double utilization = 0.0; // 使用率
        };

        PoolStats GetStatistics() const {
            std::lock_guard<std::mutex> lock(mutex_);
            
            PoolStats stats;
            stats.allocatedBlocks = allocatedBlocks_;
            stats.totalAllocations = allocationCount_;
            stats.totalDeallocations = deallocationCount_;

            // 计算空闲对象数
            Node* current = freeList_;
            while (current) {
                stats.freeObjects++;
                current = current->next;
            }

            // 计算使用率
            if (allocatedBlocks_ > 0) {
                size_t totalObjects = allocatedBlocks_ * (BLOCK_SIZE / sizeof(Node));
                size_t usedObjects = totalObjects - stats.freeObjects;
                stats.utilization = (static_cast<double>(usedObjects) / 
                                   static_cast<double>(totalObjects)) * 100.0;
            }

            return stats;
        }

    private:
        struct Node {
            alignas(ALIGNMENT) char data[OBJECT_SIZE];
            Node* next;
        };

        static constexpr size_t BLOCK_SIZE = 4096; // 4KB块大小

        Node* AllocateNewNode() {
            // 计算每个块能容纳多少个对象
            constexpr size_t objectsPerBlock = BLOCK_SIZE / sizeof(Node);
            constexpr size_t allocSize = objectsPerBlock * sizeof(Node);

            // 使用反射内存管理器分配
            Node* block = ReflectionMemoryManager::GetInstance().template Allocate<Node>(
                allocSize, MemoryTag::TypeInfo, alignof(Node));

            if (!block) return nullptr;

            // 初始化链表
            for (size_t i = 0; i < objectsPerBlock - 1; ++i) {
                block[i].next = &block[i + 1];
            }
            block[objectsPerBlock - 1].next = freeList_;
            freeList_ = block;

            allocatedBlocks_++;
            return block;
        }

        mutable std::mutex mutex_;
        Node* freeList_;
        std::atomic<size_t> allocatedBlocks_{0};
        std::atomic<size_t> allocationCount_{0};
        std::atomic<size_t> deallocationCount_{0};
    };

    /**
     * @brief 多尺寸对象池管理器
     * 管理多个固定大小的对象池
     */
    class MultiSizeObjectPool {
    public:
        // 常用的反射对象大小
        static constexpr size_t COMMON_SIZES[] = { 
            32,   // 小型字段信息
            64,   // 中型字段信息
            128,  // 大型字段信息
            256,  // 方法信息
            512   // 类型信息
        };

        MultiSizeObjectPool() {
            // 初始化各个尺寸的池
            for (size_t i = 0; i < NUM_SIZES; ++i) {
                pools_[i] = CreatePoolForSize(COMMON_SIZES[i]);
            }
        }

        ~MultiSizeObjectPool() {
            Clear();
        }

        /**
         * @brief 分配合适大小的对象
         */
        void* Allocate(size_t size) {
            for (size_t i = 0; i < NUM_SIZES; ++i) {
                if (size <= COMMON_SIZES[i]) {
                    return pools_[i]->Allocate();
                }
            }
            
            // 超出预定义尺寸，使用普通分配
            return ReflectionMemoryManager::GetInstance().Allocate(
                size, MemoryTag::TypeInfo);
        }

        /**
         * @brief 释放对象到相应池中
         */
        void Deallocate(void* ptr, size_t size) {
            if (!ptr) return;

            for (size_t i = 0; i < NUM_SIZES; ++i) {
                if (size <= COMMON_SIZES[i]) {
                    pools_[i]->Deallocate(ptr);
                    return;
                }
            }

            // 超出预定义尺寸，使用普通释放
            ReflectionMemoryManager::GetInstance().Deallocate(
                ptr, size, MemoryTag::TypeInfo);
        }

        /**
         * @brief 重置所有池
         */
        void Reset() {
            for (auto& pool : pools_) {
                if (pool) {
                    pool->Reset();
                }
            }
        }

        /**
         * @brief 清理所有池内存
         */
        void Clear() {
            for (auto& pool : pools_) {
                if (pool) {
                    pool->Clear();
                    delete pool;
                    pool = nullptr;
                }
            }
        }

        /**
         * @brief 获取综合统计信息
         */
        struct MultiPoolStats {
            size_t totalAllocatedBlocks = 0;
            size_t totalFreeObjects = 0;
            size_t totalAllocations = 0;
            size_t totalDeallocations = 0;
            double averageUtilization = 0.0;
            double poolHitRate = 0.0; // 池化命中率
        };

        MultiPoolStats GetStatistics() const {
            MultiPoolStats stats;

            for (size_t i = 0; i < NUM_SIZES; ++i) {
                if (pools_[i]) {
                    auto poolStats = pools_[i]->GetStatistics();
                    stats.totalAllocatedBlocks += poolStats.allocatedBlocks;
                    stats.totalFreeObjects += poolStats.freeObjects;
                    stats.totalAllocations += poolStats.totalAllocations;
                    stats.totalDeallocations += poolStats.totalDeallocations;
                    stats.averageUtilization += poolStats.utilization;
                }
            }

            if (NUM_SIZES > 0) {
                stats.averageUtilization /= static_cast<double>(NUM_SIZES);
            }

            // 计算池化命中率
            size_t totalOperations = stats.totalAllocations + stats.totalDeallocations;
            if (totalOperations > 0) {
                stats.poolHitRate = (static_cast<double>(stats.totalAllocations) / 
                                   static_cast<double>(totalOperations)) * 100.0;
            }

            return stats;
        }

    private:
        static constexpr size_t NUM_SIZES = sizeof(COMMON_SIZES) / sizeof(COMMON_SIZES[0]);

        // 抽象基类
        class ObjectPoolBase {
        public:
            virtual ~ObjectPoolBase() = default;
            virtual void* Allocate() = 0;
            virtual void Deallocate(void* ptr) = 0;
            virtual void Reset() = 0;
            virtual void Clear() = 0;
            virtual typename FixedSizeObjectPool<32>::PoolStats GetStatistics() const = 0;
        };

        // 模板特化工厂
        template<size_t Size>
        ObjectPoolBase* CreatePool() {
            return new FixedSizeObjectPool<Size>();
        }

        ObjectPoolBase* CreatePoolForSize(size_t size) {
            // 简化实现：只处理几个常见尺寸
            switch (size) {
                case 32: return CreatePool<32>();
                case 64: return CreatePool<64>();
                case 128: return CreatePool<128>();
                case 256: return CreatePool<256>();
                case 512: return CreatePool<512>();
                default: return nullptr;
            }
        }

        std::unique_ptr<ObjectPoolBase> pools_[NUM_SIZES];
    };

    /**
     * @brief RAII风格的对象池分配器
     */
    template<typename T>
    class PooledObject {
    public:
        explicit PooledObject()
            : ptr_(static_cast<T*>(GetObjectPool<T>().Allocate())) {
            if (ptr_) {
                new (ptr_) T(); // 构造对象
            }
        }

        template<typename... Args>
        explicit PooledObject(Args&&... args)
            : ptr_(static_cast<T*>(GetObjectPool<T>().Allocate())) {
            if (ptr_) {
                new (ptr_) T(std::forward<Args>(args)...); // 构造对象
            }
        }

        ~PooledObject() {
            if (ptr_) {
                ptr_->~T(); // 析构对象
                GetObjectPool<T>().Deallocate(ptr_);
            }
        }

        // 禁止拷贝
        PooledObject(const PooledObject&) = delete;
        PooledObject& operator=(const PooledObject&) = delete;

        // 支持移动
        PooledObject(PooledObject&& other) noexcept : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        PooledObject& operator=(PooledObject&& other) noexcept {
            if (this != &other) {
                if (ptr_) {
                    ptr_->~T();
                    GetObjectPool<T>().Deallocate(ptr_);
                }
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        T* Get() const { return ptr_; }
        T* operator->() const { return ptr_; }
        T& operator*() const { return *ptr_; }
        explicit operator bool() const { return ptr_ != nullptr; }

    private:
        template<typename U>
        static MultiSizeObjectPool& GetObjectPool() {
            static MultiSizeObjectPool pool;
            return pool;
        }

        T* ptr_;
    };

} // namespace shine::reflection