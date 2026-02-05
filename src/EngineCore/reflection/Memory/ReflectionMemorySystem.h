#pragma once

/**
 * @file ReflectionMemory.h
 * @brief 反射系统内存管理统一入口
 * 
 * 现代化的内存管理体系，提供：
 * - 池化分配器（针对反射对象优化）
 * - 标签化内存追踪
 * - 统计监控
 * - RAII内存管理
 * - 字符串专用管理
 */

#include "ReflectionMemory.h"
#include "ReflectionObjectPool.h"

namespace shine::reflection {

    /**
     * @brief 内存管理统一接口
     * 整合所有内存管理功能
     */
    class MemorySystem {
    public:
        static MemorySystem& GetInstance() {
            static MemorySystem instance;
            return instance;
        }

        // 内存分配接口
        template<typename T = void>
        static T* Allocate(size_t size, MemoryTag tag = MemoryTag::None, 
                          size_t alignment = alignof(std::max_align_t)) {
            return ReflectionMemoryManager::GetInstance().template Allocate<T>(size, tag, alignment);
        }

        template<typename T, typename... Args>
        static T* New(MemoryTag tag = MemoryTag::None, Args&&... args) {
            T* ptr = Allocate<T>(sizeof(T), tag, alignof(T));
            if (ptr) {
                new (ptr) T(std::forward<Args>(args)...);
            }
            return ptr;
        }

        template<typename T>
        static void Delete(T* ptr, MemoryTag tag = MemoryTag::None) {
            if (ptr) {
                ptr->~T();
                Deallocate(ptr, sizeof(T), tag);
            }
        }

        static void Deallocate(void* ptr, size_t size, MemoryTag tag = MemoryTag::None) {
            ReflectionMemoryManager::GetInstance().Deallocate(ptr, size, tag);
        }

        // 池化分配
        template<typename T>
        static PooledObject<T> MakePooled() {
            return PooledObject<T>();
        }

        template<typename T, typename... Args>
        static PooledObject<T> MakePooled(Args&&... args) {
            return PooledObject<T>(std::forward<Args>(args)...);
        }

        // 字符串管理
        static const char* StoreString(std::string_view str) {
            return StringMemoryManager::GetInstance().StoreString(str);
        }

        // 内存统计
        static MemoryStats GetMemoryStats() {
            return ReflectionMemoryManager::GetInstance().GetStatistics();
        }

        static typename MultiSizeObjectPool::MultiPoolStats GetPoolStats() {
            static MultiSizeObjectPool pool;
            return pool.GetStatistics();
        }

        // 系统管理
        static void ResetFrame() {
            ReflectionMemoryManager::GetInstance().ResetFrame();
        }

        static void ForceCleanup() {
            ReflectionMemoryManager::GetInstance().ForceCleanup();
            StringMemoryManager::GetInstance().ClearStrings();
        }

        // RAII包装器
        template<typename T>
        using Guard = MemoryGuard<T>;

        template<typename T>
        static Guard<T> MakeGuard(MemoryTag tag = MemoryTag::None) {
            return Guard<T>(tag);
        }

    private:
        MemorySystem() = default;
        ~MemorySystem() = default;

        // 禁止拷贝和移动
        MemorySystem(const MemorySystem&) = delete;
        MemorySystem& operator=(const MemorySystem&) = delete;
        MemorySystem(MemorySystem&&) = delete;
        MemorySystem& operator=(MemorySystem&&) = delete;
    };

    // 便利的命名空间别名
    namespace mem = MemorySystem;

} // namespace shine::reflection