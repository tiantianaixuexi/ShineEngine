#pragma once

#include "shine_define.h"
#include <cstdint>
#include <compare>
#include <array>
#include <random>
#include <string>
#include <span>
#include <expected>
#include <optional>
#include <algorithm>

// 注意：本项目使用 C++23，所有 C++23 特性默认可用
// 只需针对 WASM 平台做特殊处理

namespace shine::util
{
    // 仿Unreal Engine FGuid 类的128-bit 标识符
	struct FGuid
    {
        std::uint32_t A {0};
        std::uint32_t B {0};
        std::uint32_t C {0};
        std::uint32_t D {0};

        // 构造
        constexpr FGuid() = default;
        constexpr FGuid(std::uint32_t a, std::uint32_t b, std::uint32_t c, std::uint32_t d) noexcept
            : A(a), B(b), C(c), D(d) {}

        // 验证
        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return (A | B | C | D) != 0u;
        }

        void Invalidate() noexcept { A = B = C = D = 0u; }

        // 比较
        [[nodiscard]] std::strong_ordering operator<=>(const FGuid& other) const noexcept
        {
            if (A != other.A) return A < other.A ? std::strong_ordering::less : std::strong_ordering::greater;
            if (B != other.B) return B < other.B ? std::strong_ordering::less : std::strong_ordering::greater;
            if (C != other.C) return C < other.C ? std::strong_ordering::less : std::strong_ordering::greater;
            if (D != other.D) return D < other.D ? std::strong_ordering::less : std::strong_ordering::greater;
            return std::strong_ordering::equal;
        }

        [[nodiscard]] bool operator==(const FGuid& other) const noexcept
        {
            return A == other.A && B == other.B && C == other.C && D == other.D;
        }

        // 转字节序列（网络序：大端）
        [[nodiscard]] std::array<std::uint8_t, 16> ToBytes() const noexcept
        {
            auto be32 = [](std::uint32_t v) {
                return std::array<std::uint8_t, 4>{
                    static_cast<std::uint8_t>((v >> 24) & 0xFF),
                    static_cast<std::uint8_t>((v >> 16) & 0xFF),
                    static_cast<std::uint8_t>((v >> 8) & 0xFF),
                    static_cast<std::uint8_t>(v & 0xFF)
                };
            };
            std::array<std::uint8_t, 16> out{};
            auto a = be32(A); auto b = be32(B); auto c = be32(C); auto d = be32(D);
            std::copy(a.begin(), a.end(), out.begin() + 0);
            std::copy(b.begin(), b.end(), out.begin() + 4);
            std::copy(c.begin(), c.end(), out.begin() + 8);
            std::copy(d.begin(), d.end(), out.begin() + 12);
            return out;
        }

        // 从字节序列构造（网络序：大端）
#ifdef SHINE_PLATFORM_WASM
        // WASM 兼容：使用数组引用替代 std::span
        static FGuid FromBytes(const std::array<std::uint8_t, 16>& bytes) noexcept
        {
            auto rd32 = [](const std::uint8_t* p) -> std::uint32_t {
                return (static_cast<std::uint32_t>(p[0]) << 24) |
                       (static_cast<std::uint32_t>(p[1]) << 16) |
                       (static_cast<std::uint32_t>(p[2]) << 8)  |
                       (static_cast<std::uint32_t>(p[3])      );
            };
            return FGuid{ rd32(bytes.data()+0), rd32(bytes.data()+4), rd32(bytes.data()+8), rd32(bytes.data()+12) };
        }
        
        // 重载：接受指针和大小（WASM 兼容）
        static FGuid FromBytes(const std::uint8_t* bytes, size_t size) noexcept
        {
            if (size < 16) {
                return FGuid{}; // 无效 GUID
            }
            auto rd32 = [](const std::uint8_t* p) -> std::uint32_t {
                return (static_cast<std::uint32_t>(p[0]) << 24) |
                       (static_cast<std::uint32_t>(p[1]) << 16) |
                       (static_cast<std::uint32_t>(p[2]) << 8)  |
                       (static_cast<std::uint32_t>(p[3])      );
            };
            return FGuid{ rd32(bytes+0), rd32(bytes+4), rd32(bytes+8), rd32(bytes+12) };
        }
#else
        // C++23: 使用 std::span
        static FGuid FromBytes(std::span<const std::uint8_t, 16> bytes) noexcept
        {
            auto rd32 = [](const std::uint8_t* p) -> std::uint32_t {
                return (static_cast<std::uint32_t>(p[0]) << 24) |
                       (static_cast<std::uint32_t>(p[1]) << 16) |
                       (static_cast<std::uint32_t>(p[2]) << 8)  |
                       (static_cast<std::uint32_t>(p[3])      );
            };
            return FGuid{ rd32(bytes.data()+0), rd32(bytes.data()+4), rd32(bytes.data()+8), rd32(bytes.data()+12) };
        }
#endif

