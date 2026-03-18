#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "shine_text_view.h"
#include "util/Algorithm/FNV1a.h"

// Portable lifetime-bound annotation: warns when a returned view outlives *this
#if defined(__clang__)
#  define SHINE_LIFETIMEBOUND [[clang::lifetimebound]]
#elif defined(_MSC_VER) && _MSC_VER >= 1929
#  define SHINE_LIFETIMEBOUND [[msvc::lifetimebound]]
#else
#  define SHINE_LIFETIMEBOUND
#endif

// Force-inline hint: critical for hot path helpers (_assign_raw/_assign_from)
#if defined(_MSC_VER)
#  define SHINE_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define SHINE_FORCE_INLINE __attribute__((always_inline)) inline
#else
#  define SHINE_FORCE_INLINE inline
#endif

namespace shine
{
    class SString
    {
    public:
        friend constexpr SString operator+(const SString& lhs, const SString& rhs);
        friend constexpr SString operator+(const SString& lhs, STextView rhs);
        friend constexpr SString operator+(STextView lhs, const SString& rhs);

        using value_type = char;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = char*;
        using const_pointer = const char*;
        using reference = char&;
        using const_reference = const char&;
        using iterator = char*;
        using const_iterator = const char*;

        static constexpr size_type npos = static_cast<size_type>(-1);

    private:
        static constexpr size_type kObjectSize = 32;
        static constexpr size_type kTagIndex = kObjectSize - 1;
        static constexpr unsigned char kHeapFlag = 0x80u;
        static constexpr size_type kSsoCapacity = kTagIndex - 1; // 30 bytes

        struct HeapStorage
        {
            char* ptr;
            size_type size;
            size_type cap;
        };

        union Storage
        {
            std::array<char, kObjectSize> sso;
            HeapStorage heap;

            constexpr Storage() noexcept : sso{} {}
        } _storage{};

    public:
        // ---------------------------------------------------------
        // construction / assignment / destruction
        // ---------------------------------------------------------

        constexpr SString() noexcept = default;

        constexpr explicit SString(size_type reserve_capacity)
        {
            if (reserve_capacity > kSsoCapacity)
            {
                _init_heap(reserve_capacity);
            }
        }

        constexpr SString(const char* cstr)
        {
            if (cstr != nullptr)
            {
                _assign_raw(cstr, _cstring_length(cstr));
            }
        }

        constexpr explicit SString(STextView view)
        {
            _assign_raw(view.data(), view.size());
        }

        constexpr explicit SString(std::string_view sv)
        {
            _assign_raw(sv.data(), sv.size());
        }

        explicit SString(const std::string& str)
        {
            _assign_raw(str.data(), str.size());
        }
        

        constexpr SString(const SString& other)
        {
            if (other._is_sso())
            {
                _copy_bytes(_storage.sso.data(), other._storage.sso.data(), kObjectSize);
            }
            else
            {
                _assign_raw(other._storage.heap.ptr, other._storage.heap.size);
            }
        }

        constexpr SString(SString&& other) noexcept
        {
            if (other._is_sso())
            {
                _copy_bytes(_storage.sso.data(), other._storage.sso.data(), kObjectSize);
            }
            else
            {
                _storage.heap.ptr = other._storage.heap.ptr;
                _storage.heap.size = other._storage.heap.size;
                _storage.heap.cap = other._storage.heap.cap;
                _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
                other._reset_to_empty_sso();
            }

            if (other._is_sso())
            {
                other._reset_to_empty_sso();
            }
        }

        constexpr ~SString()
        {
            if (!_is_sso())
            {
                delete[] _storage.heap.ptr;
            }
        }

        constexpr SString& operator=(const SString& other)
        {
            if (this != &other)
            {
                _assign_from(other.data(), other.size());
            }
            return *this;
        }

        constexpr SString& operator=(SString&& other) noexcept
        {
            if (this != &other)
            {
                if (!_is_sso())
                {
                    delete[] _storage.heap.ptr;
                }

                if (other._is_sso())
                {
                    _copy_bytes(_storage.sso.data(), other._storage.sso.data(), kObjectSize);
                    other._reset_to_empty_sso();
                }
                else
                {
                    _storage.heap.ptr = other._storage.heap.ptr;
                    _storage.heap.size = other._storage.heap.size;
                    _storage.heap.cap = other._storage.heap.cap;
                    _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
                    other._reset_to_empty_sso();
                }
            }
            return *this;
        }

        constexpr SString& operator=(STextView view)
        {
            _assign_from(view.data(), view.size());
            return *this;
        }

        constexpr SString& operator=(std::string_view sv)
        {
            _assign_from(sv.data(), sv.size());
            return *this;
        }

        SString& operator=(const std::string& str)
        {
            _assign_from(str.data(), str.size());
            return *this;
        }

