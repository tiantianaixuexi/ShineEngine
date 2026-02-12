#pragma once

// =============================================================================
// TypeView.h — Base View Structure for Reflection System
// =============================================================================
//
// Defines the base TypeView structure that all view types inherit from.
// This prevents circular dependencies and duplicate definitions.
//
// C++23 / MSVC
// =============================================================================

namespace shine::reflection {

struct TypeInfo;  // Forward declaration

// =============================================================================
// TypeView — Base view structure
// =============================================================================

struct TypeView {
    const TypeInfo* typeInfo = nullptr;
    bool IsValid() const { return typeInfo != nullptr; }
};

} // namespace shine::reflection