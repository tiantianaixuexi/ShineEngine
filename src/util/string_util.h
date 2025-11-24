#pragma once

#include <string>
#include <span>
#include <unordered_map>
#include <vector>





namespace shine::util
{
	struct  UTF32Char
	{
        unsigned int mCharCode;  ///< UTF-32编码点
        unsigned int mByteCount; ///< 字符在UTF-8中的字节数

        constexpr auto operator<=>(const UTF32Char&) const = default;
        constexpr bool operator==(const UTF32Char&) const = default;
    };

    class  StringUtil
    {
    public:
        // ===================== 编码转换 =====================

        static size_t UTF8ToUTF32Char(const unsigned char* src, unsigned int& dst);

        /**
         * @brief 将UTF-8字节数据转换为UTF-32字符结构数组
         * @param src UTF-8字节数据指针
         * @param dst 用于UTF32Char数组的输出缓冲区
         * @return 转换的字符数，出错时返回0
         */
        static size_t UTF8ToUTF32(const unsigned char* src, const int srcLen, unsigned int* dst);

        /**
         * @brief 将UTF-8字节数据转换为UTF-32编码点
         * @param src UTF-8字节数据指针
         * @param dst 用于UTF-32编码点数组的输出缓冲区
         * @return 转换的字符数，出错时返回0
         */
        static size_t UTF8ToUTF32(const unsigned char* src, const int srcLen, UTF32Char* dst);

        /**
         * @brief 将UTF-32编码点转换为UTF-8字节序列
         * @param src UTF-32编码点指针
         * @param dst 用于UTF-8字节序列的输出缓冲区
         * @return 写入的字节数，出错时返回0
         */
        static size_t UTF32ToUTF8(std::span<const unsigned int> src, std::string& dst);

        /**
         * @brief 将单个UTF-32编码点转换为UTF-8字节
         * @param src UTF-32编码点
         * @param dst 输出缓冲区（至少4字节）
         * @return 写入的字节数（1-4），错误时返回0
         */
        static int UTF32CharToUTF8(char32_t src, std::span<unsigned char, 4> dst);

        /**
         * @brief 将单个UTF-32编码点转换为UTF-16（可能是代理对）
         * @param u32Ch UTF-32编码点
         * @param u16Ch 输出缓冲区（至少2个short）
         * @return 写入的short数量（1或2），错误时返回0
         */
        static int UTF32CharToUTF16(char32_t u32Ch, std::span<char16_t, 2> u16Ch);

        /**
         * @brief 获取UTF-8字符从给定字节开始的字节长度
         * @param ch UTF-8字符的首字节
         * @return 字节长度（1-4），如果不是有效的首字节则返回0
         */
        static int UTF8CharLen(unsigned char ch);

        /**
         * @brief 检查给定字节是否为有效的UTF-8首字节
         * @param ch 要检查的字节
         * @return 如果是首字节则为true，否则为false
         */
        static bool IsUTF8StartByte(unsigned char ch);

        /**
         * @brief 将单个UTF-8字符序列转换为UTF-32编码点
         * @param src 包含UTF-8字符序列的数据视图
         * @param dst UTF-32编码点的输出参数
         * @return 消耗的字节数（1-4），无效序列返回0
         */
        static int UTF8ToUTF32Char(std::span<const unsigned char> src, char32_t& dst);

#ifdef _WIN32
        /**
         * @brief 将以null结尾的宽字符串（Windows Unicode）转换为UTF-32编码点
         * @param src 以null结尾的wchar_t字符串
         * @param dst UTF-32编码点的输出缓冲区
         * @return 转换的字符数
         * @note 保留Windows函数。推荐使用std::wstring和现代转换
         */
        static size_t WstringToUTF32(const wchar_t* src, std::span<char32_t> dst);