        constexpr SString& operator=(const char* cstr)
        {
            if (cstr == nullptr)
            {
                clear();
            }
            else
            {
                _assign_from(cstr, _cstring_length(cstr));
            }
            return *this;
        }

        // ---------------------------------------------------------
        // views / conversion
        // ---------------------------------------------------------

        constexpr operator STextView() const noexcept SHINE_LIFETIMEBOUND
        {
            return { data(), size() };
        }

        [[nodiscard]] constexpr STextView view() const noexcept
        {
            return { data(), size() };
        }

        [[nodiscard]] constexpr STextView as_view() const noexcept
        {
            return view();
        }

        [[nodiscard]] constexpr std::string_view sv() const noexcept
        {
            return { data(), size() };
        }

        [[nodiscard]] std::string to_string() const
        {
            return std::string(data(), size());
        }

        // ---------------------------------------------------------
        // accessors
        // ---------------------------------------------------------

        [[nodiscard]] constexpr const char* data() const noexcept
        {
            if (_is_sso())
            {
                return _storage.sso.data();
            }

            return _storage.heap.ptr;
        }

        [[nodiscard]] constexpr char* data() noexcept
        {
            if (_is_sso())
            {
                return _storage.sso.data();
            }

            return _storage.heap.ptr;
        }

        [[nodiscard]] constexpr const char* c_str() const noexcept
        {
            return data();
        }

        [[nodiscard]] constexpr size_type size() const noexcept
        {
            if (_is_sso())
            {
                return static_cast<size_type>(static_cast<unsigned char>(_storage.sso[kTagIndex]));
            }

            return _storage.heap.size;
        }

        [[nodiscard]] constexpr size_type length() const noexcept
        {
            return size();
        }

        [[nodiscard]] constexpr size_type size_bytes() const noexcept
        {
            return size();
        }

        [[nodiscard]] constexpr size_type code_unit_count() const noexcept
        {
            return size();
        }

        [[nodiscard]] constexpr size_type code_point_count() const noexcept
        {
            return view().code_point_count();
        }

        [[nodiscard]] constexpr size_type capacity() const noexcept
        {
            if (_is_sso())
            {
                return kSsoCapacity;
            }

            return _storage.heap.cap;
        }

        [[nodiscard]] constexpr bool empty() const noexcept
        {
            return size() == 0;
        }

        // ---------------------------------------------------------
        // capacity management
        // ---------------------------------------------------------

        void reserve(size_type new_capacity)
        {
            if (new_capacity > capacity())
            {
                _grow_to(new_capacity);
            }
        }

        void shrink_to_fit()
        {
            if (_is_sso())
            {
                return;
            }

            const size_type len = _storage.heap.size;

            if (len <= kSsoCapacity)
            {
                char* old = _storage.heap.ptr;
                std::memcpy(_storage.sso.data(), old, len);
                _storage.sso[len] = '\0';
                _storage.sso[kTagIndex] = static_cast<char>(len);
                delete[] old;
                return;
            }

            if (_storage.heap.cap > len)
            {
                char* new_ptr = new char[len + 1];
                std::memcpy(new_ptr, _storage.heap.ptr, len + 1);
                delete[] _storage.heap.ptr;
                _storage.heap.ptr = new_ptr;
                _storage.heap.size = len;
                _storage.heap.cap = len;
                _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
            }
        }

        // ---------------------------------------------------------
        // mutation
        // ---------------------------------------------------------

        constexpr void clear() noexcept
        {
            if (_is_sso())
            {
                _storage.sso[0] = '\0';
                _storage.sso[kTagIndex] = 0;
            }
            else
            {
                _storage.heap.ptr[0] = '\0';
                _storage.heap.size = 0;
            }
        }

        constexpr void resize(size_type new_size, char fill = '\0')
        {
            const size_type old_size = size();

            if (new_size > capacity())
            {
                _grow_to(_next_capacity(new_size));
            }

            char* p = data();
            if (new_size > old_size)
            {
                _fill_bytes(p + old_size, fill, new_size - old_size);
            }

            p[new_size] = '\0';
            _set_size(new_size);
        }

        constexpr void push_back(char ch)
        {
            const size_type old_size = size();
            if (old_size >= capacity())
            {
                _grow_to(_next_capacity(old_size + 1));
            }

            char* p = data();
            p[old_size] = ch;
            p[old_size + 1] = '\0';
            _set_size(old_size + 1);
        }

        constexpr SString& append(STextView view_arg)
        {
            return _append_raw(view_arg.data(), view_arg.size());
        }

        SString& append(std::string_view sv_arg)
        {
            return _append_raw(sv_arg.data(), sv_arg.size());
        }

        SString& append(const std::string& str)
        {
            return _append_raw(str.data(), str.size());
        }

