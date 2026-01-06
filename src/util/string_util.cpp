#ifdef SHINE_USE_MODULE


module;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "fmt/format.h"
#include "fast_float/fast_float.h"

module shine.util.string_util;


#else

#include "string_util.ixx"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "fmt/format.h"
#include "fast_float/fast_float.h"




#include <string>
#include <string_view>
#include <system_error>
#include <span>
#include <algorithm>

#endif

namespace shine::util
{


size_t StringUtil::UTF8ToUTF32Char(const unsigned char* src,unsigned int & dst)
{
    if(src == nullptr) return 0;

    int i,len = 0;
    unsigned char c = *src++;

    if(c < 0x80){
        dst = c;
        return 1;
    }

    if(c < 0xC0 ||c > 0xFD){
        return 0;
    }

    if(c < 0xE0){
        dst = c & 0x1F;
        len = 2;
        goto end;
    }

    if(c < 0xF0){
        dst = c & 0x0F;
        len = 3;
        goto end;
    }

    if(c < 0xF8){
        dst = c & 0x07;
        len = 4;
        goto end;
    }

    if(c < 0xFC){
        dst = c & 0x03;
        len = 5;
        goto end;
    }

    dst = c & 0x01;
    len = 6;

end:

    for(i = 1; i < len; i++){
        c = *src++;
        if((c & 0xC0) != 0x80){ return 0; }
        dst = (dst << 6) | (c & 0x3F);
    }

    if( i < len){
        return 0;
    }

    return len;
}

size_t StringUtil::UTF8ToUTF32(const unsigned char* src,const int srcLen, unsigned int* dst) {
    int pos = 0;
    int i = 0;
    for (; pos < srcLen;)
    {
        int len = UTF8ToUTF32Char(src + pos, dst[i++]);
        if (len==0)
        {
            return 0;
        }
        pos += len;
    }
    return i;
}

size_t StringUtil::UTF8ToUTF32(const unsigned char* src,const int srcLen,UTF32Char* dst) {

    int pos = 0;
    int i = 0;
    for (; pos < srcLen;)
    {
        int len = UTF8ToUTF32Char(src + pos, dst[i].mCharCode);
        dst[i++].mByteCount = len;
        if (len == 0)
        {
            return 0;
        }
        pos += len;
    }
    return i;

}

size_t StringUtil::UTF32ToUTF8(std::span<const unsigned int> src, std::string& dst) {
    size_t bytesWritten = 0;
    const size_t dstCapacity = dst.size();

    for (unsigned int code : src) {
        if (code < 0x80) {
            if (bytesWritten + 1 > dstCapacity) return 0;
            dst[bytesWritten++] = static_cast<unsigned char>(code);
        } else if (code < 0x800) {
            if (bytesWritten + 2 > dstCapacity) return 0;
            dst[bytesWritten++] = static_cast<unsigned char>(0xC0 | (code >> 6));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | (code & 0x3F));
        } else if (code < 0x10000) {
            if (bytesWritten + 3 > dstCapacity) return 0;
            dst[bytesWritten++] = static_cast<unsigned char>(0xE0 | (code >> 12));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | ((code >> 6) & 0x3F));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | (code & 0x3F));
        } else if (code <= 0x10FFFF) {
            if (bytesWritten + 4 > dstCapacity) return 0;
            dst[bytesWritten++] = static_cast<unsigned char>(0xF0 | (code >> 18));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | ((code >> 12) & 0x3F));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | ((code >> 6) & 0x3F));
            dst[bytesWritten++] = static_cast<unsigned char>(0x80 | (code & 0x3F));
        } else {
            return 0;
        }
    }
    return bytesWritten;
}

