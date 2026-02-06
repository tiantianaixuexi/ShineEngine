#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdexcept>
#include <compare>
#include <format>
#include <cassert>
#include <new>

#include "shine_define.h"
#include "shine_text_view.h"

namespace shine
{
    // =========================================================
    // SString: Mutable, owning UTF-8 string with SSO
    //
    // Optimizations over std::string for game engines (C++23, MSVC):
    //   - 32-byte tagged-union SSO: same size as std::string,
    //     but 30-char SSO capacity (MSVC std::string = 15)
    //   - Discriminator at byte 31: < 0x80 = SSO, >= 0x80 = Heap
    //   - FNV-1a hash with constexpr static_hash() for compile-time
    //   - In-place replace without repeated allocations
    //   - Bitmap-accelerated find_first_of / find_last_of
    //   - std::formatter / std::hash integration (C++23)
    //
    // Layout (32 bytes total):
    //   SSO:  sso[0..29]=data, sso[len]='\0', sso[31]=length  (0..30)
    //   Heap: heap.ptr(8) + heap.size(8) + heap.cap(8), sso[31]=0x80
    //   Accessing sso[31] after writing to heap members relies on
    //   union type punning, which is guaranteed on MSVC.
    // =========================================================
    class SString
    {
    public:
        using iterator = char*;
        using const_iterator = const char*;

        static constexpr size_t kBufSize = 32;
        static constexpr size_t kTagIdx  = kBufSize - 1;        // byte 31
        static constexpr size_t kSsoMax  = kTagIdx - 1;         // 30 chars + '\0' at [30] + tag at [31]
        static constexpr size_t npos     = static_cast<size_t>(-1);

        // ---------------------------------------------------------
        // Constructors / Destructor
        // ---------------------------------------------------------

        constexpr SString() noexcept : _u{} {}
        // _u{} zero-inits: sso[0]='\0', sso[31]=0 → SSO length 0

        explicit SString(size_t reserve_cap) : _u{} {
            if (reserve_cap > kSsoMax) {
                _init_heap(reserve_cap);
            }
        }

        SString(const char* s) : _u{} {
            if (s) {
                _assign_raw(s, std::strlen(s));
            }
        }

        explicit SString(std::string_view sv) : _u{} {
            _assign_raw(sv.data(), sv.size());
        }

        explicit SString(STextView tv) : _u{} {
            _assign_raw(tv.data(), tv.size());
        }

        ~SString() {
            if (!_is_sso()) {
                delete[] _u.heap.ptr;
            }
        }

        // ---------------------------------------------------------
        // Copy  (no noexcept — heap allocation can throw)
        // ---------------------------------------------------------

        SString(const SString& other) : _u{} {
            if (other._is_sso()) {
                std::memcpy(&_u, &other._u, kBufSize);   // 32-byte flat copy
            } else {
                _assign_raw(other._u.heap.ptr, other._u.heap.size);
            }
        }

        SString& operator=(const SString& other) {
            if (this != &other) {
                _assign_from(other.data(), other.size());
            }
            return *this;
        }

        // ---------------------------------------------------------
        // Move
        // ---------------------------------------------------------

        SString(SString&& other) noexcept {
            std::memcpy(&_u, &other._u, kBufSize);       // steal everything
            other._u.sso[0] = 0;
            other._u.sso[kTagIdx] = 0;                   // reset source → empty SSO
        }

        SString& operator=(SString&& other) noexcept {
            if (this != &other) {
                if (!_is_sso()) delete[] _u.heap.ptr;
                std::memcpy(&_u, &other._u, kBufSize);
                other._u.sso[0] = 0;
                other._u.sso[kTagIdx] = 0;
            }
            return *this;
        }

        // ---------------------------------------------------------
        // Conversions
        // ---------------------------------------------------------

        operator STextView() const noexcept { return STextView(data(), size()); }

        [[nodiscard]] STextView          view()      const noexcept { return STextView(data(), size()); }
        [[nodiscard]] std::string_view    sv()        const noexcept { return {data(), size()}; }

