#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <functional>
#include <vector>

#include "shine_define.h"
#include "shine_text_view.h"
#include "../util/string_util.h"

namespace shine
{
    class SString
    {
    public:
        // =========================================================
        // P0: Survival Level - Basic Constraints
        // =========================================================
        
		constexpr SString() noexcept :_cap(0), _size(0), _p(nullptr){}

        explicit SString(size_t cap):
        _cap(cap), _size(0){
            _p = cap > 0 ?new TChar[cap] : nullptr;
		}

        SString(size_t cap, size_t size) :
            _cap(cap), _size(size){
            _p = cap > 0 ? new TChar[cap] : nullptr;
        }

        SString(TChar* p, size_t cap, size_t size) :
            _cap(cap), _size(size), _p(p) {
        }


        ~SString(){
            this->reset();
		}

        // Copy Constructor (Deep Copy) - Now allowed for convenience, but explicit? 
        // User asked to ban copy initially, but container ops usually imply copy. 
        // Stick to MOVE-ONLY as per initial P0 requirement unless asked otherwise.
        // Wait, "ban copy" was a strict requirement. I will respect it.
		SString(const SString&) = delete;
		void operator=(const SString&) = delete;

        SString(SString&& s) noexcept :
            _cap(s._cap), _size(s._size), _p(s._p)
        {
            s._cap  = 0;
            s._size = 0;
            s._p    = nullptr;
		}

        SString& operator=(SString&& s) noexcept
        {
            if (this != &s)
            {
                reset();
                _p = s._p;
                _cap = s._cap;
                _size = s._size;

                s._p = nullptr;
                s._cap = 0;
                s._size = 0;
            }
            return *this;
		}

        // Implicit conversion to STextView
        operator STextView() const noexcept {
            return STextView(_p, _size);
        }

        STextView view() const noexcept {
            return STextView(_p, _size);
        }

		TChar* data() const noexcept { return _p; }
		size_t size() const noexcept { return _size; }
		size_t capacity() const noexcept { return _cap; }
		const TChar* c_str() const noexcept { return _p; }

		bool empty() const noexcept { return _size == 0; }
        void clear() noexcept { _size = 0; }


        void reset()
        {
	        if (_p!=nullptr)
	        {
				delete[] _p;
                _p    =  nullptr;
                _cap  =  0;
                _size = 0;
	        }
        }

        // =========================================================
        // P0: UTF-16 Basic Tools (Forward to View)
        // =========================================================
        
        // P0: Length / Char Count Distinction
        size_t code_unit_count() const noexcept { return _size; }
        size_t code_point_count() const { return view().code_point_count(); }

        // P0: Safe Traversal
        void for_each_code_point(std::function<void(char32_t)> fn) const { view().for_each_code_point(fn); }
        size_t find(char32_t cp) const { return view().find(cp); }
        bool contains(char32_t cp) const { return view().contains(cp); }
        
        static constexpr size_t npos = static_cast<size_t>(-1);

        // =========================================================
        // P1: Engineering Grade - Substring / Index
        // =========================================================

        size_t utf16_index_from_code_point(size_t cp_index) const { return view().utf16_index_from_code_point(cp_index); }

        SString substr_cp(size_t pos, size_t count) const {
             STextView v = view().substr_cp(pos, count);
             return from_view(v);
        }

        SString substr_units(size_t unit_pos, size_t unit_count) const {
            STextView v = view().substr_units(unit_pos, unit_count);
            return from_view(v);
        }

        // =========================================================
        // P1: Cursor / Edit Support
        // =========================================================

        bool is_code_point_boundary(size_t utf16_index) const { return view().is_code_point_boundary(utf16_index); }
        size_t next_code_point(size_t utf16_index) const { return view().next_code_point(utf16_index); }
        size_t prev_code_point(size_t utf16_index) const { return view().prev_code_point(utf16_index); }

        // =========================================================
        // P1: Search / Compare
        // =========================================================
        
        size_t find_cp(char32_t cp, size_t start = 0) const { return view().find_cp(cp, start); }
        int compare_cp(const SString& rhs) const { return view().compare_cp(rhs.view()); }

        // =========================================================
        // P2: External Interaction (UTF-8)
        // =========================================================

        static SString from_utf8(std::string_view sv) {
            std::vector<char16_t> buf = util::StringUtil::UTF8ToUTF16(sv);
            SString res(buf.size() + 1, buf.size());
            if (!buf.empty()) {
                std::copy(buf.begin(), buf.end(), res._p);
            }
            if (res._cap > res._size) res._p[res._size] = 0;
            return res;
        }