std::vector<char16_t> StringUtil::UTF8ToUTF16(std::string_view sv) {
    size_t len = 0;
    // 第一遍扫描：计算所需 char16_t 数量
    for (size_t i = 0; i < sv.size(); ) {
        unsigned char c = sv[i];
        if (c < 0x80) { i++; len++; }
        else if ((c & 0xE0) == 0xC0) { i += 2; len++; }
        else if ((c & 0xF0) == 0xE0) { i += 3; len++; }
        else if ((c & 0xF8) == 0xF0) { i += 4; len += 2; }
        else { i++; len++; } // 无效字符占1位
    }

    std::vector<char16_t> res;
    res.reserve(len + 1);

    for (size_t i = 0; i < sv.size(); ) {
        unsigned char c = sv[i];
        if (c < 0x80) {
            res.push_back(c);
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= sv.size()) break;
            char32_t cp = ((c & 0x1F) << 6) | (sv[i + 1] & 0x3F);
            res.push_back((char16_t)cp);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= sv.size()) break;
            char32_t cp = ((c & 0x0F) << 12) | ((sv[i + 1] & 0x3F) << 6) | (sv[i + 2] & 0x3F);
            res.push_back((char16_t)cp);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= sv.size()) break;
            char32_t cp = ((c & 0x07) << 18) | ((sv[i + 1] & 0x3F) << 12) | ((sv[i + 2] & 0x3F) << 6) | (sv[i + 3] & 0x3F);
            cp -= 0x10000;
            res.push_back((char16_t)(0xD800 + (cp >> 10)));
            res.push_back((char16_t)(0xDC00 + (cp & 0x3FF)));
            i += 4;
        } else {
            res.push_back(0xFFFD);
            i++;
        }
    }
    return res;
}

