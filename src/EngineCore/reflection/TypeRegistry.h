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
#include "string/shine_string.h"
#include <memory>
#include <unordered_map>

namespace shine::reflection {

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
                           shine::SString("Type ID ") + shine::SString(std::to_string(id)) + shine::SString(" (") + shine::SString(info.name) + shine::SString(") already registered"));
        }
        
        // Eagerly build lookup tables before sharing
        info.BuildLookup();
        
        auto typeInfo = std::make_shared<TypeInfo>(std::move(info));
        const auto ownerHandle = ReflectionOwnerHandle::FromType(typeInfo.get());
        for (auto& field : typeInfo->fields) {
            field.owner = ownerHandle;
        }
        for (auto& method : typeInfo->methods) {
            method.owner = ownerHandle;
        }
        registry_[id] = typeInfo;
        nameRegistry_[shine::SString(typeInfo->name)] = typeInfo;
        return {};
    }
    
    // ---- Safe lookup with Result<T> ----
    
    Result<const TypeInfo*> Find(TypeId id) const {
        auto it = registry_.find(id);
        if (it != registry_.end()) {
            return it->second.get();
        }
        return MakeError(ErrorCode::TypeNotFound, 
                        shine::SString("Type ID ") + shine::SString(std::to_string(id)) + shine::SString(" not found"));
    }
    
    // ---- Fast lookup for performance-critical paths (hot path optimization) ----
    
    [[gnu::always_inline]]
    const TypeInfo* FindFast(TypeId id) const noexcept {
        auto it = registry_.find(id);
        return (it != registry_.end()) ? it->second.get() : nullptr;
    }

    [[gnu::always_inline]]
    const TypeInfo* FindByNameFast(shine::STextView typeName) const noexcept {
        // Try direct hash lookup first as it's the most common case
        if (const auto* info = FindFast(Hash(typeName))) {
            if (info->name == typeName) return info;
        }

        // Fallback to name registry
        auto it = nameRegistry_.find(shine::SString(typeName));
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
    std::unordered_map<shine::SString, std::shared_ptr<TypeInfo>> nameRegistry_;
};

} // namespace shine::reflection