        /**
         * @brief 将std::wstring（Windows Unicode）转换为UTF-8编码的std::string
         * @param wstr 输入的宽字符串
         * @return UTF-8编码的字符串
         */
        static std::string WstringToUTF8(std::wstring_view wstr);

        /**
         * @brief 将UTF-8编码的std::string转换为std::wstring（Windows Unicode）
         * @param u8str 输入的UTF-8字符串
         * @return 宽字符串。转换错误时返回空字符串
         */
        static std::wstring UTF8ToWstring(std::string_view u8str);
        static std::string UTF8ToWstring_Navtive(std::string_view u8str);
        /**
         * @brief 将std::wstring（Windows Unicode）转换为使用系统默认代码页的ANSI字符串
         * @param wstr 输入的宽字符串
         * @return ANSI编码的字符串
         * @warning 可能有损转换。考虑使用UTF-8
         */
        static std::string WstringToANSI(std::wstring_view wstr);

        /**
         * @brief 将ANSI字符串（使用系统默认代码页）转换为std::wstring（Windows Unicode）
         * @param astr 输入的ANSI字符串
         * @return 宽字符串
         */
        static std::wstring ANSIToWstring(std::string_view astr);

        /**
         * @brief 基于对象名称的通用排序函数（假设存在mName成员）
         * @tparam T 具有`mName`成员的类型（可能是`wchar_t*`或类似类型）
         * @param l 左侧对象指针
         * @param r 右侧对象指针
         * @return 如果l->mName按字典顺序在r->mName之前，则为true（ANSI转换）
         * @warning 使用可能有损的ANSI转换进行比较。考虑支持区域设置的比较
         */
        template <typename T>
        static bool SortNameLegacy(const T& l, const T& r) // 重命名以避免冲突
        {
            // 实现细节，类似于WideStringToANSI
            std::string lName = WideStringToANSI(l->mName);
            std::string rName = WideStringToANSI(r->mName);
            return lName < rName;
        }
#endif

        // ===================== 字符串处理与查找 =====================

        /**
         * @brief 移除与前缀匹配的前缀字符
         * @param str 输入字符串
         * @param prefix 要移除的前缀
         * @return 移除前缀后的字符串
         */
        static std::string TrimStart(std::string_view str, std::string_view prefix);

        /**
         * @brief 移除与后缀匹配的后缀字符
         * @param str 输入字符串
         * @param suffix 要移除的后缀
         * @return 移除后缀后的字符串
         */
        static std::string TrimEnd(std::string_view str, std::string_view suffix);

        /**
         * @brief 检查字符串是否以特定后缀结尾
         * @param str 输入字符串
         * @param suffix 要检查的后缀
         * @return 如果str以suffix结尾，则为true
         */
        static bool EndsWith(std::string_view str, std::string_view suffix);

        /**
         * @brief 检查字符串是否以特定后缀结尾（不区分大小写）
         * @param str 输入字符串
         * @param suffix 要检查的后缀
         * @return 如果str以suffix结尾（忽略大小写），则为true
         */
        static bool EndsWithIgnoreCase(std::string_view str, std::string_view suffix);

        /**
         * @brief 检查字符串是否以特定前缀开头
         * @param str 输入字符串
         * @param prefix 要检查的前缀
         * @return 如果str以prefix开头，则为true
         */
        static bool StartsWith(std::string_view str, std::string_view prefix);

        /**
         * @brief 将路径字符串分割为其组成部分
         * @param path 路径字符串（使用/或\分隔符）
         * @return 路径组件的向量
         */
        static std::vector<std::string> SplitPathComponents(std::string_view path);

        /**
         * @brief 从路径中获取文件扩展名
         * @param path 路径字符串
         * @return 包含点的扩展名（如".txt"），如果没有扩展名则为空字符串
         */
        static std::string_view GetFileExtension(std::string_view path);

        /**
         * @brief 获取完整路径的目录部分
         * @param path 完整路径字符串
         * @return 目录路径，如果没有目录部分则为空字符串
         */
        static std::string_view GetDirectory(std::string_view path);

