#include "encoding_util.ixx"

#include "util/shine_define.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include "editor/views/EditorView.h"
#include <array>

namespace shine::util
{
    namespace
    {
        [[nodiscard]] constexpr bool IsContinuationByte(unsigned char ch) noexcept
        {
            return (ch & 0xC0) == 0x80;
        }

        [[nodiscard]] constexpr bool IsValidCodePoint(char32_t cp) noexcept
        {
            return cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF);
        }
    }

    size_t EncodingUtil::UTF8ToUTF32Char(const unsigned char* src, unsigned int& dst)
    {
        if (src == nullptr) return 0;

        unsigned char c = src[0];
        if (c < 0x80)
        {
            dst = c;
            return 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            if (src[1] == 0 || !IsContinuationByte(src[1]) || c < 0xC2) return 0;
            dst = ((c & 0x1F) << 6) | (src[1] & 0x3F);
            return 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            if (src[1] == 0 || src[2] == 0) return 0;
            if (!IsContinuationByte(src[1]) || !IsContinuationByte(src[2])) return 0;
            if (c == 0xE0 && src[1] < 0xA0) return 0;
            if (c == 0xED && src[1] >= 0xA0) return 0;
            dst = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
            return 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            if (src[1] == 0 || src[2] == 0 || src[3] == 0) return 0;
            if (!IsContinuationByte(src[1]) || !IsContinuationByte(src[2]) || !IsContinuationByte(src[3])) return 0;
            if (c == 0xF0 && src[1] < 0x90) return 0;
            if (c > 0xF4 || (c == 0xF4 && src[1] > 0x8F)) return 0;
            dst = ((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
            return 4;
        }
        return 0;
    }

    size_t EncodingUtil::UTF8ToUTF32(const unsigned char* src, int srcLen, unsigned int* dst)
    {
        if (src == nullptr || srcLen <= 0 || dst == nullptr) return 0;
        return UTF8ToUTF32(std::span<const unsigned char>(src, static_cast<size_t>(srcLen)),
            std::span<unsigned int>(dst, static_cast<size_t>(srcLen)));
    }

    size_t EncodingUtil::UTF8ToUTF32(const unsigned char* src, int srcLen, UTF32CharType* dst)
    {
        if (src == nullptr || srcLen <= 0 || dst == nullptr) return 0;
        return UTF8ToUTF32(std::span<const unsigned char>(src, static_cast<size_t>(srcLen)),
            std::span<UTF32CharType>(dst, static_cast<size_t>(srcLen)));
    }

    size_t EncodingUtil::UTF32ToUTF8(std::span<const unsigned int> src, std::string& dst)
    {
        dst.clear();
        if (src.empty()) return 0;
        dst.resize(src.size() * 4);
        const size_t written = UTF32ToUTF8(src, std::span<unsigned char>(reinterpret_cast<unsigned char*>(dst.data()), dst.size()));
        dst.resize(written);
        return written;
    }

    std::vector<char16_t> EncodingUtil::UTF8ToUTF16(std::string_view src)
    {
        std::vector<char16_t> result(src.size());
        if (src.empty()) return result;
        const auto input = std::span<const unsigned char>(reinterpret_cast<const unsigned char*>(src.data()), src.size());
        const size_t written = UTF8ToUTF16(input, std::span<char16_t>(result.data(), result.size()));
        result.resize(written);
        return result;
    }

    std::string EncodingUtil::UTF16ToUTF8(std::u16string_view src)
    {
        std::string result(src.size() * 4, '\0');
        if (src.empty()) return result;
        const size_t written = UTF16ToUTF8(std::span<const char16_t>(src.data(), src.size()),
            std::span<unsigned char>(reinterpret_cast<unsigned char*>(result.data()), result.size()));
        result.resize(written);
        return result;
    }

    int EncodingUtil::UTF32CharToUTF8(char32_t src, std::span<unsigned char, 4> dst)
    {
        if (!IsValidCodePoint(src))
        {
            return 0;
        }
        if (src < 0x80)
        {
            dst[0] = static_cast<unsigned char>(src);
            return 1;
        }
        if (src < 0x800)
        {
            dst[0] = static_cast<unsigned char>(0xC0 | (src >> 6));
            dst[1] = static_cast<unsigned char>(0x80 | (src & 0x3F));
            return 2;
        }
        if (src < 0x10000)
        {
            dst[0] = static_cast<unsigned char>(0xE0 | (src >> 12));
            dst[1] = static_cast<unsigned char>(0x80 | ((src >> 6) & 0x3F));
            dst[2] = static_cast<unsigned char>(0x80 | (src & 0x3F));
            return 3;
        }
        dst[0] = static_cast<unsigned char>(0xF0 | (src >> 18));
        dst[1] = static_cast<unsigned char>(0x80 | ((src >> 12) & 0x3F));
        dst[2] = static_cast<unsigned char>(0x80 | ((src >> 6) & 0x3F));
        dst[3] = static_cast<unsigned char>(0x80 | (src & 0x3F));
        return 4;
    }

    int EncodingUtil::UTF32CharToUTF16(char32_t u32Ch, std::span<char16_t, 2> u16Ch)
    {
        if (!IsValidCodePoint(u32Ch))
        {
            return 0;
        }
        if (u32Ch < 0x10000)
        {
            u16Ch[0] = static_cast<char16_t>(u32Ch);
            return 1;
        }
        u32Ch -= 0x10000;
        u16Ch[0] = static_cast<char16_t>(0xD800 | (u32Ch >> 10));
        u16Ch[1] = static_cast<char16_t>(0xDC00 | (u32Ch & 0x3FF));
        return 2;
    }

    int EncodingUtil::UTF8ToUTF32Char(std::span<const unsigned char> src, char32_t& dst)
    {
        if (src.size() < 1) return 0;

        unsigned char c = src[0];
        if (c < 0x80)
        {
            dst = static_cast<char32_t>(c);
            return 1;
        }
        if ((c & 0xE0) == 0xC0 && src.size() >= 2)
        {
            if (!IsContinuationByte(src[1]) || c < 0xC2) return 0;
            dst = static_cast<char32_t>(((c & 0x1F) << 6) | (src[1] & 0x3F));
            return 2;
        }
        if ((c & 0xF0) == 0xE0 && src.size() >= 3)
        {
            if (!IsContinuationByte(src[1]) || !IsContinuationByte(src[2])) return 0;
            if (c == 0xE0 && src[1] < 0xA0) return 0;
            if (c == 0xED && src[1] >= 0xA0) return 0;
            dst = static_cast<char32_t>(((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F));
            return 3;
        }
        if ((c & 0xF8) == 0xF0 && src.size() >= 4)
        {
            if (!IsContinuationByte(src[1]) || !IsContinuationByte(src[2]) || !IsContinuationByte(src[3])) return 0;
            if (c == 0xF0 && src[1] < 0x90) return 0;
            if (c > 0xF4 || (c == 0xF4 && src[1] > 0x8F)) return 0;
            dst = static_cast<char32_t>(((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F));
            return 4;
        }
        return 0;
    }

    size_t EncodingUtil::UTF8ToUTF32(std::span<const unsigned char> src, std::span<unsigned int> dst)
    {
        if (src.empty() || dst.empty()) return 0;
        size_t outIndex = 0;
        size_t i = 0;
        while (i < src.size() && outIndex < dst.size())
        {
            char32_t cp = 0;
            const int bytes = UTF8ToUTF32Char(src.subspan(i), cp);
            if (bytes <= 0) break;
            dst[outIndex++] = static_cast<unsigned int>(cp);
            i += static_cast<size_t>(bytes);
        }
        return outIndex;
    }

    size_t EncodingUtil::UTF8ToUTF32(std::span<const unsigned char> src, std::span<UTF32Char> dst)
    {
        if (src.empty() || dst.empty()) return 0;
        size_t outIndex = 0;
        size_t i = 0;
        while (i < src.size() && outIndex < dst.size())
        {
            char32_t cp = 0;
            const int bytes = UTF8ToUTF32Char(src.subspan(i), cp);
            if (bytes <= 0) break;
            dst[outIndex].mCharCode = static_cast<unsigned int>(cp);
            dst[outIndex].mByteCount = static_cast<unsigned int>(bytes);
            ++outIndex;
            i += static_cast<size_t>(bytes);
        }
        return outIndex;
    }

    size_t EncodingUtil::UTF32ToUTF8(std::span<const unsigned int> src, std::span<unsigned char> dst)
    {
        if (src.empty() || dst.empty()) return 0;
        size_t outBytes = 0;
        for (unsigned int cp : src)
        {
            std::array<unsigned char, 4> buf{};
            const int len = UTF32CharToUTF8(static_cast<char32_t>(cp), std::span<unsigned char, 4>(buf));
            if (len <= 0) break;
            if (outBytes + static_cast<size_t>(len) > dst.size()) break;
            for (int i = 0; i < len; ++i)
            {
                dst[outBytes + static_cast<size_t>(i)] = buf[static_cast<size_t>(i)];
            }
            outBytes += static_cast<size_t>(len);
        }
        return outBytes;
    }

    size_t EncodingUtil::UTF8ToUTF16(std::span<const unsigned char> src, std::span<char16_t> dst)
    {
        if (src.empty() || dst.empty()) return 0;
        size_t outUnits = 0;
        size_t i = 0;
        while (i < src.size() && outUnits < dst.size())
        {
            char32_t cp = 0;
            const int bytes = UTF8ToUTF32Char(src.subspan(i), cp);
            if (bytes <= 0) break;
            std::array<char16_t, 2> buf{};
            const int units = UTF32CharToUTF16(cp, std::span<char16_t, 2>(buf));
            if (units <= 0) break;
            if (outUnits + static_cast<size_t>(units) > dst.size()) break;
            for (int u = 0; u < units; ++u)
            {
                dst[outUnits + static_cast<size_t>(u)] = buf[static_cast<size_t>(u)];
            }
            outUnits += static_cast<size_t>(units);
            i += static_cast<size_t>(bytes);
        }
        return outUnits;
    }

    size_t EncodingUtil::UTF16ToUTF8(std::span<const char16_t> src, std::span<unsigned char> dst)
    {
        if (src.empty() || dst.empty()) return 0;
        size_t outBytes = 0;
        size_t i = 0;
        while (i < src.size())
        {
            char32_t cp = src[i++];
            if (cp >= 0xD800 && cp <= 0xDBFF)
            {
                if (i >= src.size()) break;
                const char32_t low = src[i];
                if (low < 0xDC00 || low > 0xDFFF) break;
                ++i;
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
            }
            else if (cp >= 0xDC00 && cp <= 0xDFFF)
            {
                break;
            }
            std::array<unsigned char, 4> buf{};
            const int len = UTF32CharToUTF8(cp, std::span<unsigned char, 4>(buf));
            if (len <= 0) break;
            if (outBytes + static_cast<size_t>(len) > dst.size()) break;
            for (int b = 0; b < len; ++b)
            {
                dst[outBytes + static_cast<size_t>(b)] = buf[static_cast<size_t>(b)];
            }
            outBytes += static_cast<size_t>(len);
        }
        return outBytes;
    }

#ifdef _WIN32
    size_t EncodingUtil::WstringToUTF32(const wchar_t* src, std::span<char32_t> dst)
    {
        if (src == nullptr) return 0;

        size_t count = 0;
        while (*src != 0 && count < dst.size())
        {
            dst[count++] = static_cast<char32_t>(*src++);
        }
        return count;
    }

    std::string EncodingUtil::WstringToUTF8(std::wstring_view wstr)
    {
        if (wstr.empty()) return "";
        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0) return "";
        std::string result(static_cast<size_t>(sizeNeeded), '\0');
        const size_t written = WstringToUTF8(wstr, std::span<char>(result.data(), result.size()));
        result.resize(written);
        return result;
    }

    std::wstring EncodingUtil::UTF8ToWstring(std::string_view u8str)
    {
        if (u8str.empty()) return L"";
        const int sizeNeeded = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8str.data(), static_cast<int>(u8str.size()), nullptr, 0);
        if (sizeNeeded <= 0) return L"";
        std::wstring result(static_cast<size_t>(sizeNeeded), L'\0');
        const size_t written = UTF8ToWstring(u8str, std::span<wchar_t>(result.data(), result.size()));
        result.resize(written);
        return result;
    }

    size_t EncodingUtil::WstringToUTF8(std::wstring_view wstr, std::span<char> dst)
    {
        if (wstr.empty() || dst.empty()) return 0;
        const int written = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), dst.data(), static_cast<int>(dst.size()), nullptr, nullptr);
        if (written <= 0) return 0;
        return static_cast<size_t>(written);
    }

    size_t EncodingUtil::UTF8ToWstring(std::string_view u8str, std::span<wchar_t> dst)
    {
        if (u8str.empty() || dst.empty()) return 0;
        const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u8str.data(), static_cast<int>(u8str.size()), dst.data(), static_cast<int>(dst.size()));
        if (written <= 0) return 0;
        return static_cast<size_t>(written);
    }

    std::string EncodingUtil::UTF8ToWstring_Navtive(std::string_view u8str)
    {
        return WstringToUTF8(UTF8ToWstring(u8str));
    }

    std::string EncodingUtil::WstringToANSI(std::wstring_view wstr)
    {
        if (wstr.empty()) return "";

        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (sizeNeeded <= 0) return "";

        std::string result(static_cast<size_t>(sizeNeeded), 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), sizeNeeded, nullptr, nullptr);
        return result;
    }

    std::wstring EncodingUtil::ANSIToWstring(std::string_view astr)
    {
        if (astr.empty()) return L"";

        int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, astr.data(), static_cast<int>(astr.size()), nullptr, 0);
        if (sizeNeeded <= 0) return L"";

        std::wstring result(static_cast<size_t>(sizeNeeded), 0);
        MultiByteToWideChar(CP_ACP, 0, astr.data(), static_cast<int>(astr.size()), result.data(), sizeNeeded);
        return result;
    }
#endif
}
