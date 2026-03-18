#pragma once

#include <cassert>
#include <cstdint>

namespace shine::reflection {

struct TypeInfo;
struct FieldInfo;
struct MethodInfo;

enum class ReflectionOwnerKind : uint8_t {
    Null = 0,
    Type = 1,
    Field = 2,
    Method = 3,
};

class ReflectionOwnerHandle {
public:
    static constexpr uintptr_t kTagMask = 0x3;
    static constexpr uintptr_t kRequiredAlignment = kTagMask + 1;

    constexpr ReflectionOwnerHandle() noexcept = default;
    constexpr ReflectionOwnerHandle(std::nullptr_t) noexcept {}

    static ReflectionOwnerHandle FromType(const TypeInfo* type) noexcept {
        return Encode(type, ReflectionOwnerKind::Type);
    }

    static ReflectionOwnerHandle FromField(const FieldInfo* field) noexcept {
        return Encode(field, ReflectionOwnerKind::Field);
    }

    static ReflectionOwnerHandle FromMethod(const MethodInfo* method) noexcept {
        return Encode(method, ReflectionOwnerKind::Method);
    }

    [[nodiscard]] constexpr bool IsNull() const noexcept {
        return value_ == 0;
    }

    [[nodiscard]] constexpr ReflectionOwnerKind Kind() const noexcept {
        if (value_ == 0) {
            return ReflectionOwnerKind::Null;
        }

        const auto rawKind = static_cast<ReflectionOwnerKind>(value_ & kTagMask);
        assert(rawKind != ReflectionOwnerKind::Null && "ReflectionOwnerHandle: non-null handle cannot use null tag");
        return rawKind;
    }

    [[nodiscard]] constexpr bool IsType() const noexcept { return Kind() == ReflectionOwnerKind::Type; }
    [[nodiscard]] constexpr bool IsField() const noexcept { return Kind() == ReflectionOwnerKind::Field; }
    [[nodiscard]] constexpr bool IsMethod() const noexcept { return Kind() == ReflectionOwnerKind::Method; }

    [[nodiscard]] const TypeInfo* AsType() const noexcept {
        assert(IsNull() || IsType());
        return IsNull() ? nullptr : static_cast<const TypeInfo*>(DecodePointer());
    }

    [[nodiscard]] const FieldInfo* AsField() const noexcept {
        assert(IsNull() || IsField());
        return IsNull() ? nullptr : static_cast<const FieldInfo*>(DecodePointer());
    }

    [[nodiscard]] const MethodInfo* AsMethod() const noexcept {
        assert(IsNull() || IsMethod());
        return IsNull() ? nullptr : static_cast<const MethodInfo*>(DecodePointer());
    }

    [[nodiscard]] constexpr uintptr_t RawValue() const noexcept {
        return value_;
    }

private:
    uintptr_t value_ = 0;

    explicit constexpr ReflectionOwnerHandle(uintptr_t rawValue) noexcept
        : value_(rawValue) {}

    template <typename T>
    static ReflectionOwnerHandle Encode(const T* ptr, ReflectionOwnerKind kind) noexcept {
        if (ptr == nullptr) {
            return ReflectionOwnerHandle{};
        }

        const auto rawPtr = reinterpret_cast<uintptr_t>(ptr);
        assert((rawPtr & kTagMask) == 0 && "ReflectionOwnerHandle: pointer alignment is insufficient for tag bits");
        return ReflectionOwnerHandle(rawPtr | static_cast<uintptr_t>(kind));
    }

    [[nodiscard]] const void* DecodePointer() const noexcept {
        assert(!IsNull());
        const auto rawPtr = value_ & ~kTagMask;
        assert((rawPtr & kTagMask) == 0 && "ReflectionOwnerHandle: decoded pointer lost required alignment");
        return reinterpret_cast<const void*>(rawPtr);
    }
};

} // namespace shine::reflection