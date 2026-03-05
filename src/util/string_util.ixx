#ifdef SHINE_USE_MODULE

module;

export module shine.util.string_util;

import <string>;
import <span>;
import <unordered_map>;
import <vector>;
import <functional>;
import <cstdint>;

#else

#pragma once

#include <string>
#include <span>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>

#endif

namespace shine::util
{


    class StringUtil
    {
    public:
        // ===================== 字符串处理与查找 =====================

        /**
         * @brief 移除与前缀匹配的前缀字符
         * @param str 输入字符串
         * @param prefix 要移除的前缀
         * @return 移除前缀后的字符串
         */
        [[nodiscard]] static std::string TrimStart(std::string_view str, std::string_view prefix);

        /**
         * @brief 移除与后缀匹配的后缀字符
         * @param str 输入字符串
         * @param suffix 要移除的后缀
         * @return 移除后缀后的字符串
         */
        [[nodiscard]] static std::string TrimEnd(std::string_view str, std::string_view suffix);

        /**
         * @brief 检查字符串是否以特定后缀结尾
         * @param str        输入字符串
         * @param suffix     要检查的后缀
         * @param IgnoreCase 是否忽略大小写
         * @return 如果str以suffix结尾，则为true
         */
        [[nodiscard]] static bool EndsWith(std::string_view str, std::string_view suffix, bool IgnoreCase = true);

        /**
         * @brief 检查字符串是否以特定前缀开头
         * @param str 输入字符串
         * @param prefix 要检查的前缀
         * @return 如果str以prefix开头，则为true
         */
        [[nodiscard]] static bool StartsWith(std::string_view str, std::string_view prefix, bool IgnoreCase = true);

        /**
         * @brief 按分隔符分割字符串
         * @param str 输入字符串视图
         * @param delim 分隔符字符
         * @return 字符串向量
         */
        [[nodiscard]] static std::vector<std::string> Split(std::string_view str, char delim);

        /**
         * @brief 将字符串转换为小写
         * @param str 输入字符串视图
         * @return 小写字符串
         */
        [[nodiscard]] static std::string ToLower(std::string_view str);

        /**
         * @brief 将字符串转换为大写
         * @param str 输入字符串视图
         * @return 大写字符串
         */
        [[nodiscard]] static std::string ToUpper(std::string_view str);

        /**
         * @brief 替换字符串中所有出现的子串
         * @param str 输入字符串视图
         * @param from 要替换的子串
         * @param to 替换成的子串视图
         * @return 完成所有替换的字符串
         */
        [[nodiscard]] static std::string ReplaceAll(std::string_view str, std::string_view from, std::string_view to);

        /**
         * @brief 检查字符串是否包含子串（在C++23中使用string_view::contains）
         * @param str 输入字符串视图
         * @param substr 要查找的子串
         * @return 如果在str中找到substr则为true
         */
        [[nodiscard]] static bool Contains(std::string_view str, std::string_view substr);

        /**
         * @brief 从字符串中移除前后空格和制表符
         * @param str 输入字符串视图
         * @return 保留空格后的字符串
         */
        [[nodiscard]] static std::string Trim(std::string_view str);

        /**
         * @brief 将字符串分割为基于换行符的行(\r或\n)
         * @param text 输入字符串视图
         * @return 每行一个字符串的向量
         */
        [[nodiscard]] static std::vector<std::string> SplitLines(std::string_view text);

        /**
         * @brief 正则表达式替换
         * @param str 输入字符串视图
         * @param pattern 正则表达式模式字符串视图
         * @param replacement 替换内容字符串视图
         * @return 完成替换的字符串
         */
        [[nodiscard]] static std::string RegexReplace(std::string_view str, std::string_view pattern, std::string_view replacement);

        /**
         * @brief 简单通配符匹配(*, ?)
         * @param str 要匹配的字符串
         * @param pattern 匹配模式
         * @return 如果匹配则为true
         */
        [[nodiscard]] static bool WildcardMatch(std::string_view str, std::string_view pattern);

        // ===================== UTF-8 字符操作 =====================

        /**
         * @brief 检查字节是否为UTF-8起始字节（非续接字节）
         * @param c 字节值
         * @return 如果是起始字节则为true
         */
        [[nodiscard]] static constexpr bool IsUTF8StartByte(unsigned char c) noexcept
        {
            return (c & 0xC0) != 0x80;
        }

        /**
         * @brief 获取UTF-8字符的字节长度（根据首字节判断）
         * @param c 首字节
         * @return 字节长度（1-4），无效时返回0
         */
        [[nodiscard]] static constexpr int UTF8CharLen(unsigned char c) noexcept
        {
            if (c < 0x80) return 1;
            if ((c & 0xE0) == 0xC0) return 2;
            if ((c & 0xF0) == 0xE0) return 3;
            if ((c & 0xF8) == 0xF0) return 4;
            return 0;
        }

