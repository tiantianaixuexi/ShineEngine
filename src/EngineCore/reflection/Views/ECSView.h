#pragma once

// =============================================================================
// ECSView.h — ECS Component Layout View for Reflection System
// =============================================================================
//
// Provides component layout information for ECS systems, including size,
// alignment, and memory layout details for component pooling.
//
// C++23 / MSVC
// =============================================================================

#include "../ReflectionCore.h"

namespace shine::reflection {

// =============================================================================
// ECSView — ECS component layout interface
// =============================================================================

struct ECSView {
    struct ComponentLayout {
        std::size_t     size;
        std::size_t     alignment;
        const TypeInfo* layoutSource;
    };
    ComponentLayout layout;
    std::size_t GetSize()      const { return layout.size; }
    std::size_t GetAlignment() const { return layout.alignment; }
};

} // namespace shine::reflection