        constexpr SString& append(const char* cstr)
        {
            return append(STextView::from_cstring(cstr));
        }

        constexpr SString& append(char ch)
        {
            push_back(ch);
            return *this;
        }

        constexpr SString& operator+=(STextView view_arg) { return append(view_arg); }
        SString& operator+=(std::string_view sv_arg) { return append(sv_arg); }
        SString& operator+=(const std::string& str) { return append(str); }
        constexpr SString& operator+=(const char* cstr) { return append(STextView::from_cstring(cstr)); }
        constexpr SString& operator+=(char ch) { return append(ch); }

        constexpr SString& insert(size_type pos, STextView view_arg)
        {
            const size_type old_size = size();
            if (pos > old_size)
            {
                pos = old_size;
            }

            if (view_arg.empty())
            {
                return *this;
            }

            const size_type needed = old_size + view_arg.size();
            if (needed > capacity())
            {
                _grow_to(_next_capacity(needed));
            }

            char* p = data();
            _move_bytes(p + pos + view_arg.size(), p + pos, old_size - pos);
            _copy_bytes(p + pos, view_arg.data(), view_arg.size());
            p[needed] = '\0';
            _set_size(needed);
            return *this;
        }

        SString& insert(size_type pos, std::string_view sv_arg)
        {
            return insert(pos, STextView(sv_arg));
        }

        SString& insert(size_type pos, const std::string& str)
        {
            return insert(pos, STextView(str));
        }

        SString& insert(size_type pos, const char* cstr)
        {
            return insert(pos, STextView::from_cstring(cstr));
        }



        constexpr SString& erase(size_type pos, size_type count = npos)
        {
            const size_type old_size = size();
            if (pos >= old_size)
            {
                return *this;
            }

            count = std::min(count, old_size - pos);

            char* p = data();
            _move_bytes(p + pos, p + pos + count, old_size - pos - count);
            const size_type new_size = old_size - count;
            p[new_size] = '\0';
            _set_size(new_size);
            return *this;
        }

        // ---------------------------------------------------------
        // element access
        // ---------------------------------------------------------

        [[nodiscard]] reference operator[](size_type index) noexcept
        {
            return data()[index];
        }

        [[nodiscard]] const_reference operator[](size_type index) const noexcept
        {
            return data()[index];
        }

        [[nodiscard]] reference at(size_type index)
        {
            if (index >= size())
            {
                throw std::out_of_range("SString::at");
            }
            return data()[index];
        }

        [[nodiscard]] const_reference at(size_type index) const
        {
            if (index >= size())
            {
                throw std::out_of_range("SString::at");
            }
            return data()[index];
        }

        [[nodiscard]] reference front() noexcept
        {
            assert(!empty());
            return data()[0];
        }

        [[nodiscard]] const_reference front() const noexcept
        {
            assert(!empty());
            return data()[0];
        }

        [[nodiscard]] reference back() noexcept
        {
            assert(!empty());
            return data()[size() - 1];
        }

        [[nodiscard]] const_reference back() const noexcept
        {
            assert(!empty());
            return data()[size() - 1];
        }

        // ---------------------------------------------------------
        // iterators
        // ---------------------------------------------------------

        [[nodiscard]] iterator begin() noexcept { return data(); }
        [[nodiscard]] iterator end() noexcept { return data() + size(); }
        [[nodiscard]] const_iterator begin() const noexcept { return data(); }
        [[nodiscard]] const_iterator end() const noexcept { return data() + size(); }
        [[nodiscard]] const_iterator cbegin() const noexcept { return data(); }
        [[nodiscard]] const_iterator cend() const noexcept { return data() + size(); }

        // ---------------------------------------------------------
        // substr / search
        // ---------------------------------------------------------

        [[nodiscard]] SString substr(size_type pos, size_type count = npos) const
        {
            const size_type len = size();
            if (pos >= len)
            {
                return {};
            }

            count = std::min(count, len - pos);
            return SString(STextView(data() + pos, count));
        }

        [[nodiscard]] constexpr STextView subview(size_type pos, size_type count = npos) const noexcept
        {
            return view().substr(pos, count);
        }

        [[nodiscard]] constexpr size_type find(STextView pattern, size_type start = 0) const noexcept
        {
            return view().find(pattern, start);
        }

        [[nodiscard]] constexpr size_type find(const char* cstr, size_type start = 0) const noexcept
        {
            return find(STextView::from_cstring(cstr), start);
        }

        [[nodiscard]] constexpr size_type find(char ch, size_type start = 0) const noexcept
        {
            return view().find(ch, start);
        }

        [[nodiscard]] constexpr size_type rfind(char ch, size_type start = npos) const noexcept
        {
            return view().rfind(ch, start);
        }

        [[nodiscard]] constexpr size_type find_first_of(STextView chars, size_type start = 0) const noexcept
        {
            return view().find_first_of(chars, start);
        }