        /**
         * @brief 从路径字符串中移除文件扩展名
         * @param path 路径字符串
         * @return 没有扩展名的路径字符串
         */
        static std::string TrimFileExtension(std::string_view path);

        /**
         * @brief 从路径中获取不带扩展名的文件名
         * @param path 完整路径字符串
         * @return 没有目录或扩展名的文件名
         */
        static std::string GetFileNameWithoutExtension(std::string_view path);

        /**
         * @brief 将路径分隔符转换为标准的正斜杠('/')
         * @param path 路径字符串
         * @return 使用正斜杠的路径字符串
         */
        static std::string ToStandardPath(std::string_view path);

        /**
         * @brief 将路径分隔符转换为正斜杠('/', ToStandardPath的别名)
         * @param path 路径字符串
         * @return 使用正斜杠的路径字符串
         */
        static std::string ToLuaPath(std::string_view path); // Lua偏好使用'/'

        /**
         * @brief 将路径分隔符转换为Windows反斜杠('\')
         * @param path 路径字符串
         * @return 使用反斜杠的路径字符串
         */
        static std::string ToWindowsPath(std::string_view path);

        /**
         * @brief URL编码字符串
         * @param input 要编码的字符串
         * @return URL编码后的字符串
         */
        static std::string URLEncode(std::string_view input);

        /**
         * @brief URL解码字符串
         * @param input URL编码的字符串
         * @return 解码后的字符串。解码错误时返回空字符串
         */
        static std::string URLDecode(std::string_view input);

        /**
         * @brief 将字节转换为十六进制字符串表示
         * @param bytes 字节缓冲区
         * @return 十六进制字符串（大写）
         */
        static std::string BytesToHex(std::span<const unsigned char> bytes);




        // ===================== 现代C++工具（C++20/23/26）====================
        // (HashFNV1a, Split, Join, ToLower, ToUpper, Format 已经被淘汰)

        /**
         * @brief FNV-1a字符串哈希
         * @param str 输入字符串视图
         * @return 32位哈希值
         */
        static std::uint32_t HashFNV1a(std::string_view str);

        /**
         * @brief 按分隔符分割字符串
         * @param str 输入字符串视图
         * @param delim 分隔符字符
         * @return 字符串向量
         */
        static std::vector<std::string> Split(std::string_view str, char delim);

        /**
         * @brief 将字符串转换为小写
         * @param str 输入字符串视图
         * @return 小写字符串
         */
        static std::string ToLower(std::string_view str);

        /**
         * @brief 将字符串转换为大写
         * @param str 输入字符串视图
         * @return 大写字符串
         */
        static std::string ToUpper(std::string_view str);


        /**
         * @brief 使用文件系统语义规范化路径
         * @param path 输入路径字符串视图
         * @return 规范化后的路径字符串
         */
        static std::string NormalizePath(std::string path);

        /**
         * @brief 简单通配符匹配(*, ?)
         * @param str 要匹配的字符串
         * @param pattern 匹配模式
         * @return 如果匹配则为true
         */
        static bool WildcardMatch(std::string_view str, std::string_view pattern);


        /**
         * @brief 正则表达式替换
         * @param str 输入字符串视图
         * @param pattern 正则表达式模式字符串视图
         * @param replacement 替换内容字符串视图
         * @return 完成替换的字符串
         */
        static std::string RegexReplace(std::string_view str, std::string_view pattern, std::string_view replacement);

        /**
         * @brief 检查数据是否以UTF-8字节顺序标记(BOM)开头
         * @param data 数据缓冲区
         * @return 如果存在UTF-8 BOM则为true
         */
        static bool HasUTF8BOM(std::span<unsigned char> data);
        /**
         * @brief 检测字节流的编码（基本检测）
         * @param data 字节缓冲区
         * @return 检测到的编码("utf-8", "utf-16le", "utf-16be", "ascii", "unknown")
         */
        static std::string DetectEncoding(std::span<const unsigned char> data);


