
#pragma once



#include <string_view>
#include <vector>

#include "constexpr/constexpr_vector.h"

#include "EngineCore/reflection/Reflection.h"
#include "EngineCore/reflection/Field/FieldInfo.h"
#include "EngineCore/reflection/ReflectionHash.h"

namespace shine::reflection {



template<typename >
struct CollectData {

    std::string_view name;
    TypeId           id;
    size_t           size;
    size_t           alignment;
    

    constexpr_::constexpr_vector<FieldInfo>   fields;
    constexpr_::constexpr_vector<MethodInfo>  methods;

    const CollectData *baseType   = nullptr;
    TypeId          baseTypeId = 0;

    bool                   isEnum = false;
    std::vector<EnumEntry> enumEntries;

    void *(*create)();
    void (*destroy)(void *);
    void (*construct)(void *);
    void (*destruct)(void *);
    void (*copy)(void *dst, const void *src); // Assignment

    bool isTrivial;
    bool isManaged = false; // If true, use ObjectHandle in scripts

    const FieldInfo *FindField(std::string_view fieldName) const {
        if (const auto it = std::ranges::find_if(fields, [fieldName](const FieldInfo &f) { return f.name == fieldName; }); it != fields.end())
            return &(*it);
        return baseType ? baseType->FindField(fieldName) : nullptr;
    }
    const MethodInfo *FindMethod(std::string_view methodName) const {
        if (const auto it = std::ranges::find_if(methods, [methodName](const MethodInfo &m) { return m.name == methodName; }); it != methods.end())
            return &(*it);
        return baseType ? baseType->FindMethod(methodName) : nullptr;
    }
};
} // namespace shine::reflection