        [[nodiscard]] constexpr size_type find_first_of(const char* cstr, size_type start = 0) const noexcept
        {
            return find_first_of(STextView::from_cstring(cstr), start);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(STextView chars, size_type start = 0) const noexcept
        {
            return view().find_first_not_of(chars, start);
        }

        [[nodiscard]] constexpr size_type find_first_not_of(const char* cstr, size_type start = 0) const noexcept
        {
            return find_first_not_of(STextView::from_cstring(cstr), start);
        }

        [[nodiscard]] constexpr size_type find_last_of(STextView chars, size_type start = npos) const noexcept
        {
            return view().find_last_of(chars, start);
        }

        [[nodiscard]] constexpr size_type find_last_of(const char* cstr, size_type start = npos) const noexcept
        {
            return find_last_of(STextView::from_cstring(cstr), start);
        }

        [[nodiscard]] constexpr bool contains(STextView pattern) const noexcept
        {
            return view().contains(pattern);
        }

        [[nodiscard]] constexpr bool contains(const char* cstr) const noexcept
        {
            return contains(STextView::from_cstring(cstr));
        }

        [[nodiscard]] constexpr bool contains(char ch) const noexcept
        {
            return view().contains(ch);
        }

        [[nodiscard]] constexpr bool starts_with(STextView prefix) const noexcept
        {
            return view().starts_with(prefix);
        }

        [[nodiscard]] constexpr bool starts_with(const char* cstr) const noexcept
        {
            return starts_with(STextView::from_cstring(cstr));
        }

        [[nodiscard]] constexpr bool ends_with(STextView suffix) const noexcept
        {
            return view().ends_with(suffix);
        }

        [[nodiscard]] constexpr bool ends_with(const char* cstr) const noexcept
        {
            return ends_with(STextView::from_cstring(cstr));
        }



        [[nodiscard]] constexpr STextView trim() const noexcept
        {
            return view().trim();
        }

        [[nodiscard]] constexpr STextView trim_start() const noexcept
        {
            return view().trim_start();
        }

        [[nodiscard]] constexpr STextView trim_end() const noexcept
        {
            return view().trim_end();
        }

        // ---------------------------------------------------------
        // UTF-8 helpers
        // ---------------------------------------------------------

        [[nodiscard]] constexpr size_type byte_index_from_code_point(size_type cp_index) const noexcept
        {
            return view().byte_index_from_code_point(cp_index);
        }

        [[nodiscard]] constexpr STextView substr_code_points(size_type cp_pos, size_type cp_count = npos) const noexcept
        {
            return view().substr_code_points(cp_pos, cp_count);
        }

        [[nodiscard]] constexpr size_type find_code_point(char32_t cp, size_type byte_start = 0) const noexcept
        {
            return view().find_code_point(cp, byte_start);
        }

        [[nodiscard]] constexpr bool contains_code_point(char32_t cp) const noexcept
        {
            return view().contains_code_point(cp);
        }

        [[nodiscard]] constexpr int compare_code_points(STextView rhs) const noexcept
        {
            return view().compare_code_points(rhs);
        }

        template <class F>
        constexpr void for_each_code_point(F&& fn) const
            noexcept(noexcept(std::declval<STextView>().for_each_code_point(std::forward<F>(fn))))
        {
            view().for_each_code_point(std::forward<F>(fn));
        }

        // ---------------------------------------------------------
        // replace
        // ---------------------------------------------------------

        [[nodiscard]] constexpr SString replace(STextView from, STextView to) const
        {
            if (from.empty())
            {
                return *this;
            }

            const size_type from_len = from.size();
            const size_type to_len = to.size();
            const size_type src_len = size();
            const char* src = data();

            size_type count = 0;
            size_type pos = 0;
            while ((pos = find(from, pos)) != npos)
            {
                ++count;
                pos += from_len;
            }

            if (count == 0)
            {
                return *this;
            }

            const size_type new_size = src_len + count * (to_len - from_len);
            SString result(new_size);

            char* dst = result.data();
            size_type src_pos = 0;
            size_type dst_pos = 0;
            pos = 0;

            while ((pos = find(from, pos)) != npos)
            {
                const size_type copy_len = pos - src_pos;
                if (copy_len > 0)
                {
                    _copy_bytes(dst + dst_pos, src + src_pos, copy_len);
                    dst_pos += copy_len;
                }

                if (to_len > 0)
                {
                    _copy_bytes(dst + dst_pos, to.data(), to_len);
                    dst_pos += to_len;
                }

                src_pos = pos + from_len;
                pos = src_pos;
            }

            if (src_pos < src_len)
            {
                const size_type tail_len = src_len - src_pos;
                _copy_bytes(dst + dst_pos, src + src_pos, tail_len);
                dst_pos += tail_len;
            }

            dst[new_size] = '\0';
            result._set_size(new_size);
            return result;
        }

        [[nodiscard]] SString replace(std::string_view from, std::string_view to) const
        {
            return replace(STextView(from), STextView(to));
        }

        [[nodiscard]] SString replace(const std::string& from, const std::string& to) const
        {
            return replace(STextView(from), STextView(to));
        }

        [[nodiscard]] constexpr SString replace(const char* from, const char* to) const
        {
            return replace(STextView::from_cstring(from), STextView::from_cstring(to));
        }

        constexpr bool replace_first(STextView from, STextView to)
        {
            if (from.empty())
            {
                return false;
            }

            const size_type pos = find(from);
            if (pos == npos)
            {
                return false;
            }

            _replace_range(pos, from.size(), to);
            return true;
        }

        bool replace_first(std::string_view from, std::string_view to)
        {
            return replace_first(STextView(from), STextView(to));
        }

        bool replace_first(const std::string& from, const std::string& to)
        {
            return replace_first(STextView(from), STextView(to));
        }

        constexpr bool replace_first(const char* from, const char* to)
        {
            return replace_first(STextView::from_cstring(from), STextView::from_cstring(to));
        }

        constexpr void replace_inplace(STextView from, STextView to)
        {
            if (from.empty() || from == to)
            {
                return;
            }

            const size_type from_len = from.size();
            const size_type to_len = to.size();

            size_type first_pos = find(from);
            if (first_pos == npos)
            {
                return;
            }

            if (to_len <= from_len)
            {
                char* p = data();
                const size_type old_size = size();

                size_type read_pos = 0;
                size_type write_pos = 0;
                size_type match_pos = first_pos;

                while (match_pos != npos)
                {
                    const size_type chunk = match_pos - read_pos;
                    if (chunk > 0)
                    {
                        _move_bytes(p + write_pos, p + read_pos, chunk);
                        write_pos += chunk;
                    }

                    if (to_len > 0)
                    {
                        _copy_bytes(p + write_pos, to.data(), to_len);
                        write_pos += to_len;
                    }

                    read_pos = match_pos + from_len;
                    match_pos = find(from, read_pos);
                }

                if (read_pos < old_size)
                {
                    const size_type tail = old_size - read_pos;
                    _move_bytes(p + write_pos, p + read_pos, tail);
                    write_pos += tail;
                }

                p[write_pos] = '\0';
                _set_size(write_pos);
                return;
            }

            size_type count = 1;
            size_type search_pos = first_pos + from_len;
            while ((search_pos = find(from, search_pos)) != npos)
            {
                ++count;
                search_pos += from_len;
            }

            const size_type old_size = size();
            const size_type new_size = old_size + count * (to_len - from_len);

            if (new_size <= kSsoCapacity)
            {
                SString result(new_size);
                char* result_ptr = result.data();
                const char* old_ptr = data();
                size_type src_pos = 0;
                size_type dst_pos = 0;
                size_type cur = first_pos;

                while (cur != npos)
                {
                    const size_type chunk = cur - src_pos;
                    if (chunk > 0)
                    {
                        _copy_bytes(result_ptr + dst_pos, old_ptr + src_pos, chunk);
                        dst_pos += chunk;
                    }

                    _copy_bytes(result_ptr + dst_pos, to.data(), to_len);
                    dst_pos += to_len;

                    src_pos = cur + from_len;
                    cur = find(from, src_pos);
                }

                if (src_pos < old_size)
                {
                    const size_type tail = old_size - src_pos;
                    _copy_bytes(result_ptr + dst_pos, old_ptr + src_pos, tail);
                    dst_pos += tail;
                }

                result_ptr[new_size] = '\0';
                result._set_size(new_size);
                *this = std::move(result);
                return;
            }

            char* new_ptr = new char[new_size + 1];

            const char* old_ptr = data();
            size_type src_pos = 0;
            size_type dst_pos = 0;
            size_type cur = first_pos;

            while (cur != npos)
            {
                const size_type chunk = cur - src_pos;
                if (chunk > 0)
                {
                    _copy_bytes(new_ptr + dst_pos, old_ptr + src_pos, chunk);
                    dst_pos += chunk;
                }

                _copy_bytes(new_ptr + dst_pos, to.data(), to_len);
                dst_pos += to_len;

                src_pos = cur + from_len;
                cur = find(from, src_pos);
            }

            if (src_pos < old_size)
            {
                const size_type tail = old_size - src_pos;
                _copy_bytes(new_ptr + dst_pos, old_ptr + src_pos, tail);
                dst_pos += tail;
            }

            new_ptr[new_size] = '\0';

            if (!_is_sso())
            {
                delete[] _storage.heap.ptr;
            }

            _storage.heap.ptr = new_ptr;
            _storage.heap.size = new_size;
            _storage.heap.cap = new_size;
            _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
        }

        void replace_inplace(std::string_view from, std::string_view to)
        {
            replace_inplace(STextView(from), STextView(to));
        }

        void replace_inplace(const std::string& from, const std::string& to)
        {
            replace_inplace(STextView(from), STextView(to));
        }

        constexpr void replace_inplace(const char* from, const char* to)
        {
            replace_inplace(STextView::from_cstring(from), STextView::from_cstring(to));
        }

        // ---------------------------------------------------------
        // hashing / factories
        // ---------------------------------------------------------

        [[nodiscard]] constexpr size_type hash() const noexcept
        {
            // Use our own FNV-1a directly — avoids std::hash<string_view> dispatch,
            // and stays consistent with static_hash().
            return static_hash(sv());
        }

        [[nodiscard]] static constexpr size_type static_hash(std::string_view sv_arg) noexcept
        {
            return static_cast<size_type>(shine::algorithm::hash64(sv_arg));
        }

        [[nodiscard]] static constexpr SString from_view(STextView view_arg)
        {
            return SString(view_arg);
        }

        [[nodiscard]] static constexpr SString from_utf8(STextView view_arg)
        {
            return SString(view_arg);
        }

        [[nodiscard]] static constexpr SString from_utf8(std::string_view sv_arg)
        {
            return SString(STextView(sv_arg));
        }

        [[nodiscard]] static SString from_utf8(const std::string& str)
        {
            return SString(STextView(str));
        }

        [[nodiscard]] static constexpr SString from_utf8(const char* cstr)
        {
            return SString(STextView::from_cstring(cstr));
        }

    private:
        // ---------------------------------------------------------
        // internal helpers
        // ---------------------------------------------------------

        [[nodiscard]] constexpr bool _is_sso() const noexcept
        {
            return (static_cast<unsigned char>(_storage.sso[kTagIndex]) & kHeapFlag) == 0;
        }

        [[nodiscard]] static constexpr size_type _cstring_length(const char* src) noexcept
        {
            if (src == nullptr)
            {
                return 0;
            }

            size_type len = 0;
            while (src[len] != '\0')
            {
                ++len;
            }

            return len;
        }

        static constexpr void _constexpr_capacity_guard(size_type len)
        {
            if (std::is_constant_evaluated() && len > kSsoCapacity)
            {
                throw "SString constexpr path exceeds SSO capacity";
            }
        }

        static constexpr void _copy_bytes(char* dst, const char* src, size_type len) noexcept
        {
            if (!std::is_constant_evaluated())
            {
                std::memcpy(dst, src, len);
                return;
            }

            for (size_type i = 0; i < len; ++i)
            {
                dst[i] = src[i];
            }
        }

        static constexpr void _move_bytes(char* dst, const char* src, size_type len) noexcept
        {
            if (!std::is_constant_evaluated())
            {
                std::memmove(dst, src, len);
                return;
            }

            if (dst == src || len == 0)
            {
                return;
            }

            if (dst < src)
            {
                for (size_type i = 0; i < len; ++i)
                {
                    dst[i] = src[i];
                }
                return;
            }

            for (size_type i = len; i > 0; --i)
            {
                dst[i - 1] = src[i - 1];
            }
        }

        static constexpr void _fill_bytes(char* dst, char value, size_type len) noexcept
        {
            if (!std::is_constant_evaluated())
            {
                std::memset(dst, static_cast<unsigned char>(value), len);
                return;
            }

            for (size_type i = 0; i < len; ++i)
            {
                dst[i] = value;
            }
        }

        constexpr void _reset_to_empty_sso() noexcept
        {
            _storage.sso[0] = '\0';
            _storage.sso[kTagIndex] = 0;
        }

        constexpr void _set_size(size_type n) noexcept
        {
            if (_is_sso())
            {
                _storage.sso[kTagIndex] = static_cast<char>(n);
            }
            else
            {
                _storage.heap.size = n;
            }
        }

        [[nodiscard]] static constexpr size_type _next_capacity(size_type min_needed) noexcept
        {
            size_type cap = (min_needed <= kSsoCapacity) ? kSsoCapacity : 32;
            while (cap < min_needed)
            {
                const size_type grown = cap + (cap >> 1);
                cap = (grown > cap) ? grown : min_needed;
            }
            return cap;
        }

        constexpr void _init_heap(size_type cap)
        {
            _constexpr_capacity_guard(cap);
            _storage.heap.ptr = new char[cap + 1];
            _storage.heap.ptr[0] = '\0';
            _storage.heap.size = 0;
            _storage.heap.cap = cap;
            _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
        }

        constexpr void _grow_to(size_type new_cap)
        {
            _constexpr_capacity_guard(new_cap);
            const size_type old_size = size();
            const char* old_data = data();

            char* new_ptr = new char[new_cap + 1];
            if (old_size > 0)
            {
                _copy_bytes(new_ptr, old_data, old_size);
            }
            new_ptr[old_size] = '\0';

            if (!_is_sso())
            {
                delete[] _storage.heap.ptr;
            }

            _storage.heap.ptr = new_ptr;
            _storage.heap.size = old_size;
            _storage.heap.cap = new_cap;
            _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
        }

        constexpr SString& _append_raw(const char* src, size_type len)
        {
            if (src == nullptr || len == 0)
            {
                return *this;
            }

            const size_type old_size = size();
            const size_type needed = old_size + len;

            const char* old_data = data();
            const bool overlaps =
                (src >= old_data) && (src < old_data + old_size);

            size_type offset = 0;
            if (overlaps)
            {
                offset = static_cast<size_type>(src - old_data);
            }

            if (needed > capacity())
            {
                _grow_to(_next_capacity(needed));
            }

            if (overlaps)
            {
                src = data() + offset;
            }

            char* dst = data();
            _copy_bytes(dst + old_size, src, len);
            dst[needed] = '\0';
            _set_size(needed);
            return *this;
        }

        [[nodiscard]] static constexpr SString _concat_raw(
            const char* lhs_data, size_type lhs_size,
            const char* rhs_data, size_type rhs_size)
        {
            const size_type total = lhs_size + rhs_size;
            SString result(total);

            char* dst = result.data();
            if (lhs_size > 0)
            {
                _copy_bytes(dst, lhs_data, lhs_size);
            }
            if (rhs_size > 0)
            {
                _copy_bytes(dst + lhs_size, rhs_data, rhs_size);
            }

            dst[total] = '\0';
            result._set_size(total);
            return result;
        }

        constexpr SHINE_FORCE_INLINE void _assign_raw(const char* src, size_type len)
        {
            if (src == nullptr || len == 0)
            {
                _reset_to_empty_sso();
                return;
            }

            if (len <= kSsoCapacity)
            {
                _copy_bytes(_storage.sso.data(), src, len);
                _storage.sso[len] = '\0';
                _storage.sso[kTagIndex] = static_cast<char>(len);
                return;
            }

            _constexpr_capacity_guard(len);

            _storage.heap.ptr = new char[len + 1];
            _copy_bytes(_storage.heap.ptr, src, len);
            _storage.heap.ptr[len] = '\0';
            _storage.heap.size = len;
            _storage.heap.cap = len;
            _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
        }

        constexpr SHINE_FORCE_INLINE void _assign_from(const char* src, size_type len)
        {
            if (src == nullptr || len == 0)
            {
                if (!_is_sso())
                {
                    delete[] _storage.heap.ptr;
                }
                _reset_to_empty_sso();
                return;
            }

            const char* current = data();
            const size_type current_size = size();
            const bool overlaps = src >= current && src < (current + current_size);

            if (len <= kSsoCapacity)
            {
                char* old_heap = _is_sso() ? nullptr : _storage.heap.ptr;
                if (overlaps)
                {
                    _move_bytes(_storage.sso.data(), src, len);
                }
                else
                {
                    _copy_bytes(_storage.sso.data(), src, len);
                }
                _storage.sso[len] = '\0';
                _storage.sso[kTagIndex] = static_cast<char>(len);
                delete[] old_heap;
                return;
            }

            _constexpr_capacity_guard(len);

            if (!_is_sso() && _storage.heap.cap >= len)
            {
                if (overlaps)
                {
                    _move_bytes(_storage.heap.ptr, src, len);
                }
                else
                {
                    _copy_bytes(_storage.heap.ptr, src, len);
                }
                _storage.heap.ptr[len] = '\0';
                _storage.heap.size = len;
                return;
            }

            char* new_ptr = new char[len + 1];
            _copy_bytes(new_ptr, src, len);
            new_ptr[len] = '\0';

            if (!_is_sso())
            {
                delete[] _storage.heap.ptr;
            }

            _storage.heap.ptr = new_ptr;
            _storage.heap.size = len;
            _storage.heap.cap = len;
            _storage.sso[kTagIndex] = static_cast<char>(kHeapFlag);
        }

        constexpr void _replace_range(size_type pos, size_type old_len, STextView replacement)
        {
            const size_type rep_len = replacement.size();
            const size_type cur_size = size();

            if (old_len == rep_len)
            {
                _copy_bytes(data() + pos, replacement.data(), rep_len);
                return;
            }

            if (rep_len < old_len)
            {
                char* p = data();
                _copy_bytes(p + pos, replacement.data(), rep_len);
                _move_bytes(
                    p + pos + rep_len,
                    p + pos + old_len,
                    cur_size - (pos + old_len));
                const size_type new_size = cur_size - (old_len - rep_len);
                p[new_size] = '\0';
                _set_size(new_size);
                return;
            }

            const size_type grow_by = rep_len - old_len;
            const size_type new_size = cur_size + grow_by;

            if (new_size > capacity())
            {
                _grow_to(_next_capacity(new_size));
            }

            char* p = data();
            _move_bytes(
                p + pos + rep_len,
                p + pos + old_len,
                cur_size - (pos + old_len));
            _copy_bytes(p + pos, replacement.data(), rep_len);
            p[new_size] = '\0';
            _set_size(new_size);
        }
    };