        [[nodiscard]] std::string to_utf8()   const { return std::string(data(), size()); }
        [[nodiscard]] std::string to_string() const { return std::string(data(), size()); }

        // ---------------------------------------------------------
        // Accessors
        // ---------------------------------------------------------

        [[nodiscard]] const char* data()     const noexcept { return _is_sso() ? _u.sso : _u.heap.ptr; }
        [[nodiscard]] char*       data()           noexcept { return _is_sso() ? _u.sso : _u.heap.ptr; }
        [[nodiscard]] size_t      size()     const noexcept { return _is_sso() ? _sso_len() : _u.heap.size; }
        [[nodiscard]] size_t      length()   const noexcept { return size(); }
        [[nodiscard]] size_t      capacity() const noexcept { return _is_sso() ? kSsoMax : _u.heap.cap; }
        [[nodiscard]] const char* c_str()    const noexcept { return data(); }
        [[nodiscard]] bool        empty()    const noexcept { return size() == 0; }

        void clear() noexcept {
            if (_is_sso()) {
                _u.sso[0] = 0;
                _u.sso[kTagIdx] = 0;
            } else {
                _u.heap.ptr[0] = 0;
                _u.heap.size = 0;
            }
        }

        [[nodiscard]] size_t code_unit_count()  const noexcept { return size(); }
        [[nodiscard]] size_t code_point_count() const noexcept { return view().code_point_count(); }

        // ---------------------------------------------------------
        // FNV-1a Hash
        // ---------------------------------------------------------

        [[nodiscard]] size_t hash() const noexcept {
            return static_hash(sv());
        }

        // Compile-time FNV-1a hash (C++23 constexpr)
        [[nodiscard]] static constexpr size_t static_hash(std::string_view s) noexcept {
            constexpr size_t kFnvOffset = 14695981039346656037ULL;
            constexpr size_t kFnvPrime  = 1099511628211ULL;
            size_t h = kFnvOffset;
            for (size_t i = 0; i < s.size(); ++i) {
                h ^= static_cast<unsigned char>(s[i]);
                h *= kFnvPrime;
            }
            return h;
        }

        // ---------------------------------------------------------
        // Capacity Management
        // ---------------------------------------------------------

        void reserve(size_t new_cap) {
            if (new_cap <= capacity()) return;
            _grow_to(new_cap);
        }

        void shrink_to_fit() {
            if (_is_sso()) return;
            const size_t len = _u.heap.size;
            if (len <= kSsoMax) {
                // Move back to SSO
                char* old = _u.heap.ptr;
                std::memcpy(_u.sso, old, len);
                _u.sso[len] = 0;
                _u.sso[kTagIdx] = static_cast<char>(len);
                delete[] old;
            } else if (_u.heap.cap > len) {
                char* new_p = new (std::nothrow) char[len + 1];
                if (new_p) {
                    std::memcpy(new_p, _u.heap.ptr, len + 1);
                    delete[] _u.heap.ptr;
                    _u.heap.ptr = new_p;
                    _u.heap.cap = len;
                }
            }
        }

        // ---------------------------------------------------------
        // Mutation
        // ---------------------------------------------------------

        void push_back(char c) {
            const size_t sz = size();
            if (sz >= capacity()) [[unlikely]] {
                _grow_to(capacity() < 16 ? 32 : capacity() * 2);
            }
            char* p = data();
            p[sz] = c;
            p[sz + 1] = 0;
            _set_size(sz + 1);
        }

        SString& append(std::string_view sv_arg) {
            const size_t sz    = size();
            const size_t sv_sz = sv_arg.size();
            if (sv_sz == 0) return *this;
            const size_t needed = sz + sv_sz;
            if (needed > capacity()) [[unlikely]] {
                // Handle potential aliasing (self-append)
                const char*  d   = data();
                const ptrdiff_t off = sv_arg.data() - d;
                _grow_to(std::max(needed, capacity() * 2));
                if (off >= 0 && static_cast<size_t>(off) <= sz) {
                    sv_arg = {data() + off, sv_sz};
                }
            }
            char* p = data();
            std::memcpy(p + sz, sv_arg.data(), sv_sz);
            p[needed] = 0;
            _set_size(needed);
            return *this;
        }

