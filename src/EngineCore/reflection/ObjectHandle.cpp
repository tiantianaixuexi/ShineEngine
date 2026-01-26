#include "ObjectHandle.h"

namespace shine::reflection {

    HandleRegistry& HandleRegistry::Get() {
        static HandleRegistry instance;
        return instance;
    }

    HandleRegistry::HandleRegistry() {
        // Dummy entry 0
        entries.push_back({ nullptr, 0, 0 });
    }

    ObjectHandle HandleRegistry::Register(void* ptr) {
        if (!ptr) return { 0, 0 };

        std::lock_guard<std::mutex> lock(mutex);

        // Check if already registered
        auto it = ptrToIndex.find(ptr);
        if (it != ptrToIndex.end()) {
            return { it->second, entries[it->second].generation };
        }

        uint32_t index;
        if (freeHead != 0) {
            index = freeHead;
            freeHead = entries[index].nextFree;
        } else {
            index = static_cast<uint32_t>(entries.size());
            entries.push_back({ nullptr, 1, 0 }); // Generation starts at 1
        }

        entries[index].ptr = ptr;
        // Generation is already set (incremented on unregister)
        
        ptrToIndex[ptr] = index;

        return { index, entries[index].generation };
    }

    void HandleRegistry::Unregister(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex);

        auto it = ptrToIndex.find(ptr);
        if (it == ptrToIndex.end()) return; // Not registered or already unregistered

        uint32_t index = it->second;
        ptrToIndex.erase(it);

        entries[index].ptr = nullptr;
        entries[index].generation++; // Increment generation to invalidate old handles
        if (entries[index].generation == 0) entries[index].generation = 1; // Wrap around protection

        // Add to free list
        entries[index].nextFree = freeHead;
        freeHead = index;
    }

    void* HandleRegistry::Get(ObjectHandle handle) {
        if (handle.index == 0) return nullptr;

        std::lock_guard<std::mutex> lock(mutex);

        if (handle.index >= entries.size()) return nullptr;
        const auto& entry = entries[handle.index];

        if (entry.generation != handle.generation) return nullptr; // Expired

        return entry.ptr;
    }

}
