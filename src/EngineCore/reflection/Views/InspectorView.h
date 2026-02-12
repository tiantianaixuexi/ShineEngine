#pragma once

// =============================================================================
// InspectorView.h — Editor Inspector View for Reflection System
// =============================================================================
//
// Provides field iteration, UI schema access, and editor-specific functionality
// for property inspection and editing.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"
#include "../Core/TypeView.h"

namespace shine::reflection {

// =============================================================================
// InspectorView — Editor property inspection interface
// =============================================================================

struct InspectorView : TypeView {
    struct FieldIterator {
        const TypeInfo* type;
        std::size_t     index;
        bool operator!=(const FieldIterator& o) const { return index != o.index; }
        void operator++() { ++index; }
        const FieldInfo& operator*() const { return type->fields[index]; }
    };

    FieldIterator begin() const { return {typeInfo, 0}; }
    FieldIterator end()   const { return {typeInfo, typeInfo->fields.size()}; }

    bool IsEditable(const FieldInfo& f) const {
        return HasFlag(f.flags, PropertyFlags::EditAnywhere)
            && !HasFlag(f.flags, PropertyFlags::ReadOnly);
    }

    const UI::Schema& GetUISchema(const FieldInfo& f) const { return f.uiSchema; }

    bool IsVisible(const FieldInfo& f, const void* instance) const {
        if (auto* m = f.GetMeta(MetaKeys::EditCondition);
            m && std::holds_alternative<std::string_view>(*m))
        {
            auto condField = std::get<std::string_view>(*m);
            if (auto* ci = typeInfo->FindField(condField);
                ci && ci->typeId == GetTypeId<bool>())
            {
                bool bVal = false;
                ci->Get(instance, &bVal);
                if (!bVal) return false;
            }
        }
        return true;
    }

    std::string_view GetCategory(const FieldInfo& f) const {
        if (auto* m = f.GetMeta(MetaKeys::Category);
            m && std::holds_alternative<std::string_view>(*m))
            return std::get<std::string_view>(*m);
        return "";
    }

    void SetValue(void* instance, const FieldInfo& f, const void* value) const {
        if (IsEditable(f)) f.Set(instance, value);
    }
};

} // namespace shine::reflection