    static_assert(sizeof(SString) == 32, "SString must remain 32 bytes");
    static_assert(std::is_nothrow_move_constructible_v<SString>);
    static_assert(std::is_nothrow_move_assignable_v<SString>);

    [[nodiscard]] constexpr SString operator+(const SString& lhs, const SString& rhs)
    {
        return SString::_concat_raw(lhs.data(), lhs.size(), rhs.data(), rhs.size());
    }

    [[nodiscard]] constexpr SString operator+(const SString& lhs, STextView rhs)
    {
        return SString::_concat_raw(lhs.data(), lhs.size(), rhs.data(), rhs.size());
    }

    [[nodiscard]] constexpr SString operator+(STextView lhs, const SString& rhs)
    {
        return SString::_concat_raw(lhs.data(), lhs.size(), rhs.data(), rhs.size());
    }

    [[nodiscard]] constexpr SString operator+(const SString& lhs, const char* rhs)
    {
        return lhs + STextView::from_cstring(rhs);
    }

    [[nodiscard]] constexpr SString operator+(const char* lhs, const SString& rhs)
    {
        return STextView::from_cstring(lhs) + rhs;
    }

    [[nodiscard]] constexpr bool operator==(const SString& lhs, const SString& rhs) noexcept
    {
        return lhs.view() == rhs.view();
    }

