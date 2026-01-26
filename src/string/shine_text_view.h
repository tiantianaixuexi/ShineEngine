#pragma once
#include <algorithm>
#include <string_view>
#include <functional>
#include <memory>
#include <cstdint>

namespace shine
{
    // =========================================================
    // STextView: Read-only view of a UTF-8 string
    // Features:
    // - Zero-copy string view
    // - Shared ownership support via shared_ptr
    // - UTF-8 encoding/decoding support
    // - Code point iteration
    // =========================================================
    class STextView
    {
    public:
        using iterator = const char*;
        using const_iterator = const char*;

        constexpr STextView() noexcept : _p(nullptr), _size(0), _owner(nullptr) {}
        
        constexpr STextView(std::nullptr_t) noexcept : STextView() {}
        
        constexpr STextView(const char* p, size_t size) noexcept 
            : _p(p), _size(size), _owner(nullptr) {}
        
        constexpr STextView(const char* start, const char* end) noexcept 
            : _p(start), _size(end - start), _owner(nullptr) {}

        STextView(const char* p, size_t size, std::shared_ptr<const void> owner) noexcept
            : _p(p), _size(size), _owner(std::move(owner)) {}

        STextView(STextView&& other) noexcept
            : _p(other._p), _size(other._size), _owner(std::move(other._owner)) {
            other._p = nullptr;
            other._size = 0;
        }

        STextView& operator=(STextView&& other) noexcept {
            if (this != &other) {
                _p = other._p;
                _size = other._size;
                _owner = std::move(other._owner);
                other._p = nullptr;
                other._size = 0;
            }
            return *this;
        }

        STextView(const STextView&) = default;
        STextView& operator=(const STextView&) = default;

        static constexpr STextView from_cstring(const char* s) noexcept {
            return { s, std::char_traits<char>::length(s) } ;
        }

        template <size_t N>
        static constexpr STextView from_literal(const char (&s)[N]) noexcept {
            return STextView(s, N - 1);
        }

