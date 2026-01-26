#pragma once
#include <cstdint>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace shine::reflection {

    struct ObjectHandle {
        uint32_t index = 0;
        uint32_t generation = 0;

        bool IsValid() const { return index != 0; }
        bool operator==(const ObjectHandle& other) const { return index == other.index && generation == other.generation; }
        bool operator!=(const ObjectHandle& other) const { return !(*this == other); }
    };

    class HandleRegistry {
    public:
        static HandleRegistry& Get();

        // Register a pointer, returns a handle. 
        // If ptr is already registered, returns existing handle.
        ObjectHandle Register(void* ptr);

        // Called when object is destroyed. Invalidates the handle.
        void Unregister(void* ptr);

        // Resolve handle to pointer. Returns nullptr if invalid/expired.
        void* Get(ObjectHandle handle);

    private:
        struct Entry {
            void* ptr = nullptr;
            uint32_t generation = 0;
            uint32_t nextFree = 0; // For free list
        };

        std::vector<Entry> entries;
        uint32_t freeHead = 0;
        std::mutex mutex;
        std::unordered_map<void*, uint32_t> ptrToIndex;

        HandleRegistry();
    };

}
