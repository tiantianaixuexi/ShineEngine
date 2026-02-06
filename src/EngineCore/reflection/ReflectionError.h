#pragma once

// =============================================================================
// ReflectionError.h — Error types for the Shine reflection system (C++23)
// =============================================================================
//
// Uses std::expected<T, ReflectionError> instead of a custom Result type.
//
// =============================================================================

#include <cstdio>
#include <expected>
#include <source_location>
#include <string>
#include <string_view>

namespace shine::reflection {

// =============================================================================
// Error codes
// =============================================================================

enum class ErrorCode {
    Success = 0,

    // Type errors
    TypeNotFound,
    TypeAlreadyRegistered,
    InvalidTypeCast,
    TypeMismatch,

    // Field errors
    FieldNotFound,
    FieldNotAccessible,
    FieldReadOnly,

    // Method errors
    MethodNotFound,
    MethodNotCallable,
    ParameterCountMismatch,

    // Memory errors
    OutOfMemory,
    BufferTooSmall,

    // Serialization errors
    SerializationFailed,
    DeserializationFailed,

    // Container errors
    ContainerOperationFailed,

    // Internal
    InternalError,
    NotImplemented,
};

// Backward-compatible alias
using ReflectionErrorCode = ErrorCode;

constexpr std::string_view ErrorCodeToString(ErrorCode c) noexcept {
    switch (c) {
    case ErrorCode::Success:                  return "Success";
    case ErrorCode::TypeNotFound:             return "Type not found";
    case ErrorCode::TypeAlreadyRegistered:    return "Type already registered";
    case ErrorCode::InvalidTypeCast:          return "Invalid type cast";
    case ErrorCode::TypeMismatch:             return "Type mismatch";
    case ErrorCode::FieldNotFound:            return "Field not found";
    case ErrorCode::FieldNotAccessible:       return "Field not accessible";
    case ErrorCode::FieldReadOnly:            return "Field is read-only";
    case ErrorCode::MethodNotFound:           return "Method not found";
    case ErrorCode::MethodNotCallable:        return "Method not callable";
    case ErrorCode::ParameterCountMismatch:   return "Parameter count mismatch";
    case ErrorCode::OutOfMemory:              return "Out of memory";
    case ErrorCode::BufferTooSmall:           return "Buffer too small";
    case ErrorCode::SerializationFailed:      return "Serialization failed";
    case ErrorCode::DeserializationFailed:    return "Deserialization failed";
    case ErrorCode::ContainerOperationFailed: return "Container operation failed";
    case ErrorCode::InternalError:            return "Internal error";
    case ErrorCode::NotImplemented:           return "Not implemented";
    default:                                  return "Unknown error";
    }
}

// =============================================================================
// ReflectionError
// =============================================================================

struct ReflectionError {
    ErrorCode            code     = ErrorCode::Success;
    std::string          message;
    std::source_location location = std::source_location::current();

    bool IsError()   const { return code != ErrorCode::Success; }
    bool IsSuccess() const { return code == ErrorCode::Success; }

    std::string ToString() const {
        if (IsSuccess()) return "Success";
        std::string r = "[";
        r += ErrorCodeToString(code);
        r += "]";
        if (!message.empty()) { r += ": "; r += message; }
        r += " at ";
        r += location.file_name();
        r += ":";
        r += std::to_string(location.line());
        return r;
    }
};

// =============================================================================
// Result<T>  =  std::expected<T, ReflectionError>
// =============================================================================

template <typename T>
using Result = std::expected<T, ReflectionError>;

// =============================================================================
// Convenience factories
// =============================================================================

inline std::unexpected<ReflectionError> MakeError(
    ErrorCode code,
    std::string_view msg = "",
    std::source_location loc = std::source_location::current())
{
    return std::unexpected(ReflectionError{code, std::string(msg), loc});
}

inline ReflectionError TypeError(std::string_view type_name, std::string_view = "") {
    return {ErrorCode::TypeNotFound, std::string("Type not found: ") + std::string(type_name)};
}
inline ReflectionError FieldError(std::string_view field_name, std::string_view = "") {
    return {ErrorCode::FieldNotFound, std::string("Field not found: ") + std::string(field_name)};
}
inline ReflectionError MethodError(std::string_view method_name, std::string_view = "") {
    return {ErrorCode::MethodNotFound, std::string("Method not found: ") + std::string(method_name)};
}

// =============================================================================
// Logging
// =============================================================================

inline void LogError(const ReflectionError& e) {
    if (e.IsError())
        std::fprintf(stderr, "[Reflection][ERROR] %s\n", e.ToString().c_str());
}

template <typename T>
void LogError(const Result<T>& r) {
    if (!r) LogError(r.error());
}

} // namespace shine::reflection