        [[nodiscard]] constexpr const char* data() const noexcept { return _p; }
        [[nodiscard]] constexpr size_t size() const noexcept { return _size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }
        [[nodiscard]] bool is_valid() const noexcept { return _p != nullptr || _size == 0; }
        [[nodiscard]] bool is_shared() const noexcept { return _owner != nullptr; }
        
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return _p; }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return _p + _size; }
        
        [[nodiscard]] std::shared_ptr<const void> owner() const noexcept { return _owner; }

        void reset() noexcept {
            _p = nullptr;
            _size = 0;
            _owner.reset();
        }

        [[nodiscard]] static bool is_valid_utf8(std::string_view sv) {
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

        static std::pair<char32_t, int> utf8_to_utf32_char(std::string_view sv) noexcept {
            if (sv.empty()) return {0, 0};
            unsigned char c = sv[0];
            if (c < 0x80) return {static_cast<char32_t>(c), 1};
            if ((c & 0xE0) == 0xC0 && sv.size() >= 2) {
                char32_t cp = ((c & 0x1F) << 6) | (sv[1] & 0x3F);
                return {cp, 2};
            }
            if ((c & 0xF0) == 0xE0 && sv.size() >= 3) {
                char32_t cp = ((c & 0x0F) << 12) | ((sv[1] & 0x3F) << 6) | (sv[2] & 0x3F);
                return {cp, 3};
            }
            if ((c & 0xF8) == 0xF0 && sv.size() >= 4) {
                char32_t cp = ((c & 0x07) << 18) | ((sv[1] & 0x3F) << 12) | ((sv[2] & 0x3F) << 6) | (sv[3] & 0x3F);
                return {cp, 4};
            }
            return {0xFFFD, 1};
        }

        static int utf32_to_utf8(char32_t cp, char* out) noexcept {
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

       static char32_t decode_code_point(const char*& it, const char* end) {
             if (it >= end) return 0;
             unsigned char c = *it++;
             if (c < 0x80) return c;
             if ((c & 0xE0) == 0xC0) {
                 if (it >= end) return 0xFFFD;
                 unsigned char c2 = *it;
                 if ((c2 & 0xC0) != 0x80) return 0xFFFD;
                 it++;
                 return ((c & 0x1F) << 6) | (c2 & 0x3F);
             }
             if ((c & 0xF0) == 0xE0) {
                 if (it + 1 >= end) return 0xFFFD;
                 unsigned char c2 = *it++;
                 unsigned char c3 = *it;
                 if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return 0xFFFD;
                 return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
             }
             if ((c & 0xF8) == 0xF0) {
                 if (it + 2 >= end) return 0xFFFD;
                 unsigned char c2 = *it++;
                 unsigned char c3 = *it++;
                 unsigned char c4 = *it;
                 if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) return 0xFFFD;
                 return ((c & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
             }
             return 0xFFFD;
        }

        static void encode_code_point(char32_t cp, std::string& out) {
            if (cp < 0x80) {
                out.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }

        [[nodiscard]] constexpr size_t code_unit_count() const noexcept { return _size; }
        
        [[nodiscard]] size_t code_point_count() const {
            size_t count = 0;
            const char* p = _p;
            const char* end = _p + _size;
            while (p < end) {
                decode_code_point(p, end);
                count++;
            }
            return count;
        }

        void for_each_code_point(std::function<void(char32_t)> fn) const {
            const char* p = _p;
            const char* end = _p + _size;
            while (p < end) {
                fn(decode_code_point(p, end));
            }
        }

        [[nodiscard]] size_t find(char32_t cp) const {
            const char* p = _p;
            const char* end = _p + _size;
            const char* start = _p;
            while (p < end) {
                const char* current = p;
                if (decode_code_point(p, end) == cp) {
                    return current - start;
                }
            }
            return npos;
        }

        static constexpr size_t npos = static_cast<size_t>(-1);

        [[nodiscard]] size_t utf8_index_from_code_point(size_t cp_index) const {
            size_t current_cp = 0;
            size_t idx = 0;
            const char* p = _p;
            const char* end = _p + _size;
            while (idx < _size && current_cp < cp_index) {
                decode_code_point(p, end);
                idx = p - _p;
                current_cp++;
            }
            return (current_cp == cp_index) ? idx : npos;
        }

        [[nodiscard]] STextView substr_units(size_t unit_pos, size_t unit_count) const {
            if (unit_pos >= _size) return STextView();
            size_t count = std::min(unit_count, _size - unit_pos);
            return STextView(_p + unit_pos, count, _owner);
        }

        [[nodiscard]] STextView substr_cp(size_t pos, size_t count) const {
             size_t start_idx = utf8_index_from_code_point(pos);
             if (start_idx == npos) return STextView();
             
             const char* p = _p + start_idx;
             const char* end = _p + _size;
             for (size_t i = 0; i < count && p < end; ++i) {
                 decode_code_point(p, end);
             }
             return substr_units(start_idx, p - (_p + start_idx));
        }

        [[nodiscard]] size_t find(STextView pattern) const {
            if (pattern.empty()) return 0;
            if (pattern.size() > _size) return npos;

            const char* end = _p + _size - pattern.size() + 1;
            for (const char* p = _p; p < end; ++p) {
                if (std::ranges::equal(std::string_view(p, pattern.size()), std::string_view(pattern.data(), pattern.size()))) {
                    return p - _p;
                }
            }
            return npos;
        }

        [[nodiscard]] size_t find(STextView pattern, size_t start) const {
            if (start >= _size) return npos;
            if (pattern.empty()) return start;
            if (pattern.size() > _size - start) return npos;

            const char* end = _p + _size - pattern.size() + 1;
            for (const char* p = _p + start; p < end; ++p) {
                if (std::ranges::equal(std::string_view(p, pattern.size()), std::string_view(pattern.data(), pattern.size()))) {
                    return p - _p;
                }
            }
            return npos;
        }

        [[nodiscard]] size_t find_cp(char32_t cp) const { return find(cp); }

        [[nodiscard]] bool contains(STextView pattern) const { return find(pattern) != npos; }
        [[nodiscard]] bool contains(char32_t cp) const { return find(cp) != npos; }

        int compare_cp(STextView rhs) const {
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

        [[nodiscard]] bool equals(STextView rhs) const {
            if (_size != rhs._size) return false;
            return std::ranges::equal(std::string_view(_p, _size), std::string_view(rhs._p, rhs._size));
        }

        [[nodiscard]] bool starts_with(STextView prefix) const {
            if (prefix.size() > _size) return false;
            return std::ranges::equal(std::string_view(prefix.data(), prefix.size()), std::string_view(_p, prefix.size()));
        }

        [[nodiscard]] bool ends_with(STextView suffix) const {
            if (suffix.size() > _size) return false;
            return std::ranges::equal(std::string_view(suffix.data(), suffix.size()), 
                                      std::string_view(_p + (_size - suffix.size()), suffix.size()));
        }

        [[nodiscard]] STextView trim_start() const {
            size_t start = 0;
            while (start < _size) {
                unsigned char c = _p[start];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xC2 || c == 0xA0) {
                    if (c == 0xC2 && start + 1 < _size && _p[start + 1] == 0xA0) {
                        start += 2;
                    } else {
                        start++;
                    }
                } else {
                    break;
                }
            }
            return substr_units(start, _size - start);
        }

        [[nodiscard]] STextView trim_end() const {
            size_t end = _size;
            while (end > 0) {
                unsigned char c = _p[end - 1];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0xC2 || c == 0xA0) {
                    if (c == 0xA0 && end >= 2 && _p[end - 2] == 0xC2) {
                        end -= 2;
                    } else {
                        end--;
                    }
                } else {
                    break;
                }
            }
            return substr_units(0, end);
        }

        [[nodiscard]] STextView trim() const { return trim_start().trim_end(); }

        [[nodiscard]] std::string to_string() const { return std::string(_p, _size); }
        
        constexpr operator std::string_view() const noexcept { return std::string_view(_p, _size); }

    private:
        const char* _p = nullptr;
        size_t _size = 0;
        std::shared_ptr<const void> _owner;
    };

    [[nodiscard]] constexpr bool operator==(STextView lhs, STextView rhs) noexcept {
        return lhs.size() == rhs.size() && std::ranges::equal(lhs, rhs);
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(STextView lhs, STextView rhs) noexcept {
        auto cmp = std::string_view(lhs).compare(std::string_view(rhs));
        if (cmp < 0) return std::strong_ordering::less;
        if (cmp > 0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }
}
