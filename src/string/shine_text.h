#pragma once

#include "shine_string.h"
#include "shine_text_view.h"

namespace shine
{
    // =========================================================
    // SText: Immutable text wrapper (Localization-ready)
    //
    // Use for user-facing text that should not be mutated after creation.
    // Internally stores an SString, exposes only const access.
    // =========================================================
    class SText {
    public:
        SText() = default;

        explicit SText(SString str) noexcept : _storage(std::move(str)) {}

        explicit SText(STextView view) : _storage(std::string_view(view.data(), view.size())) {}

        explicit SText(std::string_view sv) : _storage(sv) {}

        explicit SText(const char* s) : _storage(s) {}

        [[nodiscard]] const SString& GetString() const noexcept { return _storage; }
        [[nodiscard]] STextView GetView() const noexcept { return _storage.view(); }
        [[nodiscard]] const char* c_str() const noexcept { return _storage.c_str(); }
        [[nodiscard]] size_t size() const noexcept { return _storage.size(); }
        [[nodiscard]] bool empty() const noexcept { return _storage.empty(); }

        [[nodiscard]] bool operator==(const SText& other) const noexcept {
            return _storage.sv() == other._storage.sv();
        }
        [[nodiscard]] std::strong_ordering operator<=>(const SText& other) const noexcept {
            return _storage.sv() <=> other._storage.sv();
        }

    private:
        SString _storage;
    };
}