std::string StringUtil::UTF16ToUTF8(std::u16string_view src) {
    std::string res;
    res.reserve(src.size() * 3);
    const char16_t* p = src.data();
    const char16_t* end = src.data() + src.size();
    
    // 简单的本地辅助函数
    auto is_high = [](char16_t c) { return c >= 0xD800 && c <= 0xDBFF; };
    auto is_low = [](char16_t c) { return c >= 0xDC00 && c <= 0xDFFF; };

    while (p < end) {
        char32_t cp;
        char16_t c = *p++;
        if (is_high(c)) {
            if (p < end && is_low(*p)) {
                char32_t hi = c - 0xD800;
                char32_t lo = *p++ - 0xDC00;
                cp = 0x10000 + ((hi << 10) | lo);
            } else {
                cp = 0xFFFD;
            }
        } else if (is_low(c)) {
            cp = 0xFFFD;
        } else {
            cp = c;
        }

        if (cp < 0x80) {
            res.push_back((char)cp);
        } else if (cp < 0x800) {
            res.push_back((char)(0xC0 | (cp >> 6)));
            res.push_back((char)(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            res.push_back((char)(0xE0 | (cp >> 12)));
            res.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            res.push_back((char)(0x80 | (cp & 0x3F)));
        } else {
            res.push_back((char)(0xF0 | (cp >> 18)));
            res.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
            res.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            res.push_back((char)(0x80 | (cp & 0x3F)));
        }
    }
    return res;
}

int StringUtil::UTF32CharToUTF8(char32_t src, std::span<unsigned char, 4> dst) {
    int len = 0;
    if (src < 0x80) {
        dst[0] = static_cast<unsigned char>(src);
        len = 1;
    } else if (src < 0x800) {
        dst[0] = static_cast<unsigned char>(0xC0 | (src >> 6));
        dst[1] = static_cast<unsigned char>(0x80 | (src & 0x3F));
        len = 2;
    } else if (src < 0x10000) {
        if (src >= 0xD800 && src <= 0xDFFF) return 0;
        dst[0] = static_cast<unsigned char>(0xE0 | (src >> 12));
        dst[1] = static_cast<unsigned char>(0x80 | ((src >> 6) & 0x3F));
        dst[2] = static_cast<unsigned char>(0x80 | (src & 0x3F));
        len = 3;
    } else if (src <= 0x10FFFF){
        dst[0] = static_cast<unsigned char>(0xF0 | (src >> 18));
        dst[1] = static_cast<unsigned char>(0x80 | ((src >> 12) & 0x3F));
        dst[2] = static_cast<unsigned char>(0x80 | ((src >> 6) & 0x3F));
        dst[3] = static_cast<unsigned char>(0x80 | (src & 0x3F));
        len = 4;
    } else {
        return 0;
    }
    return len;
}

int StringUtil::UTF32CharToUTF16(char32_t u32Ch, std::span<char16_t, 2> u16Ch) {
    if (u32Ch <= 0xFFFF) {
        if (u32Ch >= 0xD800 && u32Ch <= 0xDFFF) return 0;
        u16Ch[0] = static_cast<char16_t>(u32Ch);
        return 1;
    } else if (u32Ch <= 0x10FFFF) {
        u16Ch[0] = static_cast<char16_t>(0xD800 + ((u32Ch - 0x10000) >> 10));
        u16Ch[1] = static_cast<char16_t>(0xDC00 + ((u32Ch - 0x10000) & 0x3FF));
        return 2;
    }
    return 0;
}

int StringUtil::UTF8CharLen(unsigned char ch) {
    if (ch < 0x80) return 1;
    if ((ch & 0xE0) == 0xC0) return 2;
    if ((ch & 0xF0) == 0xE0) return 3;
    if ((ch & 0xF8) == 0xF0) return 4;
    return 1;
}

bool StringUtil::IsUTF8StartByte(unsigned char ch) {
    return (ch & 0xC0) != 0x80;
}

int StringUtil::UTF8ToUTF32Char(std::span<const unsigned char> src, char32_t& dst) {
    if (src.empty()) return 0;

    unsigned char c = src[0];
    int bytes = 1;
    char32_t code = 0;
    const size_t srcLen = src.size();

    if (c < 0x80) {
        code = c;
    } else if ((c & 0xE0) == 0xC0) {
        if (srcLen < 2 || (src[1] & 0xC0) != 0x80) return 0;
        code = ((c & 0x1F) << 6) | (src[1] & 0x3F);
        bytes = 2;
    } else if ((c & 0xF0) == 0xE0) {
        if (srcLen < 3 || (src[1] & 0xC0) != 0x80 || (src[2] & 0xC0) != 0x80) return 0;
        code = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
        if (code >= 0xD800 && code <= 0xDFFF) return 0;
        bytes = 3;
    } else if ((c & 0xF8) == 0xF0) {
        if (srcLen < 4 || (src[1] & 0xC0) != 0x80 || (src[2] & 0xC0) != 0x80 || (src[3] & 0xC0) != 0x80) return 0;
        code = ((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
        if (code > 0x10FFFF) return 0;
        bytes = 4;
    } else {
        return 0;
    }

    dst = code;
    return bytes;
}

#ifdef _WIN32
size_t StringUtil::WstringToUTF32(const wchar_t* src, std::span<char32_t> dst) {
    size_t count = 0;
    const size_t dstCapacity = dst.size();
    const wchar_t* p = src;

    while (*p != L'\0' && count < dstCapacity) {
        char32_t code = 0;
        wchar_t wc1 = *p++;
        if (wc1 >= 0xD800 && wc1 <= 0xDBFF) {
            if (*p >= 0xDC00 && *p <= 0xDFFF) {
                wchar_t wc2 = *p++;
                code = 0x10000 + (((char32_t)(wc1 & 0x3FF)) << 10) + (wc2 & 0x3FF);
            } else {
                code = 0xFFFD;
            }
        } else if (wc1 >= 0xDC00 && wc1 <= 0xDFFF) {
            code = 0xFFFD;
        } else {
            code = wc1;
        }
        dst[count++] = code;
    }
    return count;
}

std::string StringUtil::WstringToUTF8(std::wstring_view wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return {};
    std::string u8str(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), u8str.data(), size_needed, nullptr, nullptr);
    if (bytes_converted <= 0) return {};
    return u8str;
}

std::wstring StringUtil::UTF8ToWstring(std::string_view u8str) {
    if (u8str.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, u8str.data(), static_cast<int>(u8str.size()), nullptr, 0);
    if (size_needed <= 0) return {};
    std::wstring wstr(size_needed, 0);
    int chars_converted = MultiByteToWideChar(CP_UTF8, 0, u8str.data(), static_cast<int>(u8str.size()), wstr.data(), size_needed);
    if (chars_converted <= 0) return {};
    return wstr;
}

std::string StringUtil::UTF8ToWstring_Navtive(std::string_view u8str)
{
    std::wstring wstr = UTF8ToWstring(u8str);
    std::string str(wstr.size(), '\0');
    for (size_t i = 0; i < wstr.size(); ++i) {
        str[i] = static_cast<char>(wstr[i]);
    }
    return str;

}

std::string StringUtil::WstringToANSI(std::wstring_view wstr) {
    if (wstr.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) return {};
    std::string astr(size_needed, 0);
    int bytes_converted = WideCharToMultiByte(CP_ACP, 0, wstr.data(), static_cast<int>(wstr.size()), astr.data(), size_needed, nullptr, nullptr);
    if (bytes_converted <= 0) return {};
    return astr;
}

std::wstring StringUtil::ANSIToWstring(std::string_view astr) {
    if (astr.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_ACP, 0, astr.data(), static_cast<int>(astr.size()), nullptr, 0);
    if (size_needed <= 0) return {};
    std::wstring wstr(size_needed, 0);
    int chars_converted = MultiByteToWideChar(CP_ACP, 0, astr.data(), static_cast<int>(astr.size()), wstr.data(), size_needed);
    if (chars_converted <= 0) return {};
    return wstr;
}

#endif

// ===================== 瀛楃涓茶鍓笌鏌ユ壘 =====================

std::string StringUtil::TrimStart(std::string_view str, std::string_view prefix) {
    if (str.starts_with(prefix)) {
        return std::string(str.substr(prefix.length()));
    }
    return std::string(str);
}

std::string StringUtil::TrimEnd(std::string_view str, std::string_view suffix) {
    if (str.ends_with(suffix)) {
        return std::string(str.substr(0, str.length() - suffix.length()));
    }
    return std::string(str);
}

bool StringUtil::EndsWith(std::string_view str, std::string_view suffix, bool IgnoreCase) {
    if (str.length() < suffix.length()) {
        return false;
    }
    if (IgnoreCase)
    {
        return str.ends_with(suffix);
    }
    else
    {
        // 使用C++20的ranges::equal和视图操作
        auto str_suffix = str.substr(str.length() - suffix.length());
        for (size_t i = 0; i < suffix.length(); ++i) {
            if (std::tolower(static_cast<unsigned char>(str_suffix[i])) !=
                std::tolower(static_cast<unsigned char>(suffix[i]))) {
                return false;
            }
        }
        
        return true;
    }

    __assume(false);
}


bool StringUtil::StartsWith(std::string_view str, std::string_view prefix, bool IgnoreCase) {
    if (str.length() < prefix.length()) {
        return false;
    }
    if (IgnoreCase)
    {
        return str.starts_with(prefix);
    }

    return false;

}

std::vector<std::string> StringUtil::SplitPathComponents(std::string_view path) {
    std::vector<std::string> components;
    size_t start = 0;
    size_t pos;

    while ((pos = path.find_first_of("/\\", start)) != std::string_view::npos) {
        if (pos > start) {
            components.emplace_back(path.substr(start, pos - start));
        }
        start = path.find_first_not_of("/\\", pos);
        if (start == std::string_view::npos) {
            start = path.length();
            break;
        }
    }

    if (start < path.size()) {
        components.emplace_back(path.substr(start));
    }
    return components;
}

std::string_view StringUtil::GetFileExtension(std::string_view path) {
    size_t dotPos = path.rfind('.');
    size_t sepPos = path.find_last_of("/\\");

    if (dotPos != std::string_view::npos && (sepPos == std::string_view::npos || dotPos > sepPos)) {
        if (dotPos > 0 && path[dotPos-1] != '/' && path[dotPos-1] != '\\') {
            if (dotPos > 1 && path[dotPos-1] == '.' && (path[dotPos-2] == '/' || path[dotPos-2] == '\\')) {
                return {};
            }
            return path.substr(dotPos);
        }
    }
    return {};
}

std::string_view StringUtil::GetDirectory(std::string_view path) {
    size_t sepPos = path.find_last_of("/\\");
    if (sepPos != std::string_view::npos) {
        return path.substr(0, sepPos);
    }
    return {};
}

std::string StringUtil::TrimFileExtension(std::string_view path) {
    std::string_view ext = GetFileExtension(path);
    if (!ext.empty()) {
        size_t dotPos = path.rfind('.');
        if (dotPos > 0 && path[dotPos-1] != '/' && path[dotPos-1] != '\\') {
            if (dotPos > 1 && path[dotPos-1] == '.' && (path[dotPos-2] == '/' || path[dotPos-2] == '\\')) {
                return std::string(path);
            } else {
                return std::string(path.substr(0, path.length() - ext.length()));
            }
        }
    }
    return std::string(path);
}

std::string StringUtil::GetFileNameWithoutExtension(std::string_view path) {
    size_t sepPos = path.find_last_of("/\\");
    std::string_view filename = (sepPos == std::string_view::npos) ? path : path.substr(sepPos + 1);

    size_t dotPos = filename.rfind('.');
    if (dotPos != std::string_view::npos && dotPos > 0) {
        if (filename == "." || filename == "..") return std::string(filename);
        return std::string(filename.substr(0, dotPos));
    }
    return std::string(filename);
}

std::string StringUtil::ToStandardPath(std::string_view path) {
    std::string result = ReplaceAll(path, "\\", " / ");
    return result;
}


std::string StringUtil::ToWindowsPath(std::string_view path) {
    std::string result = ReplaceAll(path, "/", " \\");
    return result;
}


std::string StringUtil::URLEncode(std::string_view input) {

    std::string encoded;
    encoded.reserve(input.size() * 3); // 棰勭暀瓒冲绌洪棿

    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%';
            encoded += ___hex[c >> 4];
            encoded += ___hex[c & 0xF];
        }
    }
    return encoded;
}

std::string StringUtil::URLDecode(std::string_view input) {
    std::string decoded;
    decoded.reserve(input.length());
    const char* current = input.data();
    const char* const end = input.data() + input.length();

    while (current < end) {
        if (*current == '%') {
            if (current + 2 < end) {
                int value = 0;
                auto result = fast_float::from_chars(current + 1, current + 3, value);
                if (result.ec == std::errc{} && result.ptr == current + 3) {
                    decoded += static_cast<char>(value);
                    current += 3;
                } else {
                    decoded += '%';
                    current++;
                }
            } else {
                decoded += '%';
                current++;
            }
        } else if (*current == '+') {
            decoded += ' ';
            current++;
        } else {
            decoded += *current;
            current++;
        }
    }
    return decoded;
}

std::string StringUtil::BytesToHex(std::span<const unsigned char> bytes) {

    std::string result;
    result.reserve(bytes.size() * 2);

    for (unsigned char byte : bytes) {
        result += ___hex[byte >> 4];
        result += ___hex[byte & 0xF];
    }
    return result;
}

// ===================== 现代高性能字符串工具(C++23/26) =====================

std::uint32_t StringUtil::HashFNV1a(std::string_view str) {
    constexpr std::uint32_t fnv_prime = 16777619u;
    constexpr std::uint32_t fnv_offset_basis = 2166136261u;

    std::uint32_t hash = fnv_offset_basis;

    // 使用C++20的ranges视图
    for (auto c : str )
    {
        hash ^= static_cast<uint8_t>(c);
        hash *= fnv_prime;
    }

    return hash;
}

std::vector<std::string> StringUtil::Split(std::string_view str, char delim) {
    std::vector<std::string> result;

    if (str.empty()) {
        return result;
    }

    size_t count = 1;
    for (char c : str) {
        if (c == delim) ++count;
    }
    result.reserve(count);

    // 鍒嗗壊瀛楃涓?
    size_t start = 0;
    for (size_t pos = str.find(delim); pos != std::string_view::npos; pos = str.find(delim, start)) {
        result.emplace_back(str.substr(start, pos - start));
        start = pos + 1;
    }

    // 娣诲姞鏈€鍚庝竴涓儴鍒?
    if (start <= str.size()) {
        result.emplace_back(str.substr(start));
    }

    return result;
}

std::string StringUtil::ToLower(std::string_view str) {
    std::string out(str);
    for (auto c : out) {
		c = static_cast<unsigned char>(std::tolower(c));
    }
    return out;
}

std::string StringUtil::ToUpper(std::string_view str) {
    std::string out(str);
    for (auto& c : out)
    {
		c = static_cast<unsigned char>(std::toupper(c));
    }
    return out;
}

std::string ToStandardPath(std::string_view path) {
    std::string result(path);
    // 缁熶竴灏嗗弽鏂滄潬鏇挎崲涓烘鏂滄潬
    size_t pos = 0;
    while ((pos = result.find('\\', pos)) != std::string::npos) {
        result.replace(pos, 1, "/");
        pos += 1; // 璺宠繃鍒氭浛鎹㈢殑'/'
    }
    return result;
}

std::string StringUtil::NormalizePath(std::string path) {

		// 缁熶竴璺緞鍒嗛殧绗︿负 '/'
		path.replace(path.begin(), path.end(), '\\', '/');

    
        // 鍒嗗壊璺緞缁勪欢
        std::vector<std::string> components;

        // 澶勭悊缁濆璺緞鍓嶇紑
        bool isAbsolute = path.starts_with('/');

        // 浣跨敤find_if鏉ュ垎鍓茶矾寰?
        size_t start = isAbsolute ? 1 : 0;
        while (start < path.size()) {
            auto pos = path.find('/', start);
            if (pos == std::string::npos) pos = path.size();

            std::string_view component = std::string_view(path).substr(start, pos - start);

            if (!component.empty() && component != ".") {
                if (component == "..") {
                    if (!components.empty()) {
                        components.pop_back();
                    }
                }
                else {
                    components.emplace_back(component);
                }
            }

            start = pos + 1;
        }

        // 閲嶆柊缁勫悎璺緞
        std::string result;
        if (isAbsolute && !components.empty()) {
            result = "/";
        }

        for (size_t i = 0; i < components.size(); ++i) {
            result += components[i];
            if (i < components.size() - 1) {
                result += "/";
            }
        }

        // 濡傛灉璺緞涓虹┖锛岃繑鍥炲綋鍓嶇洰褰?
        if (result.empty()) {
            result = isAbsolute ? "/" : ".";
        }

        return result;
}


bool StringUtil::WildcardMatch(std::string_view str, std::string_view pattern) {
    // 杩唬鏂瑰紡鐨勯€氶厤绗﹀尮閰嶏紝閬垮厤閫掑綊鏍堟孩鍑?
    auto s = str.begin();
    auto p = pattern.begin();
    const auto s_end = str.end();
    const auto p_end = pattern.end();

    // 鐢ㄤ簬鍥炴函鐨勪綅缃褰?
    auto star_p = p_end;
    auto star_s = s_end;

    while (s != s_end) {
        // 绮剧‘鍖归厤鎴栧崟瀛楃閫氶厤绗?
        if (p != p_end && (*p == '?' || *p == *s)) {
            ++s;
            ++p;
        }
        // 閬囧埌*閫氶厤绗︼紝璁板綍浣嶇疆锛屽苟灏濊瘯璺宠繃*
        else if (p != p_end && *p == '*') {
            star_p = ++p;
            star_s = s;
        }
        // 褰撳墠涓嶅尮閰嶄絾涔嬪墠鏈?锛屽洖婧埌*鍚庣殑浣嶇疆
        else if (star_p != p_end) {
            p = star_p;
            s = ++star_s;
        }
        // 涓嶅尮閰嶄笖娌℃湁鍙洖婧殑*
        else {
            return false;
        }
    }

    // 璺宠繃灏鹃儴鐨?
    while (p != p_end && *p == '*') ++p;

    return p == p_end; // 妯″紡蹇呴』瀹屽叏鍖归厤
}

// 閫氶厤绗︽浛鎹細鍙敮鎸佸叏灞€ * 鍖归厤鏇挎崲
std::string StringUtil::RegexReplace(std::string_view str, std::string_view pattern, std::string_view replacement) {
    // 鍙敮鎸?pattern 涓?"*" 鐨勭畝鍗曟浛鎹?
    if (pattern == "*") {
        return std::string(replacement);
    }
    // 鍙敮鎸?pattern 涓烘櫘閫氬瓙涓茬殑鍏ㄥ眬鏇挎崲
    return ReplaceAll(str, pattern, replacement);
}

bool StringUtil::HasUTF8BOM(std::span<unsigned char> data) {
    constexpr unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
    if (data.size() < 3) return false;

    if (data[0] == bom[0] && data[1] == bom[1] && data[2] == bom[2]) {
        return true;
    }
    return false;
}

std::string StringUtil::DetectEncoding(std::span<const unsigned char> data) {
    // 检查BOM标记
    if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return "utf-8-bom";
    }
    if (data.size() >= 2) {
        if (data[0] == 0xFF && data[1] == 0xFE) return "utf-16le";
        if (data[0] == 0xFE && data[1] == 0xFF) return "utf-16be";
    }

    // 如果没有BOM，默认检查UTF-8
    if (data.empty()) {
        return "ascii"; // 空数据视为ASCII
    }

    const size_t checkLen = (std::min)(data.size(), size_t{4096});
    bool onlyAscii = true;

    // 使用UTF-8验证算法检查UTF-8合法性
    for (size_t i = 0; i < checkLen;) {
        const unsigned char lead = data[i];

        if (lead < 0x80) {
            // ASCII序列
            ++i;
            continue;
        }

        // 遇到非ASCII，标记
        onlyAscii = false;

        // 根据首字节确定序列长度
        int seqLen = 0;
        if ((lead & 0xE0) == 0xC0) seqLen = 2;
        else if ((lead & 0xF0) == 0xE0) seqLen = 3;
        else if ((lead & 0xF8) == 0xF0) seqLen = 4;
        else return "unknown"; // 无效UTF-8首字节

        // 检查是否有足够的后续字节
        if (i + seqLen > checkLen) return "unknown";

        // 检查所有后续字节是否符合UTF-8编码规则(10xxxxxx)
        for (int j = 1; j < seqLen; ++j) {
            if ((data[i + j] & 0xC0) != 0x80) return "unknown";
        }

        i += seqLen;
    }

    return onlyAscii ? "ascii" : "utf-8";
}

