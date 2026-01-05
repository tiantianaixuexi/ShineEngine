#pragma once

#include "shine_string.h"
#include "shine_text_view.h"

namespace shine
{
    // =========================================================
    // SText: User-facing text (Immutable, Localization-ready)
    // =========================================================
    class SText {
    public:
        SText() = default;
        explicit SText(SString str) : _storage(std::move(str)) {}
        explicit SText(STextView view) {
             SString s(view.size() + 1, view.size());
             if (view.size() > 0) {
                std::copy(view.begin(), view.end(), s.data());
             }
             if (s.capacity() > s.size()) s.data()[s.size()] = 0;
             _storage = std::move(s);
        }
        
        const SString& GetString() const { return _storage; }
        STextView GetView() const { return _storage.view(); }
        
    private:
        SString _storage;
    };
}
