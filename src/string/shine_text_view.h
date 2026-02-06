#pragma once
#include <algorithm>
#include <string_view>
#include <functional>
#include <cstdint>
#include <cstring>
#include <compare>

namespace shine
{
    // =========================================================
    // STextView: Read-only, trivially-copyable UTF-8 string view
    //
    // Design:
    //   - 16 bytes (same as std::string_view), no heap, no atomics
    //   - Zero-copy, non-owning reference
    //   - UTF-8 encoding/decoding support
    //   - Code point iteration
    //
    // If you need shared ownership, wrap in shared_ptr<SString>
    // and take STextView from it at call-site.
    // =========================================================
    class STextView
    {
    public:
        using iterator = const char*;
        using const_iterator = const char*;

        constexpr STextView() noexcept = default;

        constexpr STextView(std::nullptr_t) noexcept : STextView() {}

        constexpr STextView(const char* p, size_t size) noexcept
            : _p(p), _size(size) {}

        constexpr STextView(const char* start, const char* end) noexcept
            : _p(start), _size(static_cast<size_t>(end - start)) {}

        // Implicit from string_view for seamless interop
        constexpr STextView(std::string_view sv) noexcept
            : _p(sv.data()), _size(sv.size()) {}

        constexpr STextView(const STextView&) noexcept = default;
        constexpr STextView& operator=(const STextView&) noexcept = default;
        constexpr STextView(STextView&&) noexcept = default;
        constexpr STextView& operator=(STextView&&) noexcept = default;

        static constexpr STextView from_cstring(const char* s) noexcept {
            return { s, std::char_traits<char>::length(s) };
        }

        template <size_t N>
        static constexpr STextView from_literal(const char (&s)[N]) noexcept {
            return STextView(s, N - 1);
        }

        [[nodiscard]] constexpr const char* data() const noexcept { return _p; }
        [[nodiscard]] constexpr size_t size() const noexcept { return _size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }
        [[nodiscard]] constexpr bool is_valid() const noexcept { return _p != nullptr || _size == 0; }

        [[nodiscard]] constexpr const_iterator begin() const noexcept { return _p; }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return _p + _size; }

        [[nodiscard]] constexpr const char& operator[](size_t i) const noexcept { return _p[i]; }

        void reset() noexcept {
            _p = nullptr;
            _size = 0;
        }

        // =========================================================
        // UTF-8 Utilities (static)
        // =========================================================

        [[nodiscard]] static bool is_valid_utf8(std::string_view sv) noexcept {
            const auto* p = reinterpret_cast<const unsigned char*>(sv.data());
            const auto* end = p + sv.size();
            while (p < end) {
                unsigned char c = *p;
                int len;
                if (c < 0x80) { len = 1; }
                else if ((c & 0xE0) == 0xC0) {
                    len = 2;
                    if ((c & 0xFE) == 0xC0) return false; // overlong
                }
                else if ((c & 0xF0) == 0xE0) { len = 3; }
                else if ((c & 0xF8) == 0xF0) { len = 4; }
                else return false;

                if (p + len > end) return false;
                for (int i = 1; i < len; ++i) {
                    if ((p[i] & 0xC0) != 0x80) return false;
                }
                p += len;
            }
            return true;
        }

        [[nodiscard]] static constexpr bool is_utf8_start_byte(unsigned char c) noexcept {
            return (c & 0xC0) != 0x80;
        }

        [[nodiscard]] static constexpr int utf8_char_len(unsigned char c) noexcept {
            if (c < 0x80) return 1;
            if ((c & 0xE0) == 0xC0) return 2;
            if ((c & 0xF0) == 0xE0) return 3;
            if ((c & 0xF8) == 0xF0) return 4;
            return 0;
        }