        SString& append(STextView tv)    { return append(std::string_view(tv.data(), tv.size())); }
        SString& append(const char* s)   { return append(std::string_view(s)); }
        SString& append(char c)          { push_back(c); return *this; }

        SString& operator+=(std::string_view sv_arg) { return append(sv_arg); }
        SString& operator+=(STextView tv)            { return append(tv); }
        SString& operator+=(char c)                  { push_back(c); return *this; }

        void resize(size_t new_size, char c = 0) {
            const size_t sz = size();
            if (new_size > capacity()) {
                _grow_to(new_size);
            }
            if (new_size > sz) {
                std::memset(data() + sz, c, new_size - sz);
            }
            data()[new_size] = 0;
            _set_size(new_size);
        }

        // ---------------------------------------------------------
        // Element Access
        // ---------------------------------------------------------

        [[nodiscard]] char&       operator[](size_t i)       noexcept { return data()[i]; }
        [[nodiscard]] const char& operator[](size_t i) const noexcept { return data()[i]; }

        char& at(size_t i) {
            if (i >= size()) [[unlikely]] throw std::out_of_range("SString::at");
            return data()[i];
        }
        const char& at(size_t i) const {
            if (i >= size()) [[unlikely]] throw std::out_of_range("SString::at");
            return data()[i];
        }

        [[nodiscard]] char&       front()       noexcept { return data()[0]; }
        [[nodiscard]] const char& front() const noexcept { return data()[0]; }
        [[nodiscard]] char&       back()        noexcept { return data()[size() - 1]; }
        [[nodiscard]] const char& back()  const noexcept { return data()[size() - 1]; }

        // ---------------------------------------------------------
        // Substring
        // ---------------------------------------------------------

        [[nodiscard]] SString substr(size_t pos, size_t count = npos) const {
            const size_t sz = size();
            if (pos >= sz) return SString();
            count = std::min(count, sz - pos);
            return SString(std::string_view(data() + pos, count));
        }

        // ---------------------------------------------------------
        // Find (memchr-accelerated)
        // ---------------------------------------------------------

        [[nodiscard]] size_t find(STextView pattern, size_t start = 0) const noexcept {
            const size_t sz = size();
            if (start >= sz) return npos;
            if (pattern.empty()) return start;
            const size_t pat_len = pattern.size();
            if (pat_len > sz - start) return npos;

            const char* p     = data();
            const char  first = pattern.data()[0];
            const char* cur   = p + start;
            const char* end   = p + sz - pat_len;

            while (cur <= end) {
                cur = static_cast<const char*>(std::memchr(cur, first, static_cast<size_t>(end - cur + 1)));
                if (!cur) return npos;
                if (std::memcmp(cur, pattern.data(), pat_len) == 0) {
                    return static_cast<size_t>(cur - p);
                }
                ++cur;
            }
            return npos;
        }

        [[nodiscard]] size_t find(char c, size_t start = 0) const noexcept {
            const size_t sz = size();
            if (start >= sz) return npos;
            const char* p = static_cast<const char*>(std::memchr(data() + start, c, sz - start));
            return p ? static_cast<size_t>(p - data()) : npos;
        }

        [[nodiscard]] size_t find(std::string_view sv_arg, size_t start = 0) const noexcept {
            return find(STextView(sv_arg.data(), sv_arg.size()), start);
        }

        // ---------------------------------------------------------
        // find_first_of / find_last_of (bitmap-accelerated for ASCII)
        // ---------------------------------------------------------

