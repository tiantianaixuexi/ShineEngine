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
        const std::string name(info.name);
        
        // Check for duplicate registration
        if (registry_.find(id) != registry_.end()) {
            return MakeError(ErrorCode::TypeAlreadyRegistered, 
                           std::string("Type ") + std::string(info.name) + " already registered");
        }
        if (nameRegistry_.find(name) != nameRegistry_.end()) {
            return MakeError(ErrorCode::TypeAlreadyRegistered,
                           std::string("Type name ") + name + " already registered");
        }
        
        auto typeInfo = std::make_shared<TypeInfo>(std::move(info));
        registry_[id] = typeInfo;
        nameRegistry_[name] = typeInfo;
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
        auto it = nameRegistry_.find(std::string(typeName));
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
    std::unordered_map<std::string, std::shared_ptr<TypeInfo>> nameRegistry_;
};

} // namespace shine::reflection
