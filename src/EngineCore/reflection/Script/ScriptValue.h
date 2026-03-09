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
#include <vector>
#include <map>
#include <memory>

namespace shine::reflection {

struct ScriptValue;

struct ScriptArray {
    std::vector<ScriptValue> elements;
};

struct ScriptMap {
    std::map<std::string, ScriptValue> elements;
};

// =============================================================================
// ScriptValue — type-erased container for script values
// =============================================================================

struct ScriptValue {
    struct ArrayWrapper { std::shared_ptr<ScriptArray> ptr; };
    struct MapWrapper { std::shared_ptr<ScriptMap> ptr; };

    std::variant<std::monostate, bool, int, float, double, std::string, void*, ArrayWrapper, MapWrapper> data;

    ScriptValue() = default;

    template <typename T>
    explicit ScriptValue(T&& val) : data(std::forward<T>(val)) {}

    bool IsEmpty() const { return std::holds_alternative<std::monostate>(data); }
};

} // namespace shine::reflection