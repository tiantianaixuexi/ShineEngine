#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "shine_define.h"
#include "shine_text_view.h"

namespace shine
{
    class SString
    {
    public:
        using iterator = char*;
        using const_iterator = const char*;

        static constexpr size_t kSsoCapacity = 32;

        constexpr SString() noexcept : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {}

        explicit SString(size_t cap) : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            if (cap > kSsoCapacity) {
                size_t alloc_size = cap;
                if (alloc_size < 32) alloc_size = 32;
                _p = new (std::nothrow) char[alloc_size];
                _cap = alloc_size;
            }
            _p[0] = 0;
        }

        SString(size_t cap, size_t size) : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            _size = std::min(size, cap);
            if (_size > kSsoCapacity) {
                size_t alloc_size = cap;
                if (alloc_size < 32) alloc_size = 32;
                _p = new (std::nothrow) char[alloc_size];
                _cap = alloc_size;
            } else {
                _cap = _size;
                _p = _sso;
            }
            _sso[0] = 0;
        }

        explicit SString(std::string_view sv) : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            _size = sv.size();
            if (_size > kSsoCapacity) {
                size_t alloc_size = _size + 1;
                if (alloc_size < 32) alloc_size = 32;
                _p = new (std::nothrow) char[alloc_size];
                _cap = alloc_size;
            }
            if (_p) {
                std::memcpy(_p, sv.data(), _size);
                _p[_size] = 0;
            }
        }

        SString(const char* s) : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            _size = std::strlen(s);
            if (_size > kSsoCapacity) {
                size_t alloc_size = _size + 1;
                if (alloc_size < 32) alloc_size = 32;
                _p = new (std::nothrow) char[alloc_size];
                _cap = alloc_size;
            }
            if (_p) {
                std::memcpy(_p, s, _size);
                _p[_size] = 0;
            }
        }

        ~SString() {
            if (_p != _sso) {
                delete[] _p;
            }
        }

        SString(const SString& other) noexcept : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            _size = other._size;
            if (_size > kSsoCapacity) {
                size_t alloc_size = _size + 1;
                _p = new (std::nothrow) char[alloc_size];
                _cap = alloc_size;
                if (_p && _size > 0) {
                    std::memcpy(_p, other._p, _size + 1);
                }
            } else if (_size > 0) {
                std::memcpy(_sso, other._sso, _size + 1);
            }
        }

        SString& operator=(const SString& other) noexcept {
            if (this != &other) {
                if (_p != _sso) {
                    delete[] _p;
                }
                _size = other._size;
                if (_size > kSsoCapacity) {
                    size_t alloc_size = _size + 1;
                    _p = new (std::nothrow) char[alloc_size];
                    _cap = alloc_size;
                    if (_p) {
                        std::memcpy(_p, other._p, alloc_size);
                    }
                } else {
                    _p = _sso;
                    _cap = kSsoCapacity;
                    if (_size > 0) {
                        std::memcpy(_sso, other._sso, _size + 1);
                    }
                }
            }
            return *this;
        }

        SString(SString&& s) noexcept : _sso{}, _cap(kSsoCapacity), _size(0), _p(_sso) {
            _size = s._size;
            _cap = s._cap;
            _p = s._p;
            if (s._p == s._sso) {
                if (_size > 0) {
                    std::memcpy(_sso, s._sso, _size);
                }
                _p = _sso;
                _cap = kSsoCapacity;
            }
            s._p = s._sso;
            s._cap = kSsoCapacity;
            s._size = 0;
            s._sso[0] = 0;
        }

        SString& operator=(SString&& s) noexcept {
            if (this != &s) {
                if (_p != _sso) {
                    delete[] _p;
                }
                _size = s._size;
                _cap = s._cap;
                _p = s._p;
                if (s._p == s._sso) {
                    if (_size > 0) {
                        std::memcpy(_sso, s._sso, _size);
                    }
                    _p = _sso;
                    _cap = kSsoCapacity;
                }
                s._p = s._sso;
                s._cap = kSsoCapacity;
                s._size = 0;
                s._sso[0] = 0;
            }
            return *this;
        }

        operator STextView() const noexcept { return STextView(_p, _size); }

        [[nodiscard]] STextView view() const noexcept { return STextView(_p, _size); }
        [[nodiscard]] STextView view_no_own() const noexcept { return STextView(_p, _size); }
        [[nodiscard]] constexpr std::string_view sv() const noexcept { return std::string_view(_p, _size); }

        [[nodiscard]] std::string to_utf8() const { return std::string(_p, _size); }

        [[nodiscard]] char* data() const noexcept { return _p; }
        [[nodiscard]] constexpr size_t size() const noexcept { return _size; }
        [[nodiscard]] constexpr size_t capacity() const noexcept { return _cap; }
        [[nodiscard]] constexpr const char* c_str() const noexcept { return _p; }
        [[nodiscard]] constexpr bool empty() const noexcept { return _size == 0; }

        void clear() noexcept {
            _size = 0;
            if (_cap > 0) _p[0] = 0;
        }

        [[nodiscard]] constexpr size_t code_unit_count() const noexcept { return _size; }
        [[nodiscard]] size_t code_point_count() const { return view().code_point_count(); }

        [[nodiscard]] size_t hash() const noexcept {
            size_t h = 0;
            for (char c : std::string_view(_p, _size)) {
                h = h * 31 + static_cast<unsigned char>(c);
            }
            return h;
        }

        void reserve(size_t cap) {
            if (cap <= _cap) return;
            if (_p == _sso && cap <= kSsoCapacity) {
                _cap = cap;
                return;
            }
            char* new_p = new (std::nothrow) char[cap];
            if (!new_p) [[unlikely]] return;
            std::memcpy(new_p, _p, _size + 1);
            if (_p != _sso) {
                delete[] _p;
            }
            _p = new_p;
            _cap = cap;
        }

        void push_back(char c) {
            if (_size + 1 >= _cap) [[unlikely]] {
                size_t new_cap = (_cap == 0) ? 32 : _cap * 2;
                reserve(new_cap);
            }
            _p[_size++] = c;
            _p[_size] = 0;
        }

        SString& append(std::string_view sv) {
            size_t needed = _size + sv.size() + 1;
            if (needed > _cap) [[unlikely]] {
                reserve(std::max(needed, _cap * 2));
            }
            std::memcpy(_p + _size, sv.data(), sv.size());
            _size += sv.size();
            _p[_size] = 0;
            return *this;
        }

        SString& append(STextView sv) {
            std::string_view svv = std::string_view(sv.data(), sv.size());
            return append(svv);
        }

        void resize(size_t size, char c = 0) {
            if (size > _cap) {
                reserve(size + 1);
            }
            if (size > _size) {
                std::memset(_p + _size, c, size - _size);
            }
            _size = size;
            _p[_size] = 0;
        }

        [[nodiscard]] char& operator[](size_t i) { return _p[i]; }
        [[nodiscard]] const char& operator[](size_t i) const { return _p[i]; }

        char& at(size_t i) {
            if (i >= _size) throw std::out_of_range("SString::at");
            return _p[i];
        }

        [[nodiscard]] SString substr(size_t pos, size_t count = npos) const {
            if (pos >= _size) return SString();
            count = std::min(count, _size - pos);
            return SString(std::string_view(_p + pos, count));
        }

        static constexpr size_t npos = static_cast<size_t>(-1);

        [[nodiscard]] size_t find(STextView pattern, size_t start = 0) const {
            if (start >= _size) return npos;
            if (pattern.empty()) return start;
            size_t pat_len = pattern.size();
            if (pat_len > _size - start) return npos;

            const char* p_start = _p + start;
            const char* p_end = _p + _size - pat_len;
            char first = pattern.data()[0];

            const char* p = p_start;
            while (p <= p_end) {
                p = static_cast<const char*>(std::memchr(p, first, p_end - p + 1));
                if (!p) return npos;
                
                if (std::memcmp(p, pattern.data(), pat_len) == 0) {
                    return p - _p;
                }
                ++p;
            }
            return npos;
        }

        [[nodiscard]] size_t find(char c, size_t start = 0) const {
            if (start >= _size) return npos;
            const char* p = static_cast<const char*>(std::memchr(_p + start, c, _size - start));
            return p ? p - _p : npos;
        }

        [[nodiscard]] size_t find_first_of(STextView chars, size_t start = 0) const {
            for (size_t i = start; i < _size; ++i) {
                for (char c : std::string_view(chars)) {
                    if (_p[i] == c) return i;
                }
            }
            return npos;
        }

        [[nodiscard]] size_t find_last_of(STextView chars) const {
            for (size_t i = _size; i > 0; --i) {
                for (char c : std::string_view(chars)) {
                    if (_p[i - 1] == c) return i - 1;
                }
            }
            return npos;
        }

        [[nodiscard]] STextView trim() const {
            size_t start = 0;
            while (start < _size && (_p[start] == ' ' || _p[start] == '\t' || _p[start] == '\n' || _p[start] == '\r')) ++start;
            size_t end = _size;
            while (end > start && (_p[end - 1] == ' ' || _p[end - 1] == '\t' || _p[end - 1] == '\n' || _p[end - 1] == '\r')) --end;
            return STextView(_p + start, end - start);
        }

        [[nodiscard]] bool contains(STextView substr) const { return find(substr) != npos; }
        [[nodiscard]] bool contains(char c) const { return find(c) != npos; }

        [[nodiscard]] bool starts_with(STextView prefix) const {
            if (prefix.size() > _size) return false;
            return std::memcmp(_p, prefix.data(), prefix.size()) == 0;
        }

        [[nodiscard]] bool ends_with(STextView suffix) const {
            if (suffix.size() > _size) return false;
            return std::memcmp(_p + _size - suffix.size(), suffix.data(), suffix.size()) == 0;
        }

        [[nodiscard]] SString replace(STextView from, STextView to) const {
            if (from.empty()) return *this;

            size_t from_len = from.size();
            size_t to_len = to.size();

            size_t count = 0;
            size_t pos = 0;
            while ((pos = find(from, pos)) != npos) {
                ++count;
                pos += 1;
            }

            if (count == 0) return *this;

            size_t new_size = _size;
            if (to_len > from_len) {
                new_size += count * (to_len - from_len);
            } else if (from_len > to_len) {
                new_size -= count * (from_len - to_len);
            }

            SString res(new_size);
            res._size = new_size;

            size_t src_pos = 0;
            size_t dst_pos = 0;
            pos = 0;

            while ((pos = find(from, pos)) != npos) {
                size_t copy_len = pos - src_pos;
                if (copy_len > 0) {
                    std::memcpy(res._p + dst_pos, _p + src_pos, copy_len);
                    dst_pos += copy_len;
                }
                if (to_len > 0) {
                    std::memcpy(res._p + dst_pos, to.data(), to_len);
                    dst_pos += to_len;
                }
                src_pos = pos + from_len;
                pos = src_pos;
            }

            if (src_pos < _size) {
                std::memcpy(res._p + dst_pos, _p + src_pos, _size - src_pos);
                dst_pos += _size - src_pos;
            }

            res._p[res._size] = 0;
            return res;
        }

        bool replace_first(STextView from, STextView to) {
            size_t from_len = from.size();
            if (from_len == 0) return false;
            
            // Optimized find
            size_t pos;
            if (from_len == 1) {
                pos = find(from.data()[0]);
            } else {
                pos = find(from);
            }
            
            if (pos == npos) return false;
            size_t to_len = to.size();
            
            if (from_len == to_len) {
                std::memcpy(_p + pos, to.data(), to_len);
                return true;
            }
            
            if (to_len < from_len) {
                // Shrink
                std::memcpy(_p + pos, to.data(), to_len);
                std::memmove(_p + pos + to_len, _p + pos + from_len, _size - (pos + from_len));
                _size -= (from_len - to_len);
                _p[_size] = 0;
            } else {
                // Expand
                size_t diff = to_len - from_len;
                if (_size + diff >= _cap) {
                    reserve(_size + diff + 1);
                }
                std::memmove(_p + pos + to_len, _p + pos + from_len, _size - (pos + from_len));
                std::memcpy(_p + pos, to.data(), to_len);
                _size += diff;
                _p[_size] = 0;
            }
            return true;
        }

        void replace_inplace(STextView from, STextView to) {
            if (from.empty() || from == to) return;

            size_t pos = find(from, 0);
            if (pos == npos) return;

            size_t from_len = from.size();
            size_t to_len = to.size();
            long long diff = static_cast<long long>(to_len) - static_cast<long long>(from_len);

            if (diff <= 0) {
                size_t read_pos = pos + from_len;
                size_t write_pos = pos;
                
                if (to_len > 0) {
                    std::memcpy(_p + write_pos, to.data(), to_len);
                    write_pos += to_len;
                }

                while (true) {
                    size_t next_pos = find(from, read_pos);
                    if (next_pos == npos) {
                        size_t remaining = _size - read_pos;
                        if (remaining > 0) {
                            std::memmove(_p + write_pos, _p + read_pos, remaining);
                            write_pos += remaining;
                        }
                        break;
                    }
                    
                    size_t len = next_pos - read_pos;
                    if (len > 0) {
                        std::memmove(_p + write_pos, _p + read_pos, len);
                        write_pos += len;
                    }
                    
                    if (to_len > 0) {
                        std::memcpy(_p + write_pos, to.data(), to_len);
                        write_pos += to_len;
                    }
                    
                    read_pos = next_pos + from_len;
                }
                
                _size = write_pos;
                _p[_size] = 0;
            } else {
                // Count first to allocate exact size
                size_t count = 1;
                size_t temp_pos = pos + from_len;
                while ((temp_pos = find(from, temp_pos)) != npos) {
                    ++count;
                    temp_pos += from_len;
                }
                
                size_t new_size = _size + count * diff;
                size_t alloc_size = new_size + 1;
                if (alloc_size < 32) alloc_size = 32;

                char* new_p = new (std::nothrow) char[alloc_size];
                if (!new_p) return; 
                
                size_t src_pos = 0;
                size_t dst_pos = 0;
                size_t current_pos = pos;
                
                if (current_pos > 0) {
                    std::memcpy(new_p, _p, current_pos);
                    dst_pos = current_pos;
                }
                
                do {
                    if (to_len > 0) {
                        std::memcpy(new_p + dst_pos, to.data(), to_len);
                        dst_pos += to_len;
                    }
                    
                    src_pos = current_pos + from_len;
                    current_pos = find(from, src_pos);
                    
                    size_t len = (current_pos == npos ? _size : current_pos) - src_pos;
                    if (len > 0) {
                        std::memcpy(new_p + dst_pos, _p + src_pos, len);
                        dst_pos += len;
                    }
                } while (current_pos != npos);
                
                if (_p != _sso) {
                    delete[] _p;
                }
                _p = new_p;
                _size = new_size;
                _cap = alloc_size;
                _p[_size] = 0;
            }
        }

        [[nodiscard]] static SString from_view(STextView v) {
            SString s(v.size() + 1);
            if (v.size() > 0) {
                std::memcpy(s._p, v.data(), v.size());
            }
            s._size = v.size();
            s._p[s._size] = 0;
            return s;
        }

        [[nodiscard]] static SString from_utf8(std::string_view sv) {
            return SString(sv);
        }

        iterator begin() { return _p; }
        iterator end() { return _p + _size; }
        const_iterator begin() const { return _p; }
        const_iterator end() const { return _p + _size; }

    private:
        char _sso[kSsoCapacity]{};
        size_t _cap = kSsoCapacity;
        size_t _size = 0;
        char* _p = _sso;
    };

    [[nodiscard]] inline SString operator+(const SString& lhs, const SString& rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs.view());
        result.append(rhs.view());
        return result;
    }

    [[nodiscard]] inline SString operator+(const SString& lhs, std::string_view rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs.view());
        result.append(rhs);
        return result;
    }

    [[nodiscard]] inline SString operator+(std::string_view lhs, const SString& rhs) {
        SString result(lhs.size() + rhs.size());
        result.append(lhs);
        result.append(rhs.view());
        return result;
    }

    [[nodiscard]] inline SString operator+(const SString& lhs, const char* rhs) {
        return lhs + SString(rhs);
    }

    [[nodiscard]] inline SString operator+(const char* lhs, const SString& rhs) {
        return SString(lhs) + rhs;
    }

    [[nodiscard]] inline bool operator==(const SString& lhs, const SString& rhs) noexcept {
        return lhs.sv() == rhs.sv();
    }

    [[nodiscard]] inline bool operator==(const SString& lhs, std::string_view rhs) noexcept {
        return lhs.sv() == rhs;
    }

    [[nodiscard]] inline bool operator==(std::string_view lhs, const SString& rhs) noexcept {
        return lhs == rhs.sv();
    }

    [[nodiscard]] inline bool operator!=(const SString& lhs, const SString& rhs) noexcept {
        return !(lhs == rhs);
    }

    [[nodiscard]] inline std::strong_ordering operator<=>(const SString& lhs, const SString& rhs) noexcept {
        return lhs.sv() <=> rhs.sv();
    }
}