        [[nodiscard]] size_t find_first_of(STextView chars, size_t start = 0) const noexcept {
            // Fast path: single char → delegate to memchr-based find
            if (chars.size() == 1) return find(chars[0], start);

            const size_t sz = size();
            const char*  p  = data();
            bool table[256] = {};
            for (size_t i = 0; i < chars.size(); ++i) {
                table[static_cast<unsigned char>(chars[i])] = true;
            }
            for (size_t i = start; i < sz; ++i) {
                if (table[static_cast<unsigned char>(p[i])]) return i;
            }
            return npos;
        }

        [[nodiscard]] size_t find_last_of(STextView chars) const noexcept {
            const size_t sz = size();
            const char*  p  = data();
            bool table[256] = {};
            for (size_t i = 0; i < chars.size(); ++i) {
                table[static_cast<unsigned char>(chars[i])] = true;
            }
            for (size_t i = sz; i > 0; --i) {
                if (table[static_cast<unsigned char>(p[i - 1])]) return i - 1;
            }
            return npos;
        }

        // ---------------------------------------------------------
        // Trim / Contains / Prefix / Suffix
        // ---------------------------------------------------------

        [[nodiscard]] STextView trim() const noexcept { return view().trim(); }

        [[nodiscard]] bool contains(STextView pattern) const noexcept { return find(pattern) != npos; }
        [[nodiscard]] bool contains(char c)            const noexcept { return find(c)       != npos; }

        [[nodiscard]] bool starts_with(STextView prefix) const noexcept {
            if (prefix.size() > size()) return false;
            return std::memcmp(data(), prefix.data(), prefix.size()) == 0;
        }

        [[nodiscard]] bool ends_with(STextView suffix) const noexcept {
            if (suffix.size() > size()) return false;
            return std::memcmp(data() + size() - suffix.size(), suffix.data(), suffix.size()) == 0;
        }

        // ---------------------------------------------------------
        // Replace (returns new string)
        // ---------------------------------------------------------

        [[nodiscard]] SString replace(STextView from, STextView to) const {
            if (from.empty()) return *this;

            const size_t from_len = from.size();
            const size_t to_len   = to.size();
            const size_t sz       = size();
            const char*  src      = data();

            // Count occurrences
            size_t count = 0;
            size_t pos = 0;
            while ((pos = find(from, pos)) != npos) {
                ++count;
                pos += from_len;
            }
            if (count == 0) return *this;

            const size_t new_size = sz + count * to_len - count * from_len;
            SString res(new_size);      // reserves exact capacity (SSO if <= 30)

            char*  dst_ptr = res.data();
            size_t src_pos = 0;
            size_t dst_pos = 0;
            pos = 0;

            while ((pos = find(from, pos)) != npos) {
                const size_t copy_len = pos - src_pos;
                if (copy_len > 0) {
                    std::memcpy(dst_ptr + dst_pos, src + src_pos, copy_len);
                    dst_pos += copy_len;
                }
                if (to_len > 0) {
                    std::memcpy(dst_ptr + dst_pos, to.data(), to_len);
                    dst_pos += to_len;
                }
                src_pos = pos + from_len;
                pos = src_pos;
            }

            if (src_pos < sz) {
                std::memcpy(dst_ptr + dst_pos, src + src_pos, sz - src_pos);
                dst_pos += sz - src_pos;
            }

            dst_ptr[new_size] = 0;
            res._set_size(new_size);
            return res;
        }

        // ---------------------------------------------------------
        // replace_first (in-place, single occurrence)
        // ---------------------------------------------------------

        bool replace_first(STextView from, STextView to) {
            if (from.empty()) return false;

            const size_t pos = find(from);
            if (pos == npos) return false;

            const size_t from_len = from.size();
            const size_t to_len   = to.size();
            const size_t sz       = size();

            if (from_len == to_len) {
                std::memcpy(data() + pos, to.data(), to_len);
            } else if (to_len < from_len) {
                char* p = data();
                std::memcpy(p + pos, to.data(), to_len);
                std::memmove(p + pos + to_len, p + pos + from_len, sz - (pos + from_len));
                const size_t new_sz = sz - (from_len - to_len);
                p[new_sz] = 0;
                _set_size(new_sz);
            } else {
                const size_t diff = to_len - from_len;
                if (sz + diff > capacity()) {
                    _grow_to(sz + diff);
                }
                char* p = data();
                std::memmove(p + pos + to_len, p + pos + from_len, sz - (pos + from_len));
                std::memcpy(p + pos, to.data(), to_len);
                const size_t new_sz = sz + diff;
                p[new_sz] = 0;
                _set_size(new_sz);
            }
            return true;
        }