    [[nodiscard]] constexpr bool operator==(const SString& lhs, std::string_view rhs) noexcept
    {
        return lhs.view() == STextView(rhs);
    }

    [[nodiscard]] constexpr bool operator==(std::string_view lhs, const SString& rhs) noexcept
    {
        return STextView(lhs) == rhs.view();
    }

    [[nodiscard]] constexpr bool operator==(const SString& lhs, STextView rhs) noexcept
    {
        return lhs.view() == rhs;
    }

    [[nodiscard]] constexpr bool operator==(STextView lhs, const SString& rhs) noexcept
    {
        return lhs == rhs.view();
    }

    [[nodiscard]] constexpr bool operator==(const SString& lhs, const char* rhs) noexcept
    {
        return lhs.view() == STextView::from_cstring(rhs);
    }

    [[nodiscard]] constexpr std::strong_ordering operator<=>(const SString& lhs, const SString& rhs) noexcept
    {
        return lhs.view() <=> rhs.view();
    }

    static_assert(SString("shine").size() == 5);
    static_assert(SString("shine").starts_with("sh"));
    static_assert((SString("ab") + SString("cd")) == "abcd");
    static_assert([]() constexpr { SString s("abc"); s.push_back('d'); return s == "abcd"; }());
    static_assert([]() constexpr { SString s("abcd"); s.erase(1, 2); return s == "ad"; }());
    static_assert(SString("abcabc").replace("ab", "xy") == "xycxyc");
    static_assert([]() constexpr { SString s("abcabc"); s.replace_first("ab", "z"); return s == "zcabc"; }());
    static_assert([]() constexpr { SString s("abcabc"); s.replace_inplace("ab", "z"); return s == "zczc"; }());
    static_assert([]() constexpr { SString s("axxb"); s.replace_inplace("xx", "123"); return s == "a123b"; }());
}

template <>
struct std::hash<shine::SString>
{
    std::size_t operator()(const shine::SString& s) const noexcept
    {
        // Consistent with SString::hash() and static_hash() — single FNV-1a path.
        return shine::SString::static_hash(std::string_view(s.data(), s.size()));
    }
};

#if defined(__cpp_lib_format)
#include <format>
template <>
struct std::formatter<shine::SString, char> : std::formatter<std::string_view, char>
{
    auto format(const shine::SString& s, std::format_context& ctx) const
    {
        // Use data()+size() directly — avoids the sv() call overhead.
        return std::formatter<std::string_view, char>::format(
            std::string_view(s.data(), s.size()), ctx);
    }
};
#endif

#if __has_include("fmt/format.h")
#include "fmt/format.h"
template <>
struct fmt::formatter<shine::SString> : fmt::formatter<std::string_view>
{
    auto format(const shine::SString& s, fmt::format_context& ctx) const
    {
        return fmt::formatter<std::string_view>::format(
            std::string_view(s.data(), s.size()), ctx);
    }
};
#endif
