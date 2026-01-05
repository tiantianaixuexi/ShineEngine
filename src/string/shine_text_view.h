#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <functional>

#include "shine_define.h"

namespace shine
{
    // =========================================================
    // STextView: Read-only view of a UTF-16 string (P5 Refactor)
    // =========================================================
    class STextView
    {
    public:
        using iterator = const TChar*;
        using const_iterator = const TChar*;

        constexpr STextView() noexcept : _p(nullptr), _size(0) {}
        constexpr STextView(const TChar* p, size_t size) noexcept : _p(p), _size(size) {}
        constexpr STextView(const TChar* start, const TChar* end) noexcept : _p(start), _size(end - start) {}

        // Basic Access
        const TChar* data() const noexcept { return _p; }
        size_t size() const noexcept { return _size; }
        bool empty() const noexcept { return _size == 0; }
        
        const_iterator begin() const noexcept { return _p; }
        const_iterator end() const noexcept { return _p + _size; }
        
        // P0: UTF-16 Basic Tools
        static constexpr bool is_high_surrogate(char16_t c) noexcept {
            return c >= 0xD800 && c <= 0xDBFF;
        }

        static constexpr bool is_low_surrogate(char16_t c) noexcept {
            return c >= 0xDC00 && c <= 0xDFFF;
        }

        static char32_t decode_code_point(const char16_t*& it, const char16_t* end) {
             if (it >= end) return 0;
             char16_t c = *it++;
             if (!is_high_surrogate(c)) {
                 if (is_low_surrogate(c)) return 0xFFFD;
                 return c;
             }
             if (it == end) return 0xFFFD;
             char16_t c2 = *it;
             if (is_low_surrogate(c2)) {
                 it++;
                 char32_t hi = c - 0xD800;
                 char32_t lo = c2 - 0xDC00;
                 return 0x10000 + ((hi << 10) | lo);
             }
             return 0xFFFD;
        }

        bool is_valid_utf16() const {
            for (size_t i = 0; i < _size; ++i) {
                char16_t c = _p[i];
                if (is_high_surrogate(c)) {
                    if (i + 1 >= _size) return false;
                    if (!is_low_surrogate(_p[i+1])) return false;
                    i++;
                } else if (is_low_surrogate(c)) {
                    return false;
                }
            }
            return true;
        }

        // P0: Length / Char Count
        size_t code_unit_count() const noexcept { return _size; }
        size_t code_point_count() const {
            size_t count = 0;
            const TChar* p = _p;
            const TChar* end = _p + _size;
            while (p < end) {
                if (is_high_surrogate(*p) && (p + 1 < end) && is_low_surrogate(*(p + 1))) {
                    p += 2;
                } else {
                    p++;
                }
                count++;
            }
            return count;
        }

        // P0: Traversal
        void for_each_code_point(std::function<void(char32_t)> fn) const {
            const TChar* p = _p;
            const TChar* end = _p + _size;
            while (p < end) {
                fn(decode_code_point(p, end));
            }
        }

        size_t find(char32_t cp) const {
            const TChar* p = _p;
            const TChar* end = _p + _size;
            const TChar* start = _p;
            while (p < end) {
                const TChar* current = p;
                if (decode_code_point(p, end) == cp) {
                    return current - start;
                }
            }
            return npos;
        }

        bool contains(char32_t cp) const {
            return find(cp) != npos;
        }

        static constexpr size_t npos = static_cast<size_t>(-1);

        // P1: Substring / Index
        size_t utf16_index_from_code_point(size_t cp_index) const {
            size_t current_cp = 0;
            size_t idx = 0;
            while (idx < _size && current_cp < cp_index) {
                if (is_high_surrogate(_p[idx]) && (idx + 1 < _size) && is_low_surrogate(_p[idx + 1])) {
                    idx += 2;
                } else {
                    idx++;
                }
                current_cp++;
            }
            return (current_cp == cp_index) ? idx : npos;
        }

        STextView substr_units(size_t unit_pos, size_t unit_count) const {
            if (unit_pos >= _size) return STextView();
            size_t count = std::min(unit_count, _size - unit_pos);
            return STextView(_p + unit_pos, count);
        }

        STextView substr_cp(size_t pos, size_t count) const {
             size_t start_idx = utf16_index_from_code_point(pos);
             if (start_idx == npos) return STextView();
             
             size_t end_idx = start_idx;
             size_t found = 0;
             while (end_idx < _size && found < count) {
                  if (is_high_surrogate(_p[end_idx]) && (end_idx + 1 < _size) && is_low_surrogate(_p[end_idx + 1])) {
                    end_idx += 2;
                } else {
                    end_idx++;
                }
                found++;
             }
             return substr_units(start_idx, end_idx - start_idx);
        }