std::string StringUtil::ToPlatformPath(std::string_view path) {
#ifdef _WIN32
    return ToWindowsPath(path);
#else
    return ToStandardPath(path);
#endif
}

#ifdef _WIN32
std::string StringUtil::ToNativeEncoding(std::string_view str) {
    std::wstring wstr = UTF8ToWstring(str);
    if (wstr.empty() && !str.empty()) return {};
    return WstringToANSI(wstr);
}

std::string StringUtil::FromNativeEncoding(std::string_view str) {
    std::wstring wstr = ANSIToWstring(str);
    if (wstr.empty() && !str.empty()) return {};
    return WstringToUTF8(wstr);
}
#endif

std::string StringUtil::Interpolate(std::string_view template_str,
                                   const std::unordered_map<std::string, std::string>& replacements) {
    if (template_str.empty() || replacements.empty()) {
        return std::string(template_str);
    }

    std::string result;
    result.reserve(template_str.size() * 2); // 棰勭暀姣旀ā鏉挎洿澶氱殑绌洪棿浠ュ噺灏戦噸鏂板垎閰?

    size_t last_pos = 0;

    // 鎵惧埌鎵€鏈?{key} 妯″紡骞舵浛鎹?
    while (true) {
        size_t open_pos = template_str.find('{', last_pos);
        if (open_pos == std::string_view::npos) {
            break;
        }

        // 娣诲姞 { 鍓嶇殑鍐呭
        result.append(template_str.substr(last_pos, open_pos - last_pos));

        size_t close_pos = template_str.find('}', open_pos + 1);
        if (close_pos == std::string_view::npos) {
            // 鎵句笉鍒板尮閰嶇殑 }锛屼繚鐣欏墿浣欏唴瀹?
            result.append(template_str.substr(open_pos));
            return result;
        }

        // 鎻愬彇閿苟鏌ユ壘鏇挎崲鍊?
        std::string_view key = template_str.substr(open_pos + 1, close_pos - open_pos - 1);
        auto it = replacements.find(std::string(key));

        if (it != replacements.end()) {
            // 鎵惧埌鏇挎崲鍊硷紝娣诲姞鍒扮粨鏋?
            result.append(it->second);
        } else {
            // 鏈壘鍒版浛鎹㈠€硷紝淇濈暀鍘熷 {key}
            result.append(template_str.substr(open_pos, close_pos - open_pos + 1));
        }

        last_pos = close_pos + 1;
    }

    // 娣诲姞妯℃澘鍓╀綑閮ㄥ垎
    if (last_pos < template_str.size()) {
        result.append(template_str.substr(last_pos));
    }

    return result;
}

