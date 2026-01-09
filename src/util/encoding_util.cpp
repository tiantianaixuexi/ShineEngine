#include "encoding_util.ixx"

namespace shine::util
{
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
            if (src[1] == 0) return 0;
            dst = ((c & 0x1F) << 6) | (src[1] & 0x3F);
            return 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            if (src[1] == 0 || src[2] == 0) return 0;
            dst = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
            return 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            if (src[1] == 0 || src[2] == 0 || src[3] == 0) return 0;
            dst = ((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
            return 4;
        }
        return 0;
    }

    size_t EncodingUtil::UTF8ToUTF32(const unsigned char* src, int srcLen, unsigned int* dst)
    {
        if (src == nullptr || srcLen <= 0 || dst == nullptr) return 0;

        size_t count = 0;
        int i = 0;
        while (i < srcLen)
        {
            unsigned int codePoint = 0;
            size_t byteCount = UTF8ToUTF32Char(src + i, codePoint);
            if (byteCount == 0) break;

            dst[count++] = codePoint;
            i += static_cast<int>(byteCount);
        }
        return count;
    }

    size_t EncodingUtil::UTF8ToUTF32(const unsigned char* src, int srcLen, UTF32CharType* dst)
    {
        if (src == nullptr || srcLen <= 0 || dst == nullptr) return 0;

        size_t count = 0;
        int i = 0;
        while (i < srcLen)
        {
            unsigned int codePoint = 0;
            size_t byteCount = UTF8ToUTF32Char(src + i, codePoint);
            if (byteCount == 0) break;

            dst[count].mCharCode = codePoint;
            dst[count].mByteCount = static_cast<unsigned int>(byteCount);
            count++;
            i += static_cast<int>(byteCount);
        }
        return count;
    }

    size_t EncodingUtil::UTF32ToUTF8(std::span<const unsigned int> src, std::string& dst)
    {
        dst.clear();
        if (src.empty()) return 0;

        for (unsigned int cp : src)
        {
            if (cp < 0x80)
            {
                dst.push_back(static_cast<char>(cp));
            }
            else if (cp < 0x800)
            {
                dst.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp < 0x10000)
            {
                dst.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                dst.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                dst.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                dst.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                dst.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                dst.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return dst.size();
    }

    std::vector<char16_t> EncodingUtil::UTF8ToUTF16(std::string_view src)
    {
        std::vector<char16_t> result;
        if (src.empty()) return result;

        const unsigned char* p = reinterpret_cast<const unsigned char*>(src.data());
        const unsigned char* end = p + src.size();

        while (p < end)
        {
            unsigned int codePoint = 0;
            size_t byteCount = UTF8ToUTF32Char(p, codePoint);
            if (byteCount == 0) break;

            if (codePoint < 0x10000)
            {
                result.push_back(static_cast<char16_t>(codePoint));
            }
            else
            {
                codePoint -= 0x10000;
                result.push_back(static_cast<char16_t>(0xD800 | (codePoint >> 10)));
                result.push_back(static_cast<char16_t>(0xDC00 | (codePoint & 0x3FF)));
            }
            p += byteCount;
        }
        return result;
    }

    std::string EncodingUtil::UTF16ToUTF8(std::u16string_view src)
    {
        std::string result;
        if (src.empty()) return result;

        const char16_t* p = src.data();
        const char16_t* end = p + src.size();

        while (p < end)
        {
            char32_t codePoint = *p++;
            if (codePoint >= 0xD800 && codePoint <= 0xDBFF && p < end)
            {
                char32_t lowSurrogate = *p++;
                codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (lowSurrogate - 0xDC00);
            }

            if (codePoint < 0x80)
            {
                result.push_back(static_cast<char>(codePoint));
            }
            else if (codePoint < 0x800)
            {
                result.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else if (codePoint < 0x10000)
            {
                result.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
                result.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
            else
            {
                result.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
                result.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
            }
        }
        return result;
    }

    int EncodingUtil::UTF32CharToUTF8(char32_t src, std::span<unsigned char, 4> dst)
    {
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
            dst = static_cast<char32_t>(((c & 0x1F) << 6) | (src[1] & 0x3F));
            return 2;
        }
        if ((c & 0xF0) == 0xE0 && src.size() >= 3)
        {
            dst = static_cast<char32_t>(((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F));
            return 3;
        }
        if ((c & 0xF8) == 0xF0 && src.size() >= 4)
        {
            dst = static_cast<char32_t>(((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F));
            return 4;
        }
        return 0;
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

        std::string result;
        const wchar_t* p = wstr.data();
        const wchar_t* end = p + wstr.size();

        while (p < end)
        {
            char32_t cp = static_cast<char32_t>(*p++);
            if (cp < 0x80)
            {
                result.push_back(static_cast<char>(cp));
            }
            else if (cp < 0x800)
            {
                result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp < 0x10000)
            {
                result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return result;
    }

    std::wstring EncodingUtil::UTF8ToWstring(std::string_view u8str)
    {
        if (u8str.empty()) return L"";

        std::wstring result;
        const unsigned char* p = reinterpret_cast<const unsigned char*>(u8str.data());
        const unsigned char* end = p + u8str.size();

        while (p < end)
        {
            char32_t cp = 0;
            size_t byteCount = UTF8ToUTF32Char(p, cp);
            if (byteCount == 0) break;

            result.push_back(static_cast<wchar_t>(cp));
            p += byteCount;
        }
        return result;
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
