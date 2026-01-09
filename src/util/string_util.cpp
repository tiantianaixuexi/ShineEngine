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
#include "encoding_util.ixx"

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
        if (IgnoreCase) {
            return str.ends_with(suffix);
        }
        auto str_suffix = str.substr(str.length() - suffix.length());
        for (size_t i = 0; i < suffix.length(); ++i) {
            if (std::tolower(static_cast<unsigned char>(str_suffix[i])) !=
                std::tolower(static_cast<unsigned char>(suffix[i]))) {
                return false;
            }
        }
        return true;
    }

    bool StringUtil::StartsWith(std::string_view str, std::string_view prefix, bool IgnoreCase) {
        if (str.length() < prefix.length()) {
            return false;
        }
        if (IgnoreCase) {
            return str.starts_with(prefix);
        }
        auto str_prefix = str.substr(0, prefix.length());
        for (size_t i = 0; i < prefix.length(); ++i) {
            if (std::tolower(static_cast<unsigned char>(str_prefix[i])) !=
                std::tolower(static_cast<unsigned char>(prefix[i]))) {
                return false;
            }
        }
        return true;
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
        std::string result(path);
        size_t pos = 0;
        while ((pos = result.find('\\', pos)) != std::string::npos) {
            result.replace(pos, 1, "/");
            pos += 1;
        }
        return result;
    }

    std::string StringUtil::ToWindowsPath(std::string_view path) {
        std::string result(path);
        size_t pos = 0;
        while ((pos = result.find('/', pos)) != std::string::npos) {
            result.replace(pos, 1, "\\");
            pos += 1;
        }
        return result;
    }

    std::string StringUtil::NormalizePath(std::string path) {
        std::replace(path.begin(), path.end(), '\\', '/');

        std::vector<std::string> components;
        bool isAbsolute = path.starts_with('/');
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
                } else {
                    components.emplace_back(component);
                }
            }
            start = pos + 1;
        }

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

        if (result.empty()) {
            result = isAbsolute ? "/" : ".";
        }

        return result;
    }

    std::string StringUtil::URLEncode(std::string_view input) {
        std::string encoded;
        encoded.reserve(input.size() * 3);

        for (unsigned char c : input) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += c;
            } else {
                encoded += '%';
                encoded += toHex(c >> 4);
                encoded += toHex(c & 0xF);
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
            result += toHex(byte >> 4);
            result += toHex(byte & 0xF);
        }
        return result;
    }

    std::uint32_t StringUtil::HashFNV1a(std::string_view str) {
        constexpr std::uint32_t fnv_prime = 16777619u;
        constexpr std::uint32_t fnv_offset_basis = 2166136261u;

        std::uint32_t hash = fnv_offset_basis;

        for (auto c : str) {
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

        size_t start = 0;
        for (size_t pos = str.find(delim); pos != std::string_view::npos; pos = str.find(delim, start)) {
            result.emplace_back(str.substr(start, pos - start));
            start = pos + 1;
        }

        if (start <= str.size()) {
            result.emplace_back(str.substr(start));
        }

        return result;
    }

    std::string StringUtil::ToLower(std::string_view str) {
        std::string out(str);
        for (auto& c : out) {
            c = static_cast<unsigned char>(std::tolower(c));
        }
        return out;
    }

    std::string StringUtil::ToUpper(std::string_view str) {
        std::string out(str);
        for (auto& c : out) {
            c = static_cast<unsigned char>(std::toupper(c));
        }
        return out;
    }

    std::string StringUtil::ReplaceAll(std::string_view str, std::string_view from, std::string_view to) {
        if (from.empty() || str.empty()) return std::string(str);

        std::string result;
        result.reserve(str.size());

        size_t pos = 0;
        size_t prev_pos = 0;

        while ((pos = str.find(from, prev_pos)) != std::string_view::npos) {
            result.append(str.substr(prev_pos, pos - prev_pos));
            result.append(to);
            prev_pos = pos + from.length();
        }

        result.append(str.substr(prev_pos));
        return result;
    }

    bool StringUtil::Contains(std::string_view str, std::string_view substr) {
        return str.find(substr) != std::string_view::npos;
    }

    std::string StringUtil::Trim(std::string_view str) {
        size_t start = 0;
        while (start < str.size() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r')) {
            start++;
        }

        size_t end = str.size();
        while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t' || str[end - 1] == '\n' || str[end - 1] == '\r')) {
            end--;
        }

        return std::string(str.substr(start, end - start));
    }

    std::vector<std::string> StringUtil::SplitLines(std::string_view text) {
        std::vector<std::string> lines;
        size_t start = 0;
        size_t pos;

        while ((pos = text.find_first_of("\r\n", start)) != std::string_view::npos) {
            lines.emplace_back(text.substr(start, pos - start));
            start = pos + 1;
            if (pos + 1 < text.size() && text[pos] == '\r' && text[pos + 1] == '\n') {
                start++;
            }
        }

        if (start < text.size()) {
            lines.emplace_back(text.substr(start));
        }

        return lines;
    }

    bool StringUtil::WildcardMatch(std::string_view str, std::string_view pattern) {
        auto s = str.begin();
        auto p = pattern.begin();
        const auto s_end = str.end();
        const auto p_end = pattern.end();

        auto star_p = p_end;
        auto star_s = s_end;

        while (s != s_end) {
            if (p != p_end && (*p == '?' || *p == *s)) {
                ++s;
                ++p;
            } else if (p != p_end && *p == '*') {
                star_p = ++p;
                star_s = s;
            } else if (star_p != p_end) {
                p = star_p;
                s = ++star_s;
            } else {
                return false;
            }
        }

        while (p != p_end && *p == '*') ++p;

        return p == p_end;
    }

    std::string StringUtil::RegexReplace(std::string_view str, std::string_view pattern, std::string_view replacement) {
        if (pattern == "*") {
            return std::string(replacement);
        }
        return ReplaceAll(str, pattern, replacement);
    }

    bool StringUtil::HasUTF8BOM(std::span<unsigned char> data) {
        constexpr unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        if (data.size() < 3) return false;
        return data[0] == bom[0] && data[1] == bom[1] && data[2] == bom[2];
    }

    std::string StringUtil::DetectEncoding(std::span<const unsigned char> data) {
        if (data.size() >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            return "utf-8-bom";
        }
        if (data.size() >= 2) {
            if (data[0] == 0xFF && data[1] == 0xFE) return "utf-16le";
            if (data[0] == 0xFE && data[1] == 0xFF) return "utf-16be";
        }

        if (data.empty()) {
            return "ascii";
        }

        const size_t checkLen = std::min(data.size(), size_t{4096});
        bool onlyAscii = true;

        for (size_t i = 0; i < checkLen;) {
            const unsigned char lead = data[i];

            if (lead < 0x80) {
                ++i;
                continue;
            }

            onlyAscii = false;

            int seqLen = 0;
            if ((lead & 0xE0) == 0xC0) seqLen = 2;
            else if ((lead & 0xF0) == 0xE0) seqLen = 3;
            else if ((lead & 0xF8) == 0xF0) seqLen = 4;
            else return "unknown";

            if (i + seqLen > checkLen) return "unknown";

            for (int j = 1; j < seqLen; ++j) {
                if ((data[i + j] & 0xC0) != 0x80) return "unknown";
            }

            i += seqLen;
        }

        return onlyAscii ? "ascii" : "utf-8";
    }

    bool StringUtil::ValidateUTF8(std::string_view str) {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.data());
        const size_t len = str.size();

        for (size_t i = 0; i < len; ) {
            const unsigned char lead = bytes[i];
            size_t code_length = 0;

            if (lead < 0x80) {
                code_length = 1;
            } else if ((lead & 0xE0) == 0xC0) {
                code_length = 2;
                if (lead < 0xC2) return false;
            } else if ((lead & 0xF0) == 0xE0) {
                code_length = 3;
                if (lead == 0xE0 && i + 1 < len && (bytes[i+1] & 0xE0) == 0x80) return false;
                if (lead == 0xED && i + 1 < len && (bytes[i+1] & 0xE0) == 0xA0) return false;
            } else if ((lead & 0xF8) == 0xF0) {
                code_length = 4;
                if (lead == 0xF0 && i + 1 < len && (bytes[i+1] < 0x90)) return false;
                if (lead >= 0xF5) return false;
            } else {
                return false;
            }

            if (i + code_length > len) return false;

            for (size_t j = 1; j < code_length; ++j) {
                if ((bytes[i + j] & 0xC0) != 0x80) return false;
            }

            i += code_length;
        }

        return true;
    }

    std::string StringUtil::ToPlatformPath(std::string_view path) {
#ifdef _WIN32
        return ToWindowsPath(path);
#else
        return ToStandardPath(path);
#endif
    }

    std::string StringUtil::Interpolate(std::string_view template_str,
                                       const std::unordered_map<std::string, std::string>& replacements) {
        if (template_str.empty() || replacements.empty()) {
            return std::string(template_str);
        }

        std::string result;
        result.reserve(template_str.size() * 2);

        size_t last_pos = 0;

        while (true) {
            size_t open_pos = template_str.find('{', last_pos);
            if (open_pos == std::string_view::npos) {
                break;
            }

            result.append(template_str.substr(last_pos, open_pos - last_pos));

            size_t close_pos = template_str.find('}', open_pos + 1);
            if (close_pos == std::string_view::npos) {
                result.append(template_str.substr(open_pos));
                return result;
            }

            std::string_view key = template_str.substr(open_pos + 1, close_pos - open_pos - 1);
            auto it = replacements.find(std::string(key));

            if (it != replacements.end()) {
                result.append(it->second);
            } else {
                result.append(template_str.substr(open_pos, close_pos - open_pos + 1));
            }

            last_pos = close_pos + 1;
        }

        if (last_pos < template_str.size()) {
            result.append(template_str.substr(last_pos));
        }

        return result;
    }

    constexpr int StringUtil::NumberCount(std::string_view str, char s) noexcept {
        int count = 0;
        for (char c : str) {
            if (c == s) ++count;
        }
        return count;
    }

#ifdef _WIN32
    std::string StringUtil::ToNativeEncoding(std::string_view str) {
        std::wstring wstr = EncodingUtil::UTF8ToWstring(str);
        if (wstr.empty() && !str.empty()) return {};
        return EncodingUtil::WstringToANSI(wstr);
    }

    std::string StringUtil::FromNativeEncoding(std::string_view str) {
        std::wstring wstr = EncodingUtil::ANSIToWstring(str);
        if (wstr.empty() && !str.empty()) return {};
        return EncodingUtil::WstringToUTF8(wstr);
    }
#endif
}