        // ---------------------------------------------------------
        // replace_inplace (in-place, all occurrences)
        // ---------------------------------------------------------

        void replace_inplace(STextView from, STextView to) {
            if (from.empty() || from == to) return;

            size_t pos = find(from, 0);
            if (pos == npos) return;

            const size_t from_len = from.size();
            const size_t to_len   = to.size();
            const long long diff  = static_cast<long long>(to_len) - static_cast<long long>(from_len);

            if (diff <= 0) {
                // Shrinking or same size: single-pass in-place
                char*  p         = data();
                const size_t sz  = size();
                size_t read_pos  = pos + from_len;
                size_t write_pos = pos;

                if (to_len > 0) {
                    std::memcpy(p + write_pos, to.data(), to_len);
                    write_pos += to_len;
                }

                while (true) {
                    const size_t next_pos = find(from, read_pos);
                    if (next_pos == npos) {
                        const size_t remaining = sz - read_pos;
                        if (remaining > 0) {
                            std::memmove(p + write_pos, p + read_pos, remaining);
                            write_pos += remaining;
                        }
                        break;
                    }

                    const size_t len = next_pos - read_pos;
                    if (len > 0) {
                        std::memmove(p + write_pos, p + read_pos, len);
                        write_pos += len;
                    }

                    if (to_len > 0) {
                        std::memcpy(p + write_pos, to.data(), to_len);
                        write_pos += to_len;
                    }

                    read_pos = next_pos + from_len;
                }

                p[write_pos] = 0;
                _set_size(write_pos);
            } else {
                // Growing: count first, then build in new buffer
                const size_t sz = size();
                size_t count = 1;
                size_t temp_pos = pos + from_len;
                while ((temp_pos = find(from, temp_pos)) != npos) {
                    ++count;
                    temp_pos += from_len;
                }

                const size_t new_size = sz + count * static_cast<size_t>(diff);
                char* new_p = new (std::nothrow) char[new_size + 1];
                if (!new_p) return;

                const char* old_p = data();
                size_t src_pos  = 0;
                size_t dst_pos  = 0;
                size_t cur_pos  = pos;

                if (cur_pos > 0) {
                    std::memcpy(new_p, old_p, cur_pos);
                    dst_pos = cur_pos;
                }

                do {
                    if (to_len > 0) {
                        std::memcpy(new_p + dst_pos, to.data(), to_len);
                        dst_pos += to_len;
                    }

                    src_pos = cur_pos + from_len;
                    cur_pos = find(from, src_pos);

                    const size_t len = (cur_pos == npos ? sz : cur_pos) - src_pos;
                    if (len > 0) {
                        std::memcpy(new_p + dst_pos, old_p + src_pos, len);
                        dst_pos += len;
                    }
                } while (cur_pos != npos);

                if (!_is_sso()) {
                    delete[] _u.heap.ptr;
                }
                _u.heap.ptr  = new_p;
                _u.heap.size = new_size;
                _u.heap.cap  = new_size;
                _u.sso[kTagIdx] = static_cast<char>(kHeapFlag);
                new_p[new_size] = 0;
            }
        }

        // ---------------------------------------------------------
        // Insert / Erase
        // ---------------------------------------------------------

        SString& insert(size_t pos, std::string_view sv_arg) {
            const size_t sz = size();
            if (pos > sz) pos = sz;
            const size_t needed = sz + sv_arg.size();
            if (needed > capacity()) {
                _grow_to(std::max(needed, capacity() * 2));
            }
            char* p = data();
            std::memmove(p + pos + sv_arg.size(), p + pos, sz - pos);
            std::memcpy(p + pos, sv_arg.data(), sv_arg.size());
            p[needed] = 0;
            _set_size(needed);
            return *this;
        }

