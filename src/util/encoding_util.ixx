#ifdef SHINE_USE_MODULE

module;

export module shine.util.encoding;

import <string>;
import <span>;
import <vector>;
import <string_view>;

#else

#pragma once

#include <string>
#include <span>
#include <vector>
#include <string_view>

#endif

namespace shine::util
{
    class EncodingUtil
    {
    public:
        using UTF32CharType = UTF32Char;

        [[nodiscard]] static size_t UTF8ToUTF32Char(const unsigned char* src, unsigned int& dst);

        [[nodiscard]] static size_t UTF8ToUTF32(const unsigned char* src, int srcLen, unsigned int* dst);

        [[nodiscard]] static size_t UTF8ToUTF32(const unsigned char* src, int srcLen, UTF32Char* dst);

        [[nodiscard]] static size_t UTF32ToUTF8(std::span<const unsigned int> src, std::string& dst);

        [[nodiscard]] static std::vector<char16_t> UTF8ToUTF16(std::string_view src);

        [[nodiscard]] static std::string UTF16ToUTF8(std::u16string_view src);

        [[nodiscard]] static int UTF32CharToUTF8(char32_t src, std::span<unsigned char, 4> dst);

        [[nodiscard]] static int UTF32CharToUTF16(char32_t u32Ch, std::span<char16_t, 2> u16Ch);

        [[nodiscard]] static constexpr int UTF8CharLen(unsigned char ch) noexcept {
            if (ch < 0x80) return 1;
            if ((ch & 0xE0) == 0xC0) return 2;
            if ((ch & 0xF0) == 0xE0) return 3;
            if ((ch & 0xF8) == 0xF0) return 4;
            return 0;
        }

        [[nodiscard]] static constexpr bool IsUTF8StartByte(unsigned char ch) noexcept {
            return (ch & 0xC0) != 0x80;
        }

        [[nodiscard]] static int UTF8ToUTF32Char(std::span<const unsigned char> src, char32_t& dst);

#ifdef _WIN32
        [[nodiscard]] static size_t WstringToUTF32(const wchar_t* src, std::span<char32_t> dst);

        [[nodiscard]] static std::string WstringToUTF8(std::wstring_view wstr);

        [[nodiscard]] static std::wstring UTF8ToWstring(std::string_view u8str);

        [[nodiscard]] static std::string UTF8ToWstring_Navtive(std::string_view u8str);

        [[nodiscard]] static std::string WstringToANSI(std::wstring_view wstr);

        [[nodiscard]] static std::wstring ANSIToWstring(std::string_view astr);
#endif
    };
}
