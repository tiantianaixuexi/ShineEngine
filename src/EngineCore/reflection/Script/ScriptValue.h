#pragma once

// =============================================================================
// ScriptValue.h — Type-erased Script Value for Reflection System
// =============================================================================
//
// Provides a variant-based container for script-interoperable values.
// Supports common types like bool, numbers, strings, and object pointers.
//
// C++23 / MSVC
// =============================================================================

#include <variant>
#include <string>
#include <string_view>

namespace shine::reflection {

// =============================================================================
// ScriptValue — type-erased container for script values
// =============================================================================

struct ScriptValue {
    std::variant<std::monostate, bool, int, float, double, std::string, void*> data;

    ScriptValue() = default;

    template <typename T>
    explicit ScriptValue(T&& val) : data(std::forward<T>(val)) {}

    bool IsEmpty() const { return std::holds_alternative<std::monostate>(data); }
};

} // namespace shine::reflection