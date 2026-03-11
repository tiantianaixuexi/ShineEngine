#pragma once

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace shine
{
    // =========================================================
    // STextView
    //
    // Compact non-owning UTF-8 byte view for engine code.
    //
    // Design:
    //   - Same size as std::string_view on typical platforms
    //   - No allocation, no ownership, trivially copyable
    //   - Primary indexing model is bytes/code units
    //   - Unicode-aware helpers are explicit
    //
    // Important:
    //   - size(), substr(), find(), operator[] all operate on bytes
    //   - UTF-8 helpers are named with code_point / cp terminology
    //   - Lifetime is borrowed from external storage
    // =========================================================
    class STextView
    {
    public:
        using value_type = char;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const char*;
        using const_pointer = const char*;
        using reference = const char&;
        using const_reference = const char&;
        using iterator = const char*;
        using const_iterator = const char*;

        static constexpr size_type npos = static_cast<size_type>(-1);

        // -----------------------------------------------------
        // construction
        // -----------------------------------------------------

        constexpr STextView() noexcept = default;
        constexpr STextView(std::nullptr_t) noexcept
        {
        }

        constexpr STextView(const char* data, size_type size) noexcept
            : _data(data), _size(size)
        {
        }

        constexpr STextView(const char* first, const char* last) noexcept
            : _data(first), _size(static_cast<size_type>(last - first))
        {
        }

        constexpr STextView(std::string_view sv) noexcept
            : _data(sv.data()), _size(sv.size())
        {
        }

        constexpr STextView(const char* cstr) noexcept
            : _data(cstr), _size(cstr ? std::char_traits<char>::length(cstr) : 0)
        {
        }

        constexpr STextView(const STextView&) noexcept = default;
        constexpr STextView& operator=(const STextView&) noexcept = default;
        constexpr STextView(STextView&&) noexcept = default;
        constexpr STextView& operator=(STextView&&) noexcept = default;
        ~STextView() = default;

        template <size_type N>
        [[nodiscard]] static constexpr STextView from_literal(const char (&lit)[N]) noexcept
        {
            static_assert(N > 0);
            return STextView(lit, N - 1);
        }

        [[nodiscard]] static constexpr STextView from_cstring(const char* s) noexcept
        {
            return s ? STextView(s, std::char_traits<char>::length(s)) : STextView{};
        }

        // -----------------------------------------------------
        // access
        // -----------------------------------------------------

        [[nodiscard]] constexpr const char* data() const noexcept { return _data; }
        [[nodiscard]] constexpr size_type size() const noexcept { return _size; }
        [[nodiscard]] constexpr size_type size_bytes() const noexcept { return _size; }
        [[nodiscard]] constexpr size_type code_unit_count() const noexcept { return _size; }
        [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }
        [[nodiscard]] constexpr bool is_valid() const noexcept { return _data != nullptr || _size == 0; }

        [[nodiscard]] constexpr std::string_view sv() const noexcept
        {
            return std::string_view(_data, _size);
        }

        [[nodiscard]] std::string to_string() const
        {
            return std::string(sv());
        }

        constexpr operator std::string_view() const noexcept
        {
            return sv();
        }

        // -----------------------------------------------------
        // iterators
        // -----------------------------------------------------

        [[nodiscard]] constexpr const_iterator begin() const noexcept { return _data; }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return _data + _size; }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return _data; }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return _data + _size; }

        // -----------------------------------------------------
        // byte access
        // -----------------------------------------------------

        [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept
        {
            return _data[index];
        }

        [[nodiscard]] constexpr const_reference front() const noexcept
        {
            return _data[0];
        }

        [[nodiscard]] constexpr const_reference back() const noexcept
        {
            return _data[_size - 1];
        }

        [[nodiscard]] constexpr const_reference byte_at(size_type index) const noexcept
        {
            return _data[index];
        }

        constexpr void reset() noexcept
        {
            _data = nullptr;
            _size = 0;
        }

        // -----------------------------------------------------
        // slicing (byte-based)
        // -----------------------------------------------------

        [[nodiscard]] constexpr STextView substr(size_type byte_pos, size_type byte_count = npos) const noexcept
        {
            if (byte_pos >= _size)
            {
                return {};
            }

            const size_type count = (byte_count == npos)
                ? (_size - byte_pos)
                : std::min(byte_count, _size - byte_pos);

            return STextView(_data + byte_pos, count);
        }

        [[nodiscard]] constexpr STextView first(size_type byte_count) const noexcept
        {
            return substr(0, byte_count);
        }

        [[nodiscard]] constexpr STextView last(size_type byte_count) const noexcept
        {
            if (byte_count >= _size)
            {
                return *this;
            }

            return substr(_size - byte_count, byte_count);
        }

        // -----------------------------------------------------
        // search (byte-based)
        // -----------------------------------------------------

        [[nodiscard]] size_type find(STextView pattern, size_type start = 0) const noexcept
        {
            if (pattern._size == 0)
            {
                return start <= _size ? start : npos;
            }

            if (start >= _size || pattern._size > (_size - start))
            {
                return npos;
            }

            const char first_char = pattern._data[0];
            const char* cur = _data + start;
            const char* const last = _data + (_size - pattern._size);

            while (cur <= last)
            {
                cur = static_cast<const char*>(
                    std::memchr(cur, first_char, static_cast<size_type>(last - cur + 1))
                );

                if (cur == nullptr)
                {
                    return npos;
                }

                if (std::memcmp(cur, pattern._data, pattern._size) == 0)
                {
                    return static_cast<size_type>(cur - _data);
                }

                ++cur;
            }

            return npos;
        }

        [[nodiscard]] size_type find(char ch, size_type start = 0) const noexcept
        {
            if (start >= _size)
            {
                return npos;
            }

            const char* p = static_cast<const char*>(
                std::memchr(_data + start, ch, _size - start)
            );

            return p ? static_cast<size_type>(p - _data) : npos;
        }

        [[nodiscard]] size_type rfind(char ch, size_type start = npos) const noexcept
        {
            if (_size == 0)
            {
                return npos;
            }

            size_type pos = (start == npos || start >= _size) ? (_size - 1) : start;
            while (true)
            {
                if (_data[pos] == ch)
                {
                    return pos;
                }

                if (pos == 0)
                {
                    break;
                }

                --pos;
            }

            return npos;
        }

        [[nodiscard]] size_type find_first_of(STextView chars, size_type start = 0) const noexcept
        {
            if (start >= _size || chars.empty())
            {
                return npos;
            }

            std::array<bool, 256> table{};
            for (size_type i = 0; i < chars._size; ++i)
            {
                table[static_cast<unsigned char>(chars._data[i])] = true;
            }

            for (size_type i = start; i < _size; ++i)
            {
                if (table[static_cast<unsigned char>(_data[i])])
                {
                    return i;
                }
            }

            return npos;
        }

        [[nodiscard]] size_type find_first_not_of(STextView chars, size_type start = 0) const noexcept
        {
            if (start >= _size)
            {
                return npos;
            }

            if (chars.empty())
            {
                return start;
            }

            std::array<bool, 256> table{};
            for (size_type i = 0; i < chars._size; ++i)
            {
                table[static_cast<unsigned char>(chars._data[i])] = true;
            }

            for (size_type i = start; i < _size; ++i)
            {
                if (!table[static_cast<unsigned char>(_data[i])])
                {
                    return i;
                }
            }

            return npos;
        }

        [[nodiscard]] size_type find_last_of(STextView chars, size_type start = npos) const noexcept
        {
            if (_size == 0 || chars.empty())
            {
                return npos;
            }

            std::array<bool, 256> table{};
            for (size_type i = 0; i < chars._size; ++i)
            {
                table[static_cast<unsigned char>(chars._data[i])] = true;
            }

            size_type pos = (start == npos || start >= _size) ? (_size - 1) : start;
            while (true)
            {
                if (table[static_cast<unsigned char>(_data[pos])])
                {
                    return pos;
                }

                if (pos == 0)
                {
                    break;
                }

                --pos;
            }

            return npos;
        }

        [[nodiscard]] size_type find_last_of(char ch, size_type start = npos) const noexcept
        {
            return rfind(ch, start);
        }

        [[nodiscard]] bool contains(STextView pattern) const noexcept
        {
            return find(pattern) != npos;
        }

        [[nodiscard]] bool contains(char ch) const noexcept
        {
            return find(ch) != npos;
        }

        // -----------------------------------------------------
        // prefix / suffix / comparison
        // -----------------------------------------------------

        [[nodiscard]] constexpr bool starts_with(STextView prefix) const noexcept
        {
            return prefix._size <= _size
                && std::memcmp(_data, prefix._data, prefix._size) == 0;
        }

        [[nodiscard]] constexpr bool ends_with(STextView suffix) const noexcept
        {
            return suffix._size <= _size
                && std::memcmp(_data + (_size - suffix._size), suffix._data, suffix._size) == 0;
        }

        [[nodiscard]] constexpr bool equals(STextView rhs) const noexcept
        {
            return _size == rhs._size
                && (_data == rhs._data || std::memcmp(_data, rhs._data, _size) == 0);
        }

        // -----------------------------------------------------
        // trim (byte-based, ASCII whitespace + UTF-8 NBSP)
        // -----------------------------------------------------

        [[nodiscard]] constexpr STextView trim_start() const noexcept
        {
            size_type start = 0;
            while (start < _size)
            {
                const unsigned char c = static_cast<unsigned char>(_data[start]);

                if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')
                {
                    ++start;
                }
                else if (c == 0xC2 && (start + 1) < _size &&
                         static_cast<unsigned char>(_data[start + 1]) == 0xA0)
                {
                    start += 2;
                }
                else
                {
                    break;
                }
            }

            return substr(start);
        }

        [[nodiscard]] constexpr STextView trim_end() const noexcept
        {
            size_type end_pos = _size;
            while (end_pos > 0)
            {
                const unsigned char c = static_cast<unsigned char>(_data[end_pos - 1]);

                if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')
                {
                    --end_pos;
                }
                else if (c == 0xA0 && end_pos >= 2 &&
                         static_cast<unsigned char>(_data[end_pos - 2]) == 0xC2)
                {
                    end_pos -= 2;
                }
                else
                {
                    break;
                }
            }

            return first(end_pos);
        }

        [[nodiscard]] constexpr STextView trim() const noexcept
        {
            return trim_start().trim_end();
        }

        // -----------------------------------------------------
        // UTF-8 code point helpers
        // -----------------------------------------------------

        [[nodiscard]] size_type code_point_count() const noexcept
        {
            size_type count = 0;
            const char* p = _data;
            const char* const e = _data + _size;

            while (p < e)
            {
                (void)_decode_code_point(p, e);
                ++count;
            }

            return count;
        }

        [[nodiscard]] size_type byte_index_from_code_point(size_type cp_index) const noexcept
        {
            size_type current = 0;
            const char* p = _data;
            const char* const e = _data + _size;

            while (p < e && current < cp_index)
            {
                (void)_decode_code_point(p, e);
                ++current;
            }

            return (current == cp_index) ? static_cast<size_type>(p - _data) : npos;
        }

        [[nodiscard]] STextView substr_code_points(size_type cp_pos, size_type cp_count = npos) const noexcept
        {
            const size_type start = byte_index_from_code_point(cp_pos);
            if (start == npos)
            {
                return {};
            }

            if (cp_count == npos)
            {
                return substr(start);
            }

            const char* p = _data + start;
            const char* const e = _data + _size;
            size_type remaining = cp_count;

            while (p < e && remaining > 0)
            {
                (void)_decode_code_point(p, e);
                --remaining;
            }

            return STextView(_data + start, static_cast<size_type>(p - (_data + start)));
        }

        [[nodiscard]] size_type find_code_point(char32_t cp, size_type byte_start = 0) const noexcept
        {
            if (byte_start >= _size)
            {
                return npos;
            }

            const char* p = _data + byte_start;
            const char* const e = _data + _size;

            while (p < e)
            {
                const char* current = p;
                if (_decode_code_point(p, e) == cp)
                {
                    return static_cast<size_type>(current - _data);
                }
            }

            return npos;
        }

        [[nodiscard]] bool contains_code_point(char32_t cp) const noexcept
        {
            return find_code_point(cp) != npos;
        }

        [[nodiscard]] int compare_code_points(STextView rhs) const noexcept
        {
            const char* p1 = _data;
            const char* const e1 = _data + _size;
            const char* p2 = rhs._data;
            const char* const e2 = rhs._data + rhs._size;

            while (p1 < e1 && p2 < e2)
            {
                const char32_t c1 = _decode_code_point(p1, e1);
                const char32_t c2 = _decode_code_point(p2, e2);

                if (c1 < c2) return -1;
                if (c1 > c2) return 1;
            }

            if (p1 == e1 && p2 == e2) return 0;
            return (p1 == e1) ? -1 : 1;
        }

        template <class F>
        constexpr void for_each_code_point(F&& fn) const
            noexcept(noexcept(std::declval<F&>()(std::declval<char32_t>())))
        {
            const char* p = _data;
            const char* const e = _data + _size;

            while (p < e)
            {
                fn(_decode_code_point(p, e));
            }
        }

        // -----------------------------------------------------
        // compatibility aliases
        // -----------------------------------------------------

        [[nodiscard]] size_type utf8_index_from_code_point(size_type cp_index) const noexcept
        {
            return byte_index_from_code_point(cp_index);
        }

        [[nodiscard]] STextView substr_cp(size_type cp_pos, size_type cp_count) const noexcept
        {
            return substr_code_points(cp_pos, cp_count);
        }

        [[nodiscard]] size_type find_cp(char32_t cp, size_type byte_start = 0) const noexcept
        {
            return find_code_point(cp, byte_start);
        }

        [[nodiscard]] int compare_cp(STextView rhs) const noexcept
        {
            return compare_code_points(rhs);
        }

    private:
        static constexpr char32_t kReplacementChar = 0xFFFD;

        [[nodiscard]] static constexpr char32_t _decode_code_point(const char*& p, const char* end) noexcept
        {
            if (p >= end)
            {
                return kReplacementChar;
            }

            const unsigned char b0 = static_cast<unsigned char>(*p);

            // 1-byte ASCII
            if (b0 < 0x80)
            {
                ++p;
                return static_cast<char32_t>(b0);
            }

            // invalid leading classes
            if (b0 < 0xC2)
            {
                ++p;
                return kReplacementChar;
            }

            // 2-byte sequence
            if (b0 < 0xE0)
            {
                if ((end - p) < 2)
                {
                    ++p;
                    return kReplacementChar;
                }

                const unsigned char b1 = static_cast<unsigned char>(p[1]);
                if ((b1 & 0xC0u) != 0x80u)
                {
                    ++p;
                    return kReplacementChar;
                }

                const char32_t cp =
                    (static_cast<char32_t>(b0 & 0x1Fu) << 6) |
                    static_cast<char32_t>(b1 & 0x3Fu);

                p += 2;
                return cp;
            }

            // 3-byte sequence
            if (b0 < 0xF0)
            {
                if ((end - p) < 3)
                {
                    ++p;
                    return kReplacementChar;
                }

                const unsigned char b1 = static_cast<unsigned char>(p[1]);
                const unsigned char b2 = static_cast<unsigned char>(p[2]);

                if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u)
                {
                    ++p;
                    return kReplacementChar;
                }

                if (b0 == 0xE0 && b1 < 0xA0)
                {
                    ++p;
                    return kReplacementChar;
                }

                if (b0 == 0xED && b1 >= 0xA0)
                {
                    ++p;
                    return kReplacementChar;
                }

                const char32_t cp =
                    (static_cast<char32_t>(b0 & 0x0Fu) << 12) |
                    (static_cast<char32_t>(b1 & 0x3Fu) << 6) |
                    static_cast<char32_t>(b2 & 0x3Fu);

                p += 3;
                return cp;
            }

            // 4-byte sequence
            if (b0 < 0xF5)
            {
                if ((end - p) < 4)
                {
                    ++p;
                    return kReplacementChar;
                }

                const unsigned char b1 = static_cast<unsigned char>(p[1]);
                const unsigned char b2 = static_cast<unsigned char>(p[2]);
                const unsigned char b3 = static_cast<unsigned char>(p[3]);

                if ((b1 & 0xC0u) != 0x80u || (b2 & 0xC0u) != 0x80u || (b3 & 0xC0u) != 0x80u)
                {
                    ++p;
                    return kReplacementChar;
                }

                if (b0 == 0xF0 && b1 < 0x90)
                {
                    ++p;
                    return kReplacementChar;
                }

                if (b0 == 0xF4 && b1 >= 0x90)
                {
                    ++p;
                    return kReplacementChar;
                }

                const char32_t cp =
                    (static_cast<char32_t>(b0 & 0x07u) << 18) |
                    (static_cast<char32_t>(b1 & 0x3Fu) << 12) |
                    (static_cast<char32_t>(b2 & 0x3Fu) << 6) |
                    static_cast<char32_t>(b3 & 0x3Fu);

                p += 4;
                return cp;
            }

            ++p;
            return kReplacementChar;
        }

    private:
        const char* _data = nullptr;
        size_type _size = 0;
    };

    static_assert(std::is_trivially_copyable_v<STextView>);
    static_assert(sizeof(STextView) == sizeof(std::string_view));

    [[nodiscard]] constexpr bool operator==(STextView lhs, STextView rhs) noexcept
    {
        return lhs.equals(rhs);
    }

    template <std::size_t N>
    [[nodiscard]] constexpr bool operator==(STextView lhs, const char (&rhs)[N]) noexcept
    {
        return lhs == STextView::from_literal(rhs);
    }

    template <std::size_t N>
    [[nodiscard]] constexpr bool operator==(const char (&lhs)[N], STextView rhs) noexcept
    {
        return STextView::from_literal(lhs) == rhs;
    }

    [[nodiscard]] inline bool operator==(STextView lhs, const char* rhs) noexcept
    {
        return lhs == STextView::from_cstring(rhs);
    }

    [[nodiscard]] inline bool operator==(const char* lhs, STextView rhs) noexcept
    {
        return STextView::from_cstring(lhs) == rhs;
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(STextView lhs, STextView rhs) noexcept
    {
        const int cmp = lhs.sv().compare(rhs.sv());
        if (cmp < 0) return std::strong_ordering::less;
        if (cmp > 0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }
}
