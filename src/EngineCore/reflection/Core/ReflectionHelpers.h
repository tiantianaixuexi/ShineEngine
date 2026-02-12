#pragma once

// =============================================================================
// ReflectionHelpers.h — Helper Functions for Reflection System
// =============================================================================
//
// Contains utility functions and templates that support the reflection system.
// Includes type extraction helpers and common operations.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"

namespace shine::reflection {

// Helper: check if a flag is set
template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
constexpr bool HasFlag(Enum flags, Enum flag) {
    using underlying = std::underlying_type_t<Enum>;
    return (static_cast<underlying>(flags) & static_cast<underlying>(flag)) != 0;
}

} // namespace shine::reflection