bool StringUtil::ValidateUTF8(std::string_view str) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
    const size_t len = str.size();

    for (size_t i = 0; i < len; /* 在循环体内增加 i */) {
        const unsigned char lead = bytes[i];
        size_t code_length = 0;

        // 确定UTF-8编码的长度
        if (lead < 0x80) {
            // ASCII字符 (0xxxxxxx)
            code_length = 1;
        } else if ((lead & 0xE0) == 0xC0) {
            // 2字节序列 (110xxxxx 10xxxxxx)
            code_length = 2;

            // 非法的过长编码：C0 C1是用2字节编码ASCII的非法尝试
            if (lead < 0xC2) return false;
        } else if ((lead & 0xF0) == 0xE0) {
            // 3字节序列 (1110xxxx 10xxxxxx 10xxxxxx)
            code_length = 3;

            // UTF-8编码的代理区检查(U+D800-U+DFFF)和过长编码检查
            if (lead == 0xE0 && i + 1 < len && (bytes[i+1] & 0xE0) == 0x80) return false; // 过长编码
            if (lead == 0xED && i + 1 < len && (bytes[i+1] & 0xE0) == 0xA0) return false; // 代理区
        } else if ((lead & 0xF8) == 0xF0) {
            // 4字节序列 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
            code_length = 4;

            // 过长编码和Unicode限制检查(最大U+10FFFF)
            if (lead == 0xF0 && i + 1 < len && (bytes[i+1] < 0x90)) return false; // 过长编码
            if (lead >= 0xF5) return false; // 超出Unicode范围
        } else {
            // 无效的UTF-8首字节
            return false;
        }

        // 检查是否有足够的后续字节
        if (i + code_length > len) return false;

        // 验证所有后续字节必须是10xxxxxx格式
        for (size_t j = 1; j < code_length; ++j) {
            if ((bytes[i + j] & 0xC0) != 0x80) return false;
        }

        // 处理下一个字符
        i += code_length;
    }

    return true;
}