        // P1: Cursor / Edit Support
        bool is_code_point_boundary(size_t utf16_index) const {
            if (utf16_index == 0) return true;
            if (utf16_index >= _size) return true;
            return !is_low_surrogate(_p[utf16_index]);
        }

        size_t next_code_point(size_t utf16_index) const {
            if (utf16_index >= _size) return _size;
            if (is_high_surrogate(_p[utf16_index]) && (utf16_index + 1 < _size) && is_low_surrogate(_p[utf16_index + 1])) {
                return utf16_index + 2;
            }
            return utf16_index + 1;
        }

        size_t prev_code_point(size_t utf16_index) const {
            if (utf16_index == 0) return 0;
            if (utf16_index > _size) return _size;
            size_t prev = utf16_index - 1;
            if (is_low_surrogate(_p[prev]) && prev > 0 && is_high_surrogate(_p[prev - 1])) {
                return prev - 1;
            }
            return prev;
        }

        // P1: Search / Compare
        size_t find_cp(char32_t cp, size_t start = 0) const {
            const TChar* p = _p + start;
            const TChar* end = _p + _size;
            while (p < end) {
                const TChar* current_p = p;
                if (decode_code_point(p, end) == cp) {
                    return current_p - _p;
                }
            }
            return npos;
        }

        int compare_cp(STextView rhs) const {
            const TChar* p1 = _p;
            const TChar* end1 = _p + _size;
            const TChar* p2 = rhs._p;
            const TChar* end2 = rhs._p + rhs._size;
            
            while (p1 < end1 && p2 < end2) {
                char32_t c1 = decode_code_point(p1, end1);
                char32_t c2 = decode_code_point(p2, end2);
                if (c1 < c2) return -1;
                if (c1 > c2) return 1;
            }
            if (p1 == end1 && p2 == end2) return 0;
            return (p1 == end1) ? -1 : 1;
        }

        bool equals(STextView rhs) const {
            if (_size != rhs._size) return false;
            return std::equal(_p, _p + _size, rhs._p);
        }

        bool starts_with(STextView prefix) const {
            if (prefix.size() > _size) return false;
            return std::equal(prefix.begin(), prefix.end(), _p);
        }

        bool ends_with(STextView suffix) const {
            if (suffix.size() > _size) return false;
            return std::equal(suffix.begin(), suffix.end(), _p + (_size - suffix.size()));
        }

        STextView trim_start() const {
            size_t start = 0;
            while (start < _size) {
                char16_t c = _p[start];
                // Simple whitespace check (space, tab, CR, LF)
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    start++;
                } else {
                    break;
                }
            }
            return substr_units(start, _size - start);
        }

        STextView trim_end() const {
            size_t end = _size;
            while (end > 0) {
                char16_t c = _p[end - 1];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                    end--;
                } else {
                    break;
                }
            }
            return substr_units(0, end);
        }

        STextView trim() const {
            return trim_start().trim_end();
        }

        // P2: UTF-8 Interaction
        std::string to_utf8() const {
            std::string res;
            res.reserve(_size * 3);
            const TChar* p = _p;
            const TChar* end = _p + _size;
            while (p < end) {
                char32_t cp = decode_code_point(p, end);
                if (cp < 0x80) {
                    res.push_back((char)cp);
                } else if (cp < 0x800) {
                    res.push_back((char)(0xC0 | (cp >> 6)));
                    res.push_back((char)(0x80 | (cp & 0x3F)));
                } else if (cp < 0x10000) {
                    res.push_back((char)(0xE0 | (cp >> 12)));
                    res.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                    res.push_back((char)(0x80 | (cp & 0x3F)));
                } else {
                    res.push_back((char)(0xF0 | (cp >> 18)));
                    res.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
                    res.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                    res.push_back((char)(0x80 | (cp & 0x3F)));
                }
            }
            return res;
        }

        static bool is_valid_utf8(std::string_view sv) {
             for (size_t i = 0; i < sv.size(); ) {
                unsigned char c = sv[i];
                if (c < 0x80) { i++; }
                else if ((c & 0xE0) == 0xC0) { 
                    if (i+1 >= sv.size() || (sv[i+1] & 0xC0) != 0x80) return false;
                    if ((c & 0xFE) == 0xC0) return false;
                    i += 2; 
                }
                else if ((c & 0xF0) == 0xE0) { 
                    if (i+2 >= sv.size() || (sv[i+1] & 0xC0) != 0x80 || (sv[i+2] & 0xC0) != 0x80) return false;
                    i += 3; 
                }
                else if ((c & 0xF8) == 0xF0) { 
                    if (i+3 >= sv.size() || (sv[i+1] & 0xC0) != 0x80 || (sv[i+2] & 0xC0) != 0x80 || (sv[i+3] & 0xC0) != 0x80) return false;
                    i += 4; 
                }
                else return false;
            }
            return true;
        }

    protected:
        const TChar* _p;
        size_t _size;
    };
}