        SString& erase(size_t pos, size_t count = npos) {
            const size_t sz = size();
            if (pos >= sz) return *this;
            count = std::min(count, sz - pos);
            char* p = data();
            std::memmove(p + pos, p + pos + count, sz - pos - count);
            const size_t new_sz = sz - count;
            p[new_sz] = 0;
            _set_size(new_sz);
            return *this;
        }

        // ---------------------------------------------------------
        // Factory
        // ---------------------------------------------------------

        [[nodiscard]] static SString from_view(STextView v) {
            return SString(std::string_view(v.data(), v.size()));
        }

        [[nodiscard]] static SString from_utf8(std::string_view sv_arg) {
            return SString(sv_arg);
        }

        // ---------------------------------------------------------
        // Iterators
        // ---------------------------------------------------------

        iterator       begin()        noexcept { return data(); }
        iterator       end()          noexcept { return data() + size(); }
        const_iterator begin()  const noexcept { return data(); }
        const_iterator end()    const noexcept { return data() + size(); }
        const_iterator cbegin() const noexcept { return data(); }
        const_iterator cend()   const noexcept { return data() + size(); }

    private:
        // ---------------------------------------------------------
        // Internal Layout (32 bytes)
        //
        //   SSO mode  (tag < 0x80):
        //     sso[0 .. len-1] = character data
        //     sso[len]        = '\0'
        //     sso[31]         = length  (0 .. 30)
        //
        //   Heap mode (tag >= 0x80):
        //     heap.ptr   @ 0-7   = pointer to heap buffer
        //     heap.size  @ 8-15  = string length
        //     heap.cap   @ 16-23 = allocated capacity (excl. null)
        //     sso[31]    @ 31    = kHeapFlag (0x80)
        // ---------------------------------------------------------
        static constexpr unsigned char kHeapFlag = 0x80u;

        union {
            char sso[kBufSize];
            struct {
                char*  ptr;
                size_t size;
                size_t cap;
            } heap;
        } _u;

        // -- helpers --

        [[nodiscard]] bool _is_sso() const noexcept {
            return (static_cast<unsigned char>(_u.sso[kTagIdx]) & kHeapFlag) == 0;
        }

        [[nodiscard]] size_t _sso_len() const noexcept {
            return static_cast<size_t>(static_cast<unsigned char>(_u.sso[kTagIdx]));
        }

        void _set_size(size_t n) noexcept {
            if (_is_sso()) {
                _u.sso[kTagIdx] = static_cast<char>(n);
            } else {
                _u.heap.size = n;
            }
        }

        void _init_heap(size_t cap) {
            _u.heap.ptr  = new char[cap + 1];
            _u.heap.ptr[0] = 0;
            _u.heap.size = 0;
            _u.heap.cap  = cap;
            _u.sso[kTagIdx] = static_cast<char>(kHeapFlag);
        }

        void _grow_to(size_t new_cap) {
            const size_t cur_size = size();
            const char*  cur_data = data();
            char* new_p = new char[new_cap + 1];
            if (cur_size > 0) {
                std::memcpy(new_p, cur_data, cur_size + 1);
            } else {
                new_p[0] = 0;
            }
            if (!_is_sso()) {
                delete[] _u.heap.ptr;
            }
            _u.heap.ptr  = new_p;
            _u.heap.size = cur_size;
            _u.heap.cap  = new_cap;
            _u.sso[kTagIdx] = static_cast<char>(kHeapFlag);
        }

        void _assign_raw(const char* s, size_t len) {
            if (len <= kSsoMax) {
                if (len > 0) std::memcpy(_u.sso, s, len);
                _u.sso[len] = 0;
                _u.sso[kTagIdx] = static_cast<char>(len);
            } else {
                _u.heap.ptr = new char[len + 1];
                std::memcpy(_u.heap.ptr, s, len);
                _u.heap.ptr[len] = 0;
                _u.heap.size = len;
                _u.heap.cap  = len;
                _u.sso[kTagIdx] = static_cast<char>(kHeapFlag);
            }
        }

