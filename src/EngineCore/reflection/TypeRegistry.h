#pragma once

// =============================================================================
// TypeRegistry.h — Core Type Registration System
// =============================================================================
//
// Contains the TypeRegistry class for runtime type registration and lookup.
// This is separated from Reflection.h to improve modularity.
//
// C++23 / MSVC
// =============================================================================

#include "ReflectionCore.h"
#include "ReflectionError.h"
#include <memory>
#include <unordered_map>
#include <string>

namespace shine::reflection {

// Transparent hash for string_view to string comparisons without allocation
struct StringHash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(std::string_view txt) const noexcept {
        return std::hash<std::string_view>{}(txt);
    }
};

// =============================================================================
// Runtime Type Registry
// =============================================================================

class TypeRegistry {
public:
    static TypeRegistry& Get() {
        static TypeRegistry instance;
        return instance;
    }

    // ---- Registration with error handling ----
    
    Result<void> Register(TypeInfo info) {
        auto id = info.id;
        
        // Check for duplicate registration
        if (registry_.contains(id)) {
            return MakeError(ErrorCode::TypeAlreadyRegistered, 
                           std::string("Type ID ") + std::to_string(id) + " (" + std::string(info.name) + ") already registered");
        }
        
        // Eagerly build lookup tables before sharing
        info.BuildLookup();
        
        auto typeInfo = std::make_shared<TypeInfo>(std::move(info));
        registry_[id] = typeInfo;
        nameRegistry_[std::string(typeInfo->name)] = typeInfo;
        return {};
    }
    
    // ---- Safe lookup with Result<T> ----
    
    Result<const TypeInfo*> Find(TypeId id) const {
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            return it->second.get();
        }
        return MakeError(ErrorCode::TypeNotFound, 
                        std::string("Type ID ") + std::to_string(id) + " not found");
    }
    
    // ---- Fast lookup for performance-critical paths (hot path optimization) ----
    
    [[gnu::always_inline]]
    const TypeInfo* FindFast(TypeId id) const noexcept {
        auto it = registry_.find(id);
        return (it != registry_.end()) ? it->second.get() : nullptr;
    }

    [[gnu::always_inline]]
    const TypeInfo* FindByNameFast(std::string_view typeName) const noexcept {
        // Try direct hash lookup first as it's the most common case
        if (const auto* info = FindFast(Hash(typeName))) {
            if (info->name == typeName) return info;
        }

        // Fallback to name registry
        auto it = nameRegistry_.find(typeName);
        return (it != nameRegistry_.end()) ? it->second.get() : nullptr;
    }
    
    template <typename T>
    Result<const TypeInfo*> Find() const { 
        return Find(GetTypeId<T>()); 
    }
    

    std::size_t GetRegisteredTypeCount() const { return registry_.size(); }

private:
    TypeRegistry() = default;
    std::unordered_map<TypeId, std::shared_ptr<TypeInfo>> registry_;
    std::unordered_map<std::string, std::shared_ptr<TypeInfo>, StringHash, std::equal_to<>> nameRegistry_;
};

} // namespace shine::reflection