std::string StringUtil::ReplaceAll(std::string_view str, std::string_view from, std::string_view to) {
    if (from.empty() || str.empty()) return std::string(str);

    std::string result;
    result.reserve(str.size()); // 棰勭暀绌洪棿浠ュ噺灏戦噸鏂板垎閰?

    size_t pos = 0;
    size_t prev_pos = 0;

    // 鏌ユ壘姣忎釜鍖归厤椤瑰苟鏇挎崲
    while ((pos = str.find(from, prev_pos)) != std::string_view::npos) {
        // 娣诲姞浠庝笂娆″尮閰嶇粨鏉熷埌褰撳墠鍖归厤寮€濮嬬殑鍐呭
        result.append(str.substr(prev_pos, pos - prev_pos));
        // 娣诲姞鏇挎崲鏂囨湰
        result.append(to);
        // 绉诲姩鍒颁笅涓€涓彲鑳界殑鍖归厤浣嶇疆
        prev_pos = pos + from.size();
    }

    // 娣诲姞鏈€鍚庝竴閮ㄥ垎锛堜粠鏈€鍚庝竴娆″尮閰嶅悗鍒板瓧绗︿覆缁撳熬锛?
    result.append(str.substr(prev_pos));

    return result;
}

bool StringUtil::Contains(std::string_view str, std::string_view substr) {
    return str.find(substr) != std::string_view::npos;
}

