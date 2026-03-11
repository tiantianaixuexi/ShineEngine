#ifdef SHINE_USE_MODULE

module;

export module shine.util.string_util;

import <array>;

import <cstdint>;
import <span>;
import <string_view>;
import <unordered_map>;
import <vector>;

import "string/shine_string.h";
import "string/shine_text_view.h";

#else

#pragma once

#include <array>

#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "string/shine_string.h"
#include "string/shine_text_view.h"

#endif

namespace shine::util
{
    class StringUtil
    {
    public:
        // ===================== Common string algorithms =====================

        [[nodiscard]] static SString TrimStart(STextView str, STextView prefix)
        {
            if (prefix.empty() || !str.starts_with(prefix))
            {
                return SString::from_view(str);
            }
            return SString::from_view(str.substr(prefix.size()));
        }

        [[nodiscard]] static SString TrimEnd(STextView str, STextView suffix)
        {
            if (suffix.empty() || !str.ends_with(suffix))
            {
                return SString::from_view(str);
            }
            return SString::from_view(str.substr(0, str.size() - suffix.size()));
        }

        [[nodiscard]] static bool EndsWith(STextView str, STextView suffix, bool IgnoreCase = true)
        {
            if (!IgnoreCase)
            {
                return str.ends_with(suffix);
            }

            if (str.size() < suffix.size())
            {
                return false;
            }

            const STextView tail = str.substr(str.size() - suffix.size(), suffix.size());
            for (std::size_t i = 0; i < suffix.size(); ++i)
            {
                if (ToLowerAscii(static_cast<unsigned char>(tail[i])) !=
                    ToLowerAscii(static_cast<unsigned char>(suffix[i])))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] static bool StartsWith(STextView str, STextView prefix, bool IgnoreCase = true)
        {
            if (!IgnoreCase)
            {
                return str.starts_with(prefix);
            }

            if (str.size() < prefix.size())
            {
                return false;
            }

            const STextView head = str.substr(0, prefix.size());
            for (std::size_t i = 0; i < prefix.size(); ++i)
            {
                if (ToLowerAscii(static_cast<unsigned char>(head[i])) !=
                    ToLowerAscii(static_cast<unsigned char>(prefix[i])))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] static std::vector<SString> Split(STextView str, char delim)
        {
            std::vector<SString> result;
            if (str.empty())
            {
                return result;
            }

            int estimated_parts = 1;
            for (char c : str)
            {
                if (c == delim)
                {
                    ++estimated_parts;
                }
            }
            result.reserve(static_cast<std::size_t>(estimated_parts));

            std::size_t start = 0;
            while (true)
            {
                const std::size_t pos = str.find(delim, start);
                if (pos == STextView::npos)
                {
                    result.emplace_back(SString::from_view(str.substr(start)));
                    break;
                }

                result.emplace_back(SString::from_view(str.substr(start, pos - start)));
                start = pos + 1;
            }

            return result;
        }

        [[nodiscard]] static SString ToLower(STextView str)
        {
            SString out = SString::from_view(str);
            for (char& c : out)
            {
                c = static_cast<char>(ToLowerAscii(static_cast<unsigned char>(c)));
            }
            return out;
        }

        [[nodiscard]] static SString ToUpper(STextView str)
        {
            SString out = SString::from_view(str);
            for (char& c : out)
            {
                c = static_cast<char>(ToUpperAscii(static_cast<unsigned char>(c)));
            }
            return out;
        }

        [[nodiscard]] static SString ReplaceAll(STextView str, STextView from, STextView to)
        {
            if (str.empty() || from.empty())
            {
                return SString::from_view(str);
            }

            return SString::from_view(str).replace(from, to);
        }

        [[nodiscard]] static bool Contains(STextView str, STextView substr)
        {
            return str.contains(substr);
        }

        [[nodiscard]] static SString Trim(STextView str)
        {
            return SString::from_view(str.trim());
        }

        [[nodiscard]] static std::vector<SString> SplitLines(STextView text)
        {
            std::vector<SString> lines;
            if (text.empty())
            {
                return lines;
            }

            std::size_t start = 0;
            while (start < text.size())
            {
                const std::size_t pos = text.find_first_of(STextView::from_literal("\r\n"), start);
                if (pos == STextView::npos)
                {
                    lines.emplace_back(SString::from_view(text.substr(start)));
                    return lines;
                }

                lines.emplace_back(SString::from_view(text.substr(start, pos - start)));

                if (text[pos] == '\r' && (pos + 1) < text.size() && text[pos + 1] == '\n')
                {
                    start = pos + 2;
                }
                else
                {
                    start = pos + 1;
                }
            }

            if (start == text.size())
            {
                lines.emplace_back();
            }

            return lines;
        }

        [[nodiscard]] static SString RegexReplace(STextView str, STextView pattern, STextView replacement)
        {
            // Lightweight engine fallback:
            // treat exact "*" as "replace whole string", otherwise do literal replace-all.
            if (pattern == STextView::from_literal("*"))
            {
                return SString::from_view(replacement);
            }
            return ReplaceAll(str, pattern, replacement);
        }

        [[nodiscard]] static bool WildcardMatch(STextView str, STextView pattern)
        {
            const char* s = str.data();
            const char* p = pattern.data();
            const char* const s_end = str.data() + str.size();
            const char* const p_end = pattern.data() + pattern.size();

            const char* star_p = nullptr;
            const char* star_s = nullptr;

            while (s != s_end)
            {
                if (p != p_end && (*p == '?' || *p == *s))
                {
                    ++s;
                    ++p;
                    continue;
                }

                if (p != p_end && *p == '*')
                {
                    star_p = ++p;
                    star_s = s;
                    continue;
                }

                if (star_p != nullptr)
                {
                    p = star_p;
                    s = ++star_s;
                    continue;
                }

                return false;
            }

            while (p != p_end && *p == '*')
            {
                ++p;
            }

            return p == p_end;
        }

        // ===================== Encoding helpers =====================

        [[nodiscard]] static bool HasUTF8BOM(std::span<const unsigned char> data)
        {
            constexpr std::array<unsigned char, 3> kBom{ 0xEF, 0xBB, 0xBF };
            return data.size() >= kBom.size()
                && data[0] == kBom[0]
                && data[1] == kBom[1]
                && data[2] == kBom[2];
        }

        [[nodiscard]] static SString DetectEncoding(std::span<const unsigned char> data)
        {
            if (HasUTF8BOM(data))
            {
                return SString::from_utf8("utf-8-bom");
            }

            if (data.size() >= 2)
            {
                if (data[0] == 0xFF && data[1] == 0xFE) return SString::from_utf8("utf-16le");
                if (data[0] == 0xFE && data[1] == 0xFF) return SString::from_utf8("utf-16be");
            }

            if (data.empty())
            {
                return SString::from_utf8("ascii");
            }

            const std::size_t checkLen = data.size() < 4096 ? data.size() : 4096;
            bool onlyAscii = true;

            for (std::size_t i = 0; i < checkLen;)
            {
                const unsigned char lead = data[i];

                if (lead < 0x80)
                {
                    ++i;
                    continue;
                }

                onlyAscii = false;

                std::size_t seqLen = 0;
                if ((lead & 0xE0u) == 0xC0u) seqLen = 2;
                else if ((lead & 0xF0u) == 0xE0u) seqLen = 3;
                else if ((lead & 0xF8u) == 0xF0u) seqLen = 4;
                else return SString::from_utf8("unknown");

                if (i + seqLen > checkLen)
                {
                    return SString::from_utf8("unknown");
                }

                for (std::size_t j = 1; j < seqLen; ++j)
                {
                    if ((data[i + j] & 0xC0u) != 0x80u)
                    {
                        return SString::from_utf8("unknown");
                    }
                }

                i += seqLen;
            }

            return onlyAscii ? SString::from_utf8("ascii") : SString::from_utf8("utf-8");
        }

        [[nodiscard]] static bool ValidateUTF8(STextView str)
        {
            const std::size_t len = str.size();

            for (std::size_t i = 0; i < len;)
            {
                const unsigned char lead = static_cast<unsigned char>(str[i]);
                std::size_t code_length = 0;

                if (lead < 0x80)
                {
                    code_length = 1;
                }
                else if ((lead & 0xE0u) == 0xC0u)
                {
                    code_length = 2;
                    if (lead < 0xC2u) return false;
                }
                else if ((lead & 0xF0u) == 0xE0u)
                {
                    code_length = 3;
                    if (i + 1 >= len) return false;
                    const unsigned char b1 = static_cast<unsigned char>(str[i + 1]);
                    if (lead == 0xE0u && b1 < 0xA0u) return false;
                    if (lead == 0xEDu && b1 >= 0xA0u) return false;
                }
                else if ((lead & 0xF8u) == 0xF0u)
                {
                    code_length = 4;
                    if (i + 1 >= len) return false;
                    const unsigned char b1 = static_cast<unsigned char>(str[i + 1]);
                    if (lead == 0xF0u && b1 < 0x90u) return false;
                    if (lead == 0xF4u && b1 >= 0x90u) return false;
                    if (lead >= 0xF5u) return false;
                }
                else
                {
                    return false;
                }

                if (i + code_length > len)
                {
                    return false;
                }

                for (std::size_t j = 1; j < code_length; ++j)
                {
                    if ((static_cast<unsigned char>(str[i + j]) & 0xC0u) != 0x80u)
                    {
                        return false;
                    }
                }

                i += code_length;
            }

            return true;
        }

        // ===================== Byte / hashing / templating =====================

        [[nodiscard]] static SString BytesToHex(std::span<const unsigned char> bytes)
        {
            SString result;
            result.reserve(bytes.size() * 2);

            for (unsigned char byte : bytes)
            {
                result.push_back(toHex(static_cast<unsigned char>(byte >> 4)));
                result.push_back(toHex(static_cast<unsigned char>(byte & 0x0F)));
            }

            return result;
        }

        [[nodiscard]] static std::uint32_t HashFNV1a(STextView str)
        {
            constexpr std::uint32_t kFnvOffset = 2166136261u;
            constexpr std::uint32_t kFnvPrime  = 16777619u;

            std::uint32_t h = kFnvOffset;
            for (char c : str)
            {
                h ^= static_cast<unsigned char>(c);
                h *= kFnvPrime;
            }
            return h;
        }

        [[nodiscard]] static SString Interpolate(
            STextView template_str,
            const std::unordered_map<SString, SString>& replacements)
        {
            if (template_str.empty() || replacements.empty())
            {
                return SString::from_view(template_str);
            }

            SString result;
            result.reserve(template_str.size() + template_str.size() / 2);

            std::size_t last_pos = 0;

            while (true)
            {
                const std::size_t open_pos = template_str.find('{', last_pos);
                if (open_pos == STextView::npos)
                {
                    break;
                }

                result.append(template_str.substr(last_pos, open_pos - last_pos));

                const std::size_t close_pos = template_str.find('}', open_pos + 1);
                if (close_pos == STextView::npos)
                {
                    result.append(template_str.substr(open_pos));
                    return result;
                }

                const STextView key = template_str.substr(open_pos + 1, close_pos - open_pos - 1);
                const auto it = replacements.find(SString::from_view(key));

                if (it != replacements.end())
                {
                    result.append(it->second);
                }
                else
                {
                    result.append(template_str.substr(open_pos, close_pos - open_pos + 1));
                }

                last_pos = close_pos + 1;
            }

            if (last_pos < template_str.size())
            {
                result.append(template_str.substr(last_pos));
            }

            return result;
        }

        [[nodiscard]] static constexpr int NumberCount(STextView str, char s) noexcept
        {
            int count = 0;
            for (char c : str)
            {
                if (c == s)
                {
                    ++count;
                }
            }
            return count;
        }

        [[nodiscard]] static constexpr bool isAlphaNumeric(unsigned char c) noexcept
        {
            return (c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9');
        }

        [[nodiscard]] static constexpr char toHex(unsigned char c) noexcept
        {
            return static_cast<char>((c & 0x0F) < 10
                ? ('0' + (c & 0x0F))
                : ('A' + ((c & 0x0F) - 10)));
        }

        [[nodiscard]] static constexpr unsigned char fromHex(unsigned char c) noexcept
        {
            if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
            if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
            if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
            return 0;
        }

        [[nodiscard]] static constexpr bool isAlphaNumericHex(unsigned char c) noexcept
        {
            return (c >= '0' && c <= '9')
                || (c >= 'A' && c <= 'F')
                || (c >= 'a' && c <= 'f');
        }

#ifdef _WIN32
        [[nodiscard]] static SString WstringToUTF8(std::wstring_view wstr);
        [[nodiscard]] static std::wstring UTF8ToWstring(STextView u8str);
        [[nodiscard]] static SString ToNativeEncoding(STextView str);
        [[nodiscard]] static SString FromNativeEncoding(STextView str);
#endif

    private:
        [[nodiscard]] static constexpr unsigned char ToLowerAscii(unsigned char c) noexcept
        {
            return (c >= 'A' && c <= 'Z')
                ? static_cast<unsigned char>(c - 'A' + 'a')
                : c;
        }

        [[nodiscard]] static constexpr unsigned char ToUpperAscii(unsigned char c) noexcept
        {
            return (c >= 'a' && c <= 'z')
                ? static_cast<unsigned char>(c - 'a' + 'A')
                : c;
        }
    };
}