        void _assign_from(const char* s, size_t len) {
            if (len <= kSsoMax) {
                // Save heap ptr before overwriting union bytes
                char* old_heap = _is_sso() ? nullptr : _u.heap.ptr;
                if (len > 0) std::memcpy(_u.sso, s, len);
                _u.sso[len] = 0;
                _u.sso[kTagIdx] = static_cast<char>(len);
                delete[] old_heap;
            } else if (!_is_sso() && _u.heap.cap >= len) {
                // Reuse existing heap buffer
                std::memcpy(_u.heap.ptr, s, len);
                _u.heap.ptr[len] = 0;
                _u.heap.size = len;
            } else {
                // New heap allocation
                char* new_p = new char[len + 1];
                std::memcpy(new_p, s, len);
                new_p[len] = 0;
                if (!_is_sso()) delete[] _u.heap.ptr;
                _u.heap.ptr  = new_p;
                _u.heap.size = len;
                _u.heap.cap  = len;
                _u.sso[kTagIdx] = static_cast<char>(kHeapFlag);
            }
        }
    };

    // ---------------------------------------------------------
    // sizeof check
    // ---------------------------------------------------------
    static_assert(sizeof(SString) == 32, "SString should be 32 bytes (same as std::string, 2x SSO capacity)");

    // ---------------------------------------------------------
    // Concatenation Operators
    // ---------------------------------------------------------

    [[nodiscard]] inline SString operator+(const SString& lhs, const SString& rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs.sv());
        result.append(rhs.sv());
        return result;
    }

    [[nodiscard]] inline SString operator+(const SString& lhs, std::string_view rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs.sv());
        result.append(rhs);
        return result;
    }

    [[nodiscard]] inline SString operator+(std::string_view lhs, const SString& rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs);
        result.append(rhs.sv());
        return result;
    }

    [[nodiscard]] inline SString operator+(const SString& lhs, const char* rhs) {
        return lhs + std::string_view(rhs);
    }

    [[nodiscard]] inline SString operator+(const char* lhs, const SString& rhs) {
        return std::string_view(lhs) + rhs;
    }

    // ---------------------------------------------------------
    // Comparison Operators
    // ---------------------------------------------------------

    [[nodiscard]] inline bool operator==(const SString& lhs, const SString& rhs) noexcept {
        return lhs.sv() == rhs.sv();
    }

    [[nodiscard]] inline bool operator==(const SString& lhs, std::string_view rhs) noexcept {
        return lhs.sv() == rhs;
    }

    [[nodiscard]] inline bool operator==(std::string_view lhs, const SString& rhs) noexcept {
        return lhs == rhs.sv();
    }

    [[nodiscard]] inline bool operator==(const SString& lhs, const char* rhs) noexcept {
        return lhs.sv() == std::string_view(rhs);
    }

    [[nodiscard]] inline std::strong_ordering operator<=>(const SString& lhs, const SString& rhs) noexcept {
        return lhs.sv() <=> rhs.sv();
    }
}

// ---------------------------------------------------------
// std::hash specialization
// ---------------------------------------------------------
template<>
struct std::hash<shine::SString> {
    [[nodiscard]] size_t operator()(const shine::SString& s) const noexcept {
        return s.hash();
    }
};

// ---------------------------------------------------------
// std::formatter specializations (C++23)
// ---------------------------------------------------------
template<>
struct std::formatter<shine::SString> : std::formatter<std::string_view> {
    auto format(const shine::SString& s, auto& ctx) const {
        return std::formatter<std::string_view>::format(s.sv(), ctx);
    }
};

template<>
struct std::formatter<shine::STextView> : std::formatter<std::string_view> {
    auto format(shine::STextView tv, auto& ctx) const {
        return std::formatter<std::string_view>::format(std::string_view(tv), ctx);
    }
};