std::string StringUtil::Trim(std::string_view str) {
    if (str.empty()) return {};

    // 鎵惧埌绗竴涓潪绌虹櫧瀛楃
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    // 鍏ㄦ槸绌虹櫧瀛楃鐨勬儏鍐?
    if (start == str.size()) return {};

    // 鎵惧埌鏈€鍚庝竴涓潪绌虹櫧瀛楃
    size_t end = str.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(str[end]))) {
        --end;
    }

    // 涓€娆℃€ф埅鍙栭渶瑕佺殑閮ㄥ垎
    return std::string(str.substr(start, end - start + 1));
}

std::vector<std::string> StringUtil::SplitLines(std::string_view text) {
    std::vector<std::string> lines;

    if (text.empty()) {
        return lines;
    }

    lines.reserve(NumberCount(text, '\n') + 1);

    size_t start = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n' || text[i] == '\r') {
            lines.emplace_back(text.substr(start, i - start));

            // 处理CRLF序列
            if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }

            start = i + 1;
        }
    }

    // 娣诲姞鏈€鍚庝竴琛岋紙濡傛灉涓嶄互鎹㈣绗︾粨鏉燂級
    if (start < text.size()) {
        lines.emplace_back(text.substr(start));
    }

    return lines;
}

int StringUtil::NumberCount(std::string_view str, char s) noexcept
{
    int count = 0;
    for (auto& c : str) {
        if (c == s) {
			++count;
		}
    }
    return count;
}


} // namespace shine::Util


