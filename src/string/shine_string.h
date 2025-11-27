#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include "third/fmt/format.h"
#include "third/fmt/printf.h"

namespace shine
{
    class FString
    {
    public:
        using ElementType = char;

        FString() = default;
        FString(const char* str) : Data(str ? str : "") {}
        FString(const std::string& str) : Data(str) {}
        FString(std::string&& str) : Data(std::move(str)) {}
        FString(size_t n, char c) : Data(n, c) {}
        FString(const char* str, size_t len) : Data(str, len) {}

        // Copy and Move
        FString(const FString&) = default;
        FString(FString&&) = default;
        FString& operator=(const FString&) = default;
        FString& operator=(FString&&) = default;

        // Assignment
        FString& operator=(const char* str) { Data = str ? str : ""; return *this; }
        FString& operator=(const std::string& str) { Data = str; return *this; }

        // Accessors
        const char* operator*() const { return Data.c_str(); }
        const char* C_Str() const { return Data.c_str(); }
        char* GetCharData() { return Data.data(); }
        size_t Len() const { return Data.length(); }
        bool IsEmpty() const { return Data.empty(); }

        // Iterators
        auto begin() { return Data.begin(); }
        auto end() { return Data.end(); }
        auto begin() const { return Data.begin(); }
        auto end() const { return Data.end(); }

        // Operators
        FString operator+(const FString& other) const { return FString(Data + other.Data); }
        FString operator+(const char* str) const { return FString(Data + (str ? str : "")); }
        FString& operator+=(const FString& other) { Data += other.Data; return *this; }
        FString& operator+=(const char* str) { if (str) Data += str; return *this; }
        FString& operator+=(char c) { Data += c; return *this; }

        bool operator==(const FString& other) const { return Data == other.Data; }
        bool operator!=(const FString& other) const { return Data != other.Data; }
        bool operator<(const FString& other) const { return Data < other.Data; }
        
        // Comparisons with char*
        bool operator==(const char* other) const { return other && Data == other; }
        bool operator!=(const char* other) const { return !other || Data != other; }

        char& operator[](size_t index) { return Data[index]; }
        const char& operator[](size_t index) const { return Data[index]; }

        // UE5-like API
        
        // Static creators
        static FString Printf(const char* fmt, ...);
        
        template <typename... Args>
        static FString Format(const char* fmt, Args&&... args) {
            try {
                return FString(fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
            } catch (...) {
                return FString("");
            }
        }

        // String manipulation
        // Returns the length of the string in bytes
        size_t Len() const { return Data.length(); }
        // Returns the number of UTF-8 characters (code points)
        size_t LenChars() const;

        // Character-based substring (Indices are code points, not bytes)
        FString Mid(size_t start, size_t count = std::string::npos) const;
        FString Left(size_t count) const;
        FString Right(size_t count) const;
        
        // Byte-based substring (Indices are bytes) (Faster)
        FString MidBytes(size_t start, size_t count = std::string::npos) const;
        
        bool Split(const FString& InS, FString* LeftS, FString* RightS, bool bUseCase = true) const;

        FString Replace(const char* From, const char* To, bool bIgnoreCase = false) const;
        
        bool Contains(const char* subStr, bool bIgnoreCase = false) const;
        bool Equals(const FString& other, bool bIgnoreCase = false) const;
        
        int Find(const char* subStr, bool bIgnoreCase = false, bool bSearchFromEnd = false, int StartPosition = -1) const;

        void Empty() { Data.clear(); }
        void Reset() { Data.clear(); }
        
        void Append(const char* str) { if(str) Data += str; }
        void AppendChar(char c) { Data += c; }

        void InsertAt(size_t Index, const char* str);
        void InsertAt(size_t Index, char c);

        void RemoveAt(size_t Index, size_t Count = 1, bool bAllowShrinking = true);

        FString TrimStart() const;
        FString TrimEnd() const;
        FString Trim() const;

        FString ToUpper() const;
        FString ToLower() const;
        
        FString Reverse() const;

        // Conversion
        bool ToBool() const;
        int ToInt() const;
        float ToFloat() const;
        double ToDouble() const;

        // Helpers
        static bool IsNumeric(const char* str);

        // Underlying std::string access (for compatibility)
        const std::string& GetStdString() const { return Data; }
        std::string& GetStdString() { return Data; }

    private:
        std::string Data;
    };

    // Global operators for char* + FString
    inline FString operator+(const char* lhs, const FString& rhs) {
        return FString(lhs) + rhs;
    }
}
