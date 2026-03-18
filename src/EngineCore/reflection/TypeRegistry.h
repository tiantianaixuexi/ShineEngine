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
        const auto typeName = info.GetNameView();
        
        // Check for duplicate registration
        if (idRegistry_.contains(id)) {
            return MakeError(ErrorCode::TypeAlreadyRegistered, 
                           shine::SString("Type ID ") + shine::SString(std::to_string(id)) + shine::SString(" (") + shine::SString(typeName) + shine::SString(") already registered"));
        }
        
        // Eagerly build lookup tables before sharing
        info.BuildLookup();

        TypeInfo* typeInfo = typeArena_.Create(std::move(info));
        if (!typeInfo) {
            return MakeError(ErrorCode::OutOfMemory,
                           shine::SString("Failed to allocate reflected type arena slot for ") + shine::SString(typeName));
        }

        const auto ownerHandle = ReflectionOwnerHandle::FromType(typeInfo);
        for (auto& field : typeInfo->fields) {
            field.owner = ownerHandle;
        }
        for (auto& method : typeInfo->methods) {
            method.owner = ownerHandle;
        }
        const std::size_t index = types_.size();
        types_.push_back(typeInfo);
        idRegistry_[id] = index;
        nameRegistry_[shine::SString(typeInfo->GetNameView())] = index;
        return {};
    }
    
    // ---- Safe lookup with Result<T> ----
    
    Result<const TypeInfo*> Find(TypeId id) const {
        auto it = idRegistry_.find(id);
        if (it != idRegistry_.end()) {
            return types_[it->second];
        }
        return MakeError(ErrorCode::TypeNotFound, 
                        shine::SString("Type ID ") + shine::SString(std::to_string(id)) + shine::SString(" not found"));
    }
    
    // ---- Fast lookup for performance-critical paths (hot path optimization) ----
    
    [[gnu::always_inline]]
    const TypeInfo* FindFast(TypeId id) const noexcept {
        auto it = idRegistry_.find(id);
        return (it != idRegistry_.end()) ? types_[it->second] : nullptr;
    }

    [[gnu::always_inline]]
    const TypeInfo* FindByNameFast(shine::STextView typeName) const noexcept {
        // Try direct hash lookup first as it's the most common case
        if (const auto* info = FindFast(Hash(typeName))) {
            if (info->GetNameView() == typeName) return info;
        }

        // Fallback to name registry
        auto it = nameRegistry_.find(shine::SString(typeName));
        return (it != nameRegistry_.end()) ? types_[it->second] : nullptr;
    }
    
    template <typename T>
    Result<const TypeInfo*> Find() const { 
        return Find(GetTypeId<T>()); 
    }

    std::size_t GetArenaPageCount() const noexcept { return typeArena_.PageCount(); }
    std::size_t GetArenaSlotsPerPage() const noexcept { return typeArena_.SlotsPerPage(); }
    std::size_t GetArenaPageIndex(const TypeInfo* typeInfo) const noexcept { return typeArena_.PageIndexOf(typeInfo); }


    std::size_t GetRegisteredTypeCount() const { return types_.size(); }

private:
    TypeRegistry() = default;
    ReflectionArena<TypeInfo> typeArena_;
    std::vector<TypeInfo*> types_;
    std::unordered_map<TypeId, std::size_t> idRegistry_;
    std::unordered_map<shine::SString, std::size_t> nameRegistry_;
};

} // namespace shine::reflection
