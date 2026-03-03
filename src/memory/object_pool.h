#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <new>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

#include "memory/memory.ixx"

namespace shine::co {

    template <typename T, MemoryTag Tag = MemoryTag::Core, size_t BlockCount = 256>
    class ObjectPool {
    public:
        ObjectPool() = default;
        ~ObjectPool() { clear(); }

        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;
        ObjectPool(ObjectPool&&) = delete;
        ObjectPool& operator=(ObjectPool&&) = delete;

        template <typename... TArgs>
        T* create(TArgs&&... args) {
            Node* node = nullptr;
            {
                std::scoped_lock lock(mutex_);
                if (!freeList_) {
                    allocateBlock();
                }
                node = freeList_;
                freeList_ = freeList_->next;
            }

            T* obj = nullptr;
            try {
                obj = std::construct_at(reinterpret_cast<T*>(node), std::forward<TArgs>(args)...);
            } catch (...) {
                std::scoped_lock lock(mutex_);
                node->next = freeList_;
                freeList_ = node;
                throw;
            }

            {
                std::scoped_lock lock(mutex_);
                liveObjects_.insert(obj);
                liveCount_.fetch_add(1, std::memory_order_relaxed);
            }

            return obj;
        }

        void destroy(T* obj) {
            if (!obj) {
                return;
            }

            bool owned = false;
            {
                std::scoped_lock lock(mutex_);
                auto it = liveObjects_.find(obj);
                if (it == liveObjects_.end()) {
                    return;
                }
                liveObjects_.erase(it);
                liveCount_.fetch_sub(1, std::memory_order_relaxed);
                owned = true;
            }

            if (!owned) {
                return;
            }

            std::destroy_at(obj);

            {
                std::scoped_lock lock(mutex_);
                auto* node = reinterpret_cast<Node*>(obj);
                node->next = freeList_;
                freeList_ = node;
            }
        }

        [[nodiscard]] size_t liveCount() const noexcept {
            return liveCount_.load(std::memory_order_relaxed);
        }

        static ObjectPool& Get() {
            static ObjectPool pool;
            return pool;
        }

        template <typename... TArgs>
        static T* Create(TArgs&&... args) {
            return Get().create(std::forward<TArgs>(args)...);
        }

        static void Destroy(T* obj) {
            Get().destroy(obj);
        }

    private:
        struct Node {
            Node* next = nullptr;
        };

        static constexpr size_t AlignUp(size_t size, size_t align) {
            return (size + align - 1) & ~(align - 1);
        }

        void allocateBlock() {
            MemoryScope scope(Tag);
            const size_t slotSize = AlignUp((std::max)(sizeof(T), sizeof(Node)), alignof(T));
            const size_t blockSize = slotSize * BlockCount;
            void* memory = Memory::Alloc(blockSize, alignof(T));
            blocks_.push_back(memory);

            auto* bytes = static_cast<std::byte*>(memory);
            for (size_t i = 0; i < BlockCount; ++i) {
                auto* node = reinterpret_cast<Node*>(bytes + i * slotSize);
                node->next = freeList_;
                freeList_ = node;
            }
        }

        void clear() {
            std::unordered_set<T*> live;
            std::vector<void*> blocks;
            {
                std::scoped_lock lock(mutex_);
                live.swap(liveObjects_);
                blocks.swap(blocks_);
                freeList_ = nullptr;
                liveCount_.store(0, std::memory_order_relaxed);
            }
            for (auto* obj : live) {
                std::destroy_at(obj);
            }
            for (void* block : blocks) {
                Memory::Free(block);
            }
        }

        Node* freeList_ = nullptr;
        std::vector<void*> blocks_;
        std::unordered_set<T*> liveObjects_;
        std::atomic_size_t liveCount_{0};
        mutable std::mutex mutex_;
    };

    template <typename T, typename Enable = void>
    struct ObjectPoolConfig {
        static constexpr MemoryTag Tag = MemoryTag::Core;
        static constexpr size_t BlockCount = 256;
    };

    template <typename T>
    using ObjectPoolFor = ObjectPool<T, ObjectPoolConfig<T>::Tag, ObjectPoolConfig<T>::BlockCount>;

    template <typename T, typename... TArgs>
    T* PooledCreate(TArgs&&... args) {
        return ObjectPoolFor<T>::Create(std::forward<TArgs>(args)...);
    }

    template <typename T>
    void PooledDestroy(T* obj) {
        ObjectPoolFor<T>::Destroy(obj);
    }

    template <typename T>
    [[nodiscard]] size_t PooledLiveCount() {
        return ObjectPoolFor<T>::Get().liveCount();
    }

#define SHINE_CONFIG_OBJECT_POOL(Type, TagValue, BlockCountValue)                              \
    namespace shine::co {                                                                       \
        template <>                                                                             \
        struct ObjectPoolConfig<Type, void> {                                                   \
            static constexpr MemoryTag Tag = TagValue;                                          \
            static constexpr size_t BlockCount = static_cast<size_t>(BlockCountValue);         \
        };                                                                                      \
    }

} // namespace shine::co