        [[nodiscard]] static constexpr std::pair<char32_t, int> utf8_to_utf32_char(const char* p, size_t avail) noexcept {
            if (avail == 0) return {0, 0};
            unsigned char c = static_cast<unsigned char>(p[0]);
            if (c < 0x80) return {static_cast<char32_t>(c), 1};
            if ((c & 0xE0) == 0xC0 && avail >= 2) {
                char32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F);
                return {cp, 2};
            }
            if ((c & 0xF0) == 0xE0 && avail >= 3) {
                char32_t cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6)
                    | (static_cast<unsigned char>(p[2]) & 0x3F);
                return {cp, 3};
            }
            if ((c & 0xF8) == 0xF0 && avail >= 4) {
                char32_t cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12)
                    | ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) | (static_cast<unsigned char>(p[3]) & 0x3F);
                return {cp, 4};
            }
            return {0xFFFD, 1}; // replacement char
        }

        static constexpr char32_t decode_code_point(const char*& it, const char* end) noexcept {
            if (it >= end) return 0;
            auto [cp, len] = utf8_to_utf32_char(it, static_cast<size_t>(end - it));
            it += len;
            return cp;
        }

        static constexpr int utf32_to_utf8(char32_t cp, char* out) noexcept {
            if (cp < 0x80) {
                out[0] = static_cast<char>(cp);
                return 1;
            }
            if (cp < 0x800) {
                out[0] = static_cast<char>(0xC0 | (cp >> 6));
                out[1] = static_cast<char>(0x80 | (cp & 0x3F));
                return 2;
            }
            if (cp < 0x10000) {
                out[0] = static_cast<char>(0xE0 | (cp >> 12));
                out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out[2] = static_cast<char>(0x80 | (cp & 0x3F));
                return 3;
            }
            out[0] = static_cast<char>(0xF0 | (cp >> 18));
            out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[3] = static_cast<char>(0x80 | (cp & 0x3F));
            return 4;
        }

        static void encode_code_point(char32_t cp, std::string& out) {
            char buf[4];
            int len = utf32_to_utf8(cp, buf);
            out.append(buf, static_cast<size_t>(len));
        }

        // =========================================================
        // Code Point Operations
        // =========================================================

        [[nodiscard]] constexpr size_t code_unit_count() const noexcept { return _size; }

        [[nodiscard]] size_t code_point_count() const noexcept {
            size_t count = 0;
            const char* p = _p;
            const char* e = _p + _size;
            while (p < e) {
                decode_code_point(p, e);
                ++count;
            }
            return count;
        }

        template <typename Fn>
        void for_each_code_point(Fn&& fn) const {
            const char* p = _p;
            const char* e = _p + _size;
            while (p < e) {
                fn(decode_code_point(p, e));
            }
        }

        [[nodiscard]] size_t find(char32_t cp) const noexcept {
            const char* p = _p;
            const char* e = _p + _size;
            while (p < e) {
                const char* current = p;
                if (decode_code_point(p, e) == cp) {
                    return static_cast<size_t>(current - _p);
                }
            }
            return npos;
        }

        static constexpr size_t npos = static_cast<size_t>(-1);

        [[nodiscard]] size_t utf8_index_from_code_point(size_t cp_index) const noexcept {
            size_t current_cp = 0;
            const char* p = _p;
            const char* e = _p + _size;
            while (p < e && current_cp < cp_index) {
                decode_code_point(p, e);
                ++current_cp;
            }
            return (current_cp == cp_index) ? static_cast<size_t>(p - _p) : npos;
        }

        [[nodiscard]] constexpr STextView substr_units(size_t unit_pos, size_t unit_count) const noexcept {
            if (unit_pos >= _size) return STextView();
            size_t count = std::min(unit_count, _size - unit_pos);
            return STextView(_p + unit_pos, count);
        }

        [[nodiscard]] STextView substr_cp(size_t pos, size_t count) const noexcept {
            size_t start_idx = utf8_index_from_code_point(pos);
            if (start_idx == npos) return STextView();

            const char* p = _p + start_idx;
            const char* e = _p + _size;
            for (size_t i = 0; i < count && p < e; ++i) {
                decode_code_point(p, e);
            }
            return substr_units(start_idx, static_cast<size_t>(p - (_p + start_idx)));
        }

        // =========================================================
        // Find / Search (byte-level, use memchr for speed)
        // =========================================================

        [[nodiscard]] size_t find(STextView pattern) const noexcept {
            return find(pattern, 0);
        }

        [[nodiscard]] size_t find(STextView pattern, size_t start) const noexcept {
            if (pattern.empty()) return start <= _size ? start : npos;
            if (start >= _size) return npos;
            size_t pat_len = pattern.size();
            if (pat_len > _size - start) return npos;

            const char first = pattern._p[0];
            const char* p = _p + start;
            const char* p_end = _p + _size - pat_len;

            while (p <= p_end) {
                p = static_cast<const char*>(std::memchr(p, first, static_cast<size_t>(p_end - p + 1)));
                if (!p) return npos;
                if (std::memcmp(p, pattern._p, pat_len) == 0) {
                    return static_cast<size_t>(p - _p);
                }
                ++p;
            }
            return npos;
        }

        [[nodiscard]] size_t find_cp(char32_t cp) const noexcept { return find(cp); }

        [[nodiscard]] bool contains(STextView pattern) const noexcept { return find(pattern) != npos; }
        [[nodiscard]] bool contains(char32_t cp) const noexcept { return find(cp) != npos; }

        // =========================================================
        // Comparison
        // =========================================================

        [[nodiscard]] int compare_cp(STextView rhs) const noexcept {
            const char* p1 = _p;
            const char* end1 = _p + _size;
            const char* p2 = rhs._p;
            const char* end2 = rhs._p + rhs._size;

            while (p1 < end1 && p2 < end2) {
                char32_t c1 = decode_code_point(p1, end1);
                char32_t c2 = decode_code_point(p2, end2);
                if (c1 < c2) return -1;
                if (c1 > c2) return 1;
            }
            if (p1 == end1 && p2 == end2) return 0;
            return (p1 == end1) ? -1 : 1;
        }

        [[nodiscard]] constexpr bool equals(STextView rhs) const noexcept {
            return _size == rhs._size && (_p == rhs._p || std::string_view(_p, _size) == std::string_view(rhs._p, rhs._size));
        }

        [[nodiscard]] bool starts_with(STextView prefix) const noexcept {
            if (prefix._size > _size) return false;
            return std::memcmp(_p, prefix._p, prefix._size) == 0;
        }

        [[nodiscard]] bool ends_with(STextView suffix) const noexcept {
            if (suffix._size > _size) return false;
            return std::memcmp(_p + _size - suffix._size, suffix._p, suffix._size) == 0;
        }

        // =========================================================
        // Trim (whitespace: space, tab, \n, \r, UTF-8 NBSP U+00A0)
        // =========================================================

        [[nodiscard]] STextView trim_start() const noexcept {
            size_t start = 0;
            while (start < _size) {
                unsigned char c = static_cast<unsigned char>(_p[start]);
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    ++start;
                } else if (c == 0xC2 && start + 1 < _size && static_cast<unsigned char>(_p[start + 1]) == 0xA0) {
                    start += 2; // UTF-8 NBSP (U+00A0 = 0xC2 0xA0)
                } else {
                    break;
                }
            }
            return substr_units(start, _size - start);
        }

        [[nodiscard]] STextView trim_end() const noexcept {
            size_t end = _size;
            while (end > 0) {
                unsigned char c = static_cast<unsigned char>(_p[end - 1]);
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    --end;
                } else if (c == 0xA0 && end >= 2 && static_cast<unsigned char>(_p[end - 2]) == 0xC2) {
                    end -= 2; // UTF-8 NBSP
                } else {
                    break;
                }
            }
            return substr_units(0, end);
        }

        [[nodiscard]] STextView trim() const noexcept { return trim_start().trim_end(); }

        // =========================================================
        // Conversion
        // =========================================================

        [[nodiscard]] std::string to_string() const { return std::string(_p, _size); }

        constexpr operator std::string_view() const noexcept { return std::string_view(_p, _size); }

    private:
        const char* _p = nullptr;
        size_t _size = 0;
    };

    // =========================================================
    // Operators
    // =========================================================

    [[nodiscard]] constexpr bool operator==(STextView lhs, STextView rhs) noexcept {
        return lhs.size() == rhs.size()
            && (lhs.data() == rhs.data() || std::string_view(lhs) == std::string_view(rhs));
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(STextView lhs, STextView rhs) noexcept {
        auto cmp = std::string_view(lhs).compare(std::string_view(rhs));
        if (cmp < 0) return std::strong_ordering::less;
        if (cmp > 0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

    // static_assert to guarantee the view is lightweight
    static_assert(sizeof(STextView) == 2 * sizeof(void*), "STextView should be pointer+size only");
    static_assert(std::is_trivially_copyable_v<STextView>, "STextView must be trivially copyable");
}