        /**
         * @brief 将UTF-8编码的字符解码为UTF-32码点
         * @param p 指向UTF-8字节序列的指针
         * @param avail 可用字节数
         * @return {码点, 消耗字节数}，无效时返回 {U+FFFD, 1}
         */
        [[nodiscard]] static constexpr std::pair<char32_t, int> UTF8ToUTF32Char(const char* p, size_t avail) noexcept
        {
            if (avail == 0) return {0, 0};
            unsigned char c = static_cast<unsigned char>(p[0]);
            if (c < 0x80) return {static_cast<char32_t>(c), 1};
            if ((c & 0xE0) == 0xC0 && avail >= 2) {
                char32_t cp = ((c & 0x1F) << 6) | (static_cast<unsigned char>(p[1]) & 0x3F);
                return {cp, 2};
            }
            if ((c & 0xF0) == 0xE0 && avail >= 3) {
                char32_t cp = ((c & 0x0F) << 12) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 6)
                    | (static_cast<unsigned char>(p[2]) & 0x3F);
                return {cp, 3};
            }
            if ((c & 0xF8) == 0xF0 && avail >= 4) {
                char32_t cp = ((c & 0x07) << 18) | ((static_cast<unsigned char>(p[1]) & 0x3F) << 12)
                    | ((static_cast<unsigned char>(p[2]) & 0x3F) << 6) | (static_cast<unsigned char>(p[3]) & 0x3F);
                return {cp, 4};
            }
            return {0xFFFD, 1}; // replacement char
        }

        /**
         * @brief 解码一个UTF-8码点，并移动迭代器
         * @param it 迭代器引用（会被前进）
         * @param end 结束位置
         * @return 解码的UTF-32码点
         */
        static constexpr char32_t DecodeCodePoint(const char*& it, const char* end) noexcept
        {
            if (it >= end) return 0;
            auto [cp, len] = UTF8ToUTF32Char(it, static_cast<size_t>(end - it));
            it += len;
            return cp;
        }

