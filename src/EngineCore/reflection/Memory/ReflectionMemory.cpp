#include "ReflectionMemory.h"
#include <algorithm>
#include <cstring>

// =============================================================================
// ReflectionMemoryManager — simplified implementation using engine memory
// =============================================================================
//
// All allocations now route directly through shine::co::Memory with
// MemoryTag::Reflection, eliminating duplicate statistics tracking.
//

namespace shine::reflection {

    // ---- Allocate ----
    // (Implementation moved to header for inlining)

    // ---- Deallocate ----
    // (Implementation moved to header for inlining)

    // ---- UpdateStatistics ----
    // (Now handled by engine memory system)

    // =========================================================================
    // StringMemoryManager — implementation (unchanged)
    // =========================================================================

    StringMemoryManager::~StringMemoryManager() {
        ClearStrings();
    }

    const char* StringMemoryManager::StoreString(shine::STextView str) {
        // ---- Fast-path: deduplicate via hash set ----
        //
        // If the exact string content was already interned, return
        // the existing pointer.  This avoids allocating duplicate
        // copies of common names like "float", "int", field names, etc.

        if (auto it = internSet_.find(str); it != internSet_.end()) {
            return it->data();
        }

        // ---- Slow-path: store in arena + insert into set ----
        const char* stored = arena_.Store(str);
        if (stored) {
            // Create a string_view pointing into the arena memory
            internSet_.emplace(stored, str.size());
            stringCount_.fetch_add(1, std::memory_order_relaxed);
        }
        return stored;
    }

    void StringMemoryManager::ClearStrings() {
        internSet_.clear();
        arena_.Clear();
        stringCount_.store(0, std::memory_order_relaxed);
    }

    size_t StringMemoryManager::GetStringCount() const noexcept {
        return stringCount_.load(std::memory_order_relaxed);
    }

    size_t StringMemoryManager::GetTotalBytes() const noexcept {
        return arena_.GetTotalStored();
    }

    // =========================================================================
    // StringArena — implementation
    // =========================================================================

    const char* StringMemoryManager::StringArena::Store(shine::STextView str) {
        const size_t required = str.size() + 1;  // +1 for null terminator

        // Try to fit in the last block
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

        // Allocate a new block (via operator new → engine Memory)
        size_t blockCap = std::max(BLOCK_SIZE, required);
        Block block;
        block.buffer   = std::make_unique<char[]>(blockCap);
        block.capacity = blockCap;
        block.used     = 0;

        char* dest = block.buffer.get();
        std::memcpy(dest, str.data(), str.size());
        dest[str.size()] = '\0';
        block.used = required;
        totalStored_ += required;

        blocks_.push_back(std::move(block));
        return dest;
    }

    void StringMemoryManager::StringArena::Clear() {
        blocks_.clear();
        totalStored_ = 0;
    }

} // namespace shine::reflection
