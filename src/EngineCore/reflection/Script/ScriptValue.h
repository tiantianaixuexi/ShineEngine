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

#include <concepts>
#include <map>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>
#include "string/shine_string.h"

namespace shine::reflection {

struct ScriptValue;

struct ScriptArray {
    std::vector<ScriptValue> elements;
};

struct ScriptMap {
    std::map<shine::SString, ScriptValue> elements;
};

// =============================================================================
// ScriptValue — type-erased container for script values
// =============================================================================

struct ScriptValue {
    struct ArrayWrapper { std::shared_ptr<ScriptArray> ptr; };
    struct MapWrapper { std::shared_ptr<ScriptMap> ptr; };

    std::variant<std::monostate, bool, int, float, double, shine::SString,shine::STextView, void*, ArrayWrapper, MapWrapper> data;

    ScriptValue() = default;

    explicit ScriptValue(std::string_view val) : data(shine::SString(val)) {}

    template <typename T>
        requires std::constructible_from<decltype(data), T&&>
    explicit ScriptValue(T&& val) : data(std::forward<T>(val)) {}

    [[nodiscard]] bool IsEmpty() const { return std::holds_alternative<std::monostate>(data); }
};

} // namespace shine::reflection