        std::string to_utf8() const {
            return util::StringUtil::UTF16ToUTF8(std::u16string_view(reinterpret_cast<const char16_t*>(_p), _size));
        }

        static bool is_valid_utf8(std::string_view sv) { return STextView::is_valid_utf8(sv); }
        bool is_valid_utf16() const { return view().is_valid_utf16(); }

        void replace_invalid_sequences() {
            for (size_t i = 0; i < _size; ++i) {
                if (STextView::is_high_surrogate(_p[i])) {
                    if (i + 1 >= _size || !STextView::is_low_surrogate(_p[i+1])) {
                        _p[i] = 0xFFFD;
                    } else {
                        i++;
                    }
                } else if (STextView::is_low_surrogate(_p[i])) {
                     _p[i] = 0xFFFD;
                }
            }
        }

        // =========================================================
        // P3: Container Operations (Enrichment)
        // =========================================================

        void reserve(size_t new_cap) {
            if (new_cap <= _cap) return;
            
            TChar* new_p = new TChar[new_cap];
            if (_size > 0) {
                std::copy(_p, _p + _size, new_p);
            }
            if (_p) delete[] _p;
            _p = new_p;
            _cap = new_cap;
        }

        void resize(size_t new_size) {
            if (new_size > _cap) {
                reserve(std::max(new_size, _cap * 2)); // Growth strategy
            }
            _size = new_size;
            if (_cap > _size) _p[_size] = 0; // Null terminate if possible
        }

        void push_back(TChar c) {
            if (_size + 1 >= _cap) { // +1 for safety/null-term preference
                reserve(std::max((size_t)8, _cap * 2));
            }
            _p[_size++] = c;
            if (_cap > _size) _p[_size] = 0;
        }

        void append(STextView sv) {
            if (sv.empty()) return;
            if (_size + sv.size() >= _cap) {
                reserve(std::max(_size + sv.size() + 1, _cap * 2));
            }
            std::copy(sv.begin(), sv.end(), _p + _size);
            _size += sv.size();
            if (_cap > _size) _p[_size] = 0;
        }

        SString& operator+=(STextView sv) {
            append(sv);
            return *this;
        }

        SString& operator+=(TChar c) {
            push_back(c);
            return *this;
        }

        // =========================================================
        // P3: Common Algorithms (Shortcuts)
        // =========================================================
        bool starts_with(STextView prefix) const { return view().starts_with(prefix); }
        bool ends_with(STextView suffix) const { return view().ends_with(suffix); }

        SString trim() const {
            return from_view(view().trim());
        }

        SString replace(STextView from, STextView to) const {
            if (from.empty()) return SString(*this, 0); // Deep copy hack? No, SString is move only.
            // Wait, we need a way to copy if we want to return a modified copy.
            // But copy ctor is deleted. 
            // We should create a new string manually.
            
            // Count occurrences to pre-allocate
            size_t count = 0;
            size_t pos = 0;
            while ((pos = view().find_cp(from.size() > 0 ? from.data()[0] : 0, pos)) != npos) { // find_cp is char based... wait.
                 // find_cp finds a code point. 'from' is a string.
                 // We need a proper find(substring) in STextView first.
                 // For now, let's just do a naive implementation.
                 break; 
            }
            
            // Actually, let's implement a simple non-optimized replace
            SString res;
            size_t start = 0;
            // We need find(substring) in View. It's missing. 
            // I'll add it to View first or do it manually here.
            
            const TChar* p = _p;
            const TChar* end = _p + _size;
            
            while (p < end) {
                if (static_cast<size_t>(end - p) >= from.size() && std::equal(from.begin(), from.end(), p)) {
                    res.append(to);
                    p += from.size();
                } else {
                    res.push_back(*p++);
                }
            }
            return res;
        }

    private:
        // Helper to clone from view
        SString(const SString& other, int) : _cap(other._size), _size(other._size) {
             _p = _cap > 0 ? new TChar[_cap] : nullptr;
             if (_size > 0) std::copy(other._p, other._p + _size, _p);
        }

        static SString from_view(STextView v) {
            SString s(v.size() + 1, v.size());
            if (v.size() > 0) {
                std::copy(v.begin(), v.end(), s._p);
            }
            if (s._cap > s._size) s._p[s._size] = 0;
            return s;
        }

        size_t _cap   = 0;
        size_t _size  = 0;
        TChar*  _p     = nullptr;
    };
}