        // GUID 字符串：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx（可选大写字母）
        [[nodiscard]] std::string ToString(bool withBraces = false, bool uppercase = false) const
        {
            auto bytes = ToBytes();
            auto hex = [&](std::uint8_t v){
                constexpr char lower[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
                constexpr char upper[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
                const char* tbl = uppercase ? upper : lower;
                char buf[2];
                buf[0] = tbl[(v >> 4) & 0xF];
                buf[1] = tbl[v & 0xF];
                return std::array<char,2>{buf[0], buf[1]};
            };

            std::string s;
            s.reserve(withBraces ? 38 : 36);
            if (withBraces) s.push_back('{');

            auto push2 = [&](std::uint8_t v){ auto h = hex(v); s.push_back(h[0]); s.push_back(h[1]); };
            // 8-4-4-4-12 分组
            for (int i=0;i<4;i++) push2(bytes[i]);
            s.push_back('-');
            for (int i=4;i<6;i++) push2(bytes[i]);
            s.push_back('-');
            for (int i=6;i<8;i++) push2(bytes[i]);
            s.push_back('-');
            for (int i=8;i<10;i++) push2(bytes[i]);
            s.push_back('-');
            for (int i=10;i<16;i++) push2(bytes[i]);

            if (withBraces) s.push_back('}');
            return s;
        }

        enum class ParseError { InvalidLength, InvalidChar, InvalidFormat };

#ifdef SHINE_PLATFORM_WASM
        // WASM 兼容：使用 std::optional + ParseError 输出参数
        struct ParseResult {
            std::optional<FGuid> value;
            ParseError error = ParseError::InvalidLength;
            bool has_value() const noexcept { return value.has_value(); }
            FGuid operator*() const { return *value; }
        };

        static ParseResult Parse(SString text)
        {
            ParseResult result;
            
            // 允许有或无大括号；统一去除大括号
            size_t startPos = 0;
            if (text.size() == 38 && text.front()=='{' && text.back()=='}') {
                startPos = 1;
            }
            if (text.size() != 36 + (startPos > 0 ? 2 : 0)) {
                result.error = ParseError::InvalidLength;
                return result;
            }
            // 使用偏移量访问，避免 substr 的类型问题
            const char* data = text.data() + startPos;

            const auto& hexVal = [](const char c) noexcept ->int {
                if (c>='0' && c<='9') return c - '0';
                if (c>='a' && c<='f') return c - 'a' + 10;
                if (c>='A' && c<='F') return c - 'A' + 10;
                return -1;
            };

            const auto &readByte = [&](const char hi,const  char lo) noexcept ->std::optional<std::uint8_t>{
                int h = hexVal(hi), l = hexVal(lo);
                if (h < 0 || l < 0) return std::nullopt;
                return static_cast<std::uint8_t>((h<<4) | l);
            };

            std::array<std::uint8_t,16> b{};
            // 验证分隔符位置
            if (data[8] != '-' || data[13] != '-' || data[18] != '-' || data[23] != '-') {
                result.error = ParseError::InvalidFormat;
                return result;
            }

            auto idx = 0;
            auto take2 = [&](int offset)->bool{
                auto r = readByte(data[offset], data[offset+1]);
                if (!r.has_value()) return false;
                b[idx++] = r.value();
                return true;
            };

            // 8-4-4-4-12
            for (int i=0;i<8;i+=2) {
                if (!take2(i)) {
                    result.error = ParseError::InvalidChar;
                    return result;
                }
            }
            for (int i=9;i<13;i+=2) {
                if (!take2(i)) {
                    result.error = ParseError::InvalidChar;
                    return result;
                }
            }
            for (int i=14;i<18;i+=2) {
                if (!take2(i)) {
                    result.error = ParseError::InvalidChar;
                    return result;
                }
            }
            for (int i=19;i<23;i+=2) {
                if (!take2(i)) {
                    result.error = ParseError::InvalidChar;
                    return result;
                }
            }
            for (int i=24;i<36;i+=2) {
                if (!take2(i)) {
                    result.error = ParseError::InvalidChar;
                    return result;
                }
            }

            result.value = FromBytes(b);
            return result;
        }
#else
        // C++23: 使用 std::expected
        static std::expected<FGuid, ParseError> Parse(SString text)
        {
            // 允许有或无大括号；统一去除大括号
            size_t startPos = 0;
            if (text.size() == 38 && text.front()=='{' && text.back()=='}') {
                startPos = 1;
            }
            if (text.size() != 36 + (startPos > 0 ? 2 : 0)) {
                return std::unexpected(ParseError::InvalidLength);
            }
            // 使用偏移量访问，避免 substr 的类型问题
            const char* data = text.data() + startPos;

            const auto& hexVal = [](const char c) noexcept ->int {
                if (c>='0' && c<='9') return c - '0';
                if (c>='a' && c<='f') return c - 'a' + 10;
                if (c>='A' && c<='F') return c - 'A' + 10;
                return -1;
            };

            const auto &readByte = [&](const char hi,const  char lo) noexcept ->std::expected<std::uint8_t, ParseError>{
                int h = hexVal(hi), l = hexVal(lo);
                if (h < 0 || l < 0) return std::unexpected(ParseError::InvalidChar);
                return static_cast<std::uint8_t>((h<<4) | l);
            };

            std::array<std::uint8_t,16> b{};
            // 验证分隔符位置
            if (data[8] != '-' || data[13] != '-' || data[18] != '-' || data[23] != '-')
                return std::unexpected(ParseError::InvalidFormat);

            auto idx = 0;
            auto take2 = [&](int offset)->bool{
                auto r = readByte(data[offset], data[offset+1]);
                if (!r.has_value()) return false;
                b[idx++] = r.value();
                return true;
            };

            // 8-4-4-4-12
            for (int i=0;i<8;i+=2) if (!take2(i)) return std::unexpected(ParseError::InvalidChar);
            for (int i=9;i<13;i+=2) if (!take2(i)) return std::unexpected(ParseError::InvalidChar);
            for (int i=14;i<18;i+=2) if (!take2(i)) return std::unexpected(ParseError::InvalidChar);
            for (int i=19;i<23;i+=2) if (!take2(i)) return std::unexpected(ParseError::InvalidChar);
            for (int i=24;i<36;i+=2) if (!take2(i)) return std::unexpected(ParseError::InvalidChar);

            return FromBytes(std::span<const std::uint8_t,16>(b));
        }
#endif

        // 生成随机 GUID（符合v4/variant 标准）
        static FGuid NewGuid()
        {
            std::array<std::uint8_t,16> b{};
            std::random_device rd;
            std::mt19937_64 gen{ static_cast<std::mt19937_64::result_type>( (static_cast<std::uint64_t>(rd())<<32) ^ rd()) };
            auto dist = std::uniform_int_distribution<std::uint32_t>(0, 0xFFFFFFFFu);
            for (int i=0;i<4;i++) {
                auto x = dist(gen);
                b[i*4+0] = static_cast<std::uint8_t>((x>>24)&0xFF);
                b[i*4+1] = static_cast<std::uint8_t>((x>>16)&0xFF);
                b[i*4+2] = static_cast<std::uint8_t>((x>>8 )&0xFF);
                b[i*4+3] = static_cast<std::uint8_t>((x    )&0xFF);
            }
            // 设置版本为4（字节组6的高两位）
            b[6] = static_cast<std::uint8_t>((b[6] & 0x0F) | 0x40);
            // 设置变体（字节组8的两高位为10）
            b[8] = static_cast<std::uint8_t>((b[8] & 0x3F) | 0x80);
#ifdef SHINE_PLATFORM_WASM
            return FromBytes(b);
#else
            return FromBytes(std::span<const std::uint8_t,16>(b));
#endif
        }
    };
}

// 哈希支持
namespace std
{
    template<>
    struct hash<shine::util::FGuid>
    {
        size_t operator()(const shine::util::FGuid& g) const noexcept
        {
            // 简单地将两个32-bit 合并
            std::uint64_t h1 = (static_cast<std::uint64_t>(g.A) << 32) | g.B;
            std::uint64_t h2 = (static_cast<std::uint64_t>(g.C) << 32) | g.D;
            // 64-bit 合并
            h1 ^= (h2 + 0x9e3779b97f4a7c15ULL + (h1<<6) + (h1>>2));
            return static_cast<size_t>(h1 ^ (h1>>32));
        }
    };
}