        /**
         * @brief 将UTF-32码点编码为UTF-8
         * @param cp UTF-32码点
         * @param out 输出缓冲区（至少4字节）
         * @return 写入的字节数
         */
        static constexpr int UTF32ToUTF8(char32_t cp, char* out) noexcept
        {
            if (cp < 0x80) {
                out[0] = static_cast<char>(cp);
                return 1;
            }
            if (cp < 0x800) {
                out[0] = static_cast<char>(0xC0 | (cp >> 6));
                out[1] = static_cast<char>(0x80 | (cp & 0x3F));
                return 2;
            }
            if (cp < 0x10000) {
                out[0] = static_cast<char>(0xE0 | (cp >> 12));
                out[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out[2] = static_cast<char>(0x80 | (cp & 0x3F));
                return 3;
            }
            out[0] = static_cast<char>(0xF0 | (cp >> 18));
            out[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out[3] = static_cast<char>(0x80 | (cp & 0x3F));
            return 4;
        }

        /**
         * @brief 将UTF-32码点编码并追加到std::string
         * @param cp UTF-32码点
         * @param out 目标字符串
         */
        static void EncodeCodePoint(char32_t cp, std::string& out)
        {
            char buf[4];
            int len = UTF32ToUTF8(cp, buf);
            out.append(buf, static_cast<size_t>(len));
        }

        // ===================== 编码检测 =====================

        /**
         * @brief 检查数据是否以UTF-8字节顺序标记(BOM)开头
         * @param data 数据缓冲区
         * @return 如果存在UTF-8 BOM则为true
         */
        [[nodiscard]] static bool HasUTF8BOM(std::span<const unsigned char> data);

        /**
         * @brief 检测字节流的编码（基本检测）
         * @param data 字节缓冲区
         * @return 检测到的编码("utf-8", "utf-16le", "utf-16be", "ascii", "unknown")
         */
        [[nodiscard]] static std::string DetectEncoding(std::span<const unsigned char> data);

        /**
         * @brief 验证字符串是否包含有效的UTF-8序列
         * @param str 输入字符串视图
         * @return 如果是有效的UTF-8则为true
         */
        [[nodiscard]] static bool ValidateUTF8(std::string_view str);

        // ===================== 路径处理 =====================

        /**
         * @brief 将路径字符串分割为其组成部分
         * @param path 路径字符串（使用/或\分隔符）
         * @return 路径组件的向量
         */
        [[nodiscard]] static std::vector<std::string> SplitPathComponents(std::string_view path);

        /**
         * @brief 从路径中获取文件扩展名
         * @param path 路径字符串
         * @return 包含点的扩展名（如".txt"），如果没有扩展名则为空字符串
         */
        [[nodiscard]] static std::string_view GetFileExtension(std::string_view path);
        [[nodiscard]] static std::string NormalizeFileExtension(std::string_view path);

        /**
         * @brief 获取完整路径的目录部分
         * @param path 完整路径字符串
         * @return 目录路径，如果没有目录部分则为空字符串
         */
        [[nodiscard]] static std::string_view GetDirectory(std::string_view path);

        /**
         * @brief 从路径字符串中移除文件扩展名
         * @param path 路径字符串
         * @return 没有扩展名的路径字符串
         */
        [[nodiscard]] static std::string TrimFileExtension(std::string_view path);

        /**
         * @brief 从路径中获取不带扩展名的文件名
         * @param path 完整路径字符串
         * @return 没有目录或扩展名的文件名
         */
        [[nodiscard]] static std::string GetFileNameWithoutExtension(std::string_view path);

        /**
         * @brief 将路径分隔符转换为标准的正斜杠('/')
         * @param path 路径字符串
         * @return 使用正斜杠的路径字符串
         */
        [[nodiscard]] static std::string ToStandardPath(std::string_view path);

        /**
         * @brief 将路径分隔符转换为Windows反斜杠('\')
         * @param path 路径字符串
         * @return 使用反斜杠的路径字符串
         */
        [[nodiscard]] static std::string ToWindowsPath(std::string_view path);

        /**
         * @brief 将路径分隔符转换为本机格式
         * @param path 输入路径字符串视图
         * @return 具有本机分隔符的路径字符串
         */
        [[nodiscard]] static std::string ToPlatformPath(std::string_view path);

        // ===================== 字节处理 =====================

        /**
         * @brief 将字节转换为十六进制字符串表示
         * @param bytes 字节缓冲区
         * @return 十六进制字符串（大写）
         */
        [[nodiscard]] static std::string BytesToHex(std::span<const unsigned char> bytes);

        /**
         * @brief FNV-1a字符串哈希
         * @param str 输入字符串视图
         * @return 32位哈希值
         */
        [[nodiscard]] static std::uint32_t HashFNV1a(std::string_view str);

        /**
         * @brief 简单字符串模板插值。将{key}替换为映射中的值
         * @param template_str 具有{key}占位符的模板字符串视图
         * @param replacements 用于替换的键值对映射
         * @return 插值后的字符串
         */
        [[nodiscard]] static std::string Interpolate(std::string_view template_str,
            const std::unordered_map<std::string, std::string>& replacements);

        /**
         * @brief 计算字符串中指定字符的数量
         * @param str 输入字符串视图
         * @param s 要计数的字符
         * @return 字符数量
         */
        [[nodiscard]] static constexpr int NumberCount(std::string_view str, char s) noexcept;

        /**
         * @brief 检查字符是否为字母数字
         * @param c 要检查的字符
         * @return 如果是字母数字则为true
         */
        [[nodiscard]] static constexpr bool isAlphaNumeric(unsigned char c) noexcept
        {
            if (c >= 'A' && c <= 'Z') return true;
            if (c >= 'a' && c <= 'z') return true;
            if (c >= '0' && c <= '9') return true;
            return false;
        }

        /**
         * @brief 数值转换为十六进制字符
         * @param c 数值（0-15）
         * @return 十六进制字符
         */
        [[nodiscard]] static constexpr char toHex(unsigned char c) noexcept
        {
            static constexpr char hex[] = "0123456789ABCDEF";
            return hex[c & 0xF];
        }

        /**
         * @brief 十六进制字符转换为数值
         * @param c 十六进制字符
         * @return 数值（0-15），如果无效则返回0
         */
        [[nodiscard]] static constexpr unsigned char fromHex(unsigned char c) noexcept
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            return 0;
        }

        /**
         * @brief 检查字符是否为有效十六进制字符
         * @param c 要检查的字符
         * @return 如果是有效十六进制字符则为true
         */
        [[nodiscard]] static constexpr bool isAlphaNumericHex(unsigned char c) noexcept
        {
            if (c >= '0' && c <= '9') return true;
            if (c >= 'A' && c <= 'F') return true;
            if (c >= 'a' && c <= 'f') return true;
            return false;
        }

#ifdef _WIN32
        /**
         * @brief 将UTF-8字符串转换为系统本机多字节编码(ANSI)
         * @param str UTF-8字符串视图
         * @return 本机编码的字符串
         * @warning 可能有损。推荐使用UTF-8或UTF-16(wstring)
         */
        [[nodiscard]] static std::string ToNativeEncoding(std::string_view str);

        /**
         * @brief 将字符串从系统本机多字节编码(ANSI)转换为UTF-8
         * @param str 本机编码的字符串视图
         * @return UTF-8字符串
         */
        [[nodiscard]] static std::string FromNativeEncoding(std::string_view str);
#else
        // 在非Windows中，假定本机是UTF-8
        [[nodiscard]] static std::string ToNativeEncoding(std::string_view str) { return std::string(str); }
        [[nodiscard]] static std::string FromNativeEncoding(std::string_view str) { return std::string(str); }
#endif

    };
}