        /**
         * @brief 将路径分隔符转换为本机格式
         * @param path 输入路径字符串视图
         * @return 具有本机分隔符的路径字符串
         */
        static std::string ToPlatformPath(std::string_view path);

#ifdef _WIN32
        /**
         * @brief 将UTF-8字符串转换为系统本机多字节编码(ANSI)
         * @param str UTF-8字符串视图
         * @return 本机编码的字符串
         * @warning 可能有损。推荐使用UTF-8或UTF-16(wstring)
         */
        static std::string ToNativeEncoding(std::string_view str);

        /**
         * @brief 将字符串从系统本机多字节编码(ANSI)转换为UTF-8
         * @param str 本机编码的字符串视图
         * @return UTF-8字符串
         */
        static std::string FromNativeEncoding(std::string_view str);
#else
        // 在非Windows中，假定本机是UTF-8
        static std::string ToNativeEncoding(std::string_view str) { return std::string(str); }
        static std::string FromNativeEncoding(std::string_view str) { return std::string(str); }
#endif
        /**
         * @brief 简单字符串模板插值。将{key}替换为映射中的值
         * @param template_str 具有{key}占位符的模板字符串视图
         * @param replacements 用于替换的键值对映射
         * @return 插值后的字符串
         */
        static std::string Interpolate(std::string_view template_str,
            const std::unordered_map<std::string, std::string>& replacements);

        /**
         * @brief 验证字符串是否包含有效的UTF-8序列
         * @param str 输入字符串视图
         * @return 如果是有效的UTF-8则为true
         */
        static bool ValidateUTF8(std::string_view str);


        /**
         * @brief 替换字符串中所有出现的子串
         * @param str 输入字符串视图
         * @param from 要替换的子串
         * @param to 替换成的子串视图
         * @return 完成所有替换的字符串
         */
        static std::string ReplaceAll(std::string_view str, std::string_view from, std::string_view to);

        /**
         * @brief 检查字符串是否包含子串（在C++23中使用string_view::contains）
         * @param str 输入字符串视图
         * @param substr 要查找的子串
         * @return 如果在str中找到substr则为true
         */
        static bool Contains(std::string_view str, std::string_view substr);

        /**
         * @brief 从字符串中移除前后空格和制表符
         * @param str 输入字符串视图
         * @return 保留空格后的字符串
         */
        static std::string Trim(std::string_view str);

        /**
         * @brief 将字符串分割为基于换行符的行(\r或\n)
         * @param text 输入字符串视图
         * @return 每行一个字符串的向量
         */
        static std::vector<std::string> SplitLines(std::string_view text);


        static int NumberCount(std::string_view str, char s) noexcept;

        static constexpr  char ___hex[] = "0123456789ABCDEF";

    	static constexpr  bool isAlphaNumeric(unsigned char c) noexcept
    	{
            // 检查是否是大写字母 (A-Z)
            if (c >= 'A' && c <= 'Z') return true;
            // 检查是否是小写字母 (a-z)
            if (c >= 'a' && c <= 'z') return true;
            // 检查是否是数字 (0-9)
            if (c >= '0' && c <= '9') return true;
            return false;
    	}

        // 数值转换为十六进制字符
    	static constexpr char toHex(unsigned char c) noexcept
    	{
            return ___hex[c & 0xF];
    	}

        // 十六进制字符转换为数值
    	static constexpr unsigned char fromHex(unsigned char c) noexcept
    	{
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            return 0;
    	}

    private:
        // 用于不区分大小写比较的辅助函数
        struct CaseInsensitiveCompare {
            bool operator()(unsigned char c1, unsigned char c2) const {
                return std::tolower(c1) == std::tolower(c2);
            }
        };




    };

} // namespace shine::Util

