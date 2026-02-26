// ============================================================
// MD5 C++23/26 实现 (符合 RFC 1321)
// 特性: constexpr 计算, std::expected 错误处理, std::span 输入
// 依赖: fmt 库 (https://github.com/fmtlib/fmt)
// ============================================================

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <span>
#include <expected>
#include <bit>
#include <concepts>
#include <algorithm>
#include <ranges>
#include <limits>
#include <fstream>
#include <filesystem>

#include "fmt/format.h"
#include "fmt/ranges.h"

namespace shine::algorithm
{

// MD5 结果类型
struct MD5Digest {
    std::array<std::uint8_t, 16> bytes{};
    
    // C++23 显式 this 参数
    [[nodiscard]] constexpr auto data(this auto& self) noexcept -> decltype(auto) {
        return (self.bytes.data());
    }
    
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return 16; }
    
    // 转换为十六进制字符串 (使用 fmt 库)
    [[nodiscard]] std::string to_hex() const {
        return fmt::format("{:02x}", fmt::join(bytes, ""));
    }
    
    [[nodiscard]] std::string to_hex_upper() const {
        return fmt::format("{:02X}", fmt::join(bytes, ""));
    }
    
    // 紧凑比较
    [[nodiscard]] constexpr bool operator==(const MD5Digest&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const MD5Digest&) const noexcept = default;
};

class MD5 {
public:
    using Digest = MD5Digest;
    
    // 错误码
    enum class Error {
        None = 0,
        InvalidInput,
        Overflow,          // 消息长度超过 2^64 位
        StateError         // 在错误状态下操作
    };
    
    // ==================== 构造 ====================
    
    constexpr MD5() noexcept = default;
    
    // 禁止拷贝，允许移动
    MD5(const MD5&) = delete;
    MD5& operator=(const MD5&) = delete;
    MD5(MD5&&) noexcept = default;
    MD5& operator=(MD5&&) noexcept = default;
    
    // ==================== 核心接口 ====================
    
    // 重置状态
    constexpr void reset() noexcept {
        m_state = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
        m_count = 0;
        m_buffer.fill(0);
        m_finalized = false;
    }
    
    // 更新哈希状态 (C++23 std::span)
    [[nodiscard]] constexpr std::expected<void, Error> update(std::span<const std::byte> data) noexcept {
        if (m_finalized) return std::unexpected(Error::StateError);
        
        const auto input = std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(data.data()), 
            data.size()
        );
        
        // 检查溢出
        constexpr auto max_bits = std::numeric_limits<std::uint64_t>::max();
        if (m_count > max_bits - (input.size() << 3)) {
            return std::unexpected(Error::Overflow);
        }
        
        std::size_t index = (m_count >> 3) & 0x3F;
        m_count += static_cast<std::uint64_t>(input.size()) << 3;
        
        std::size_t part_len = 64 - index;
        std::size_t i = 0;
        
        // 处理完整块
        if (input.size() >= part_len) {
            std::copy_n(input.begin(), part_len, m_buffer.begin() + index);
            transform(m_buffer);
            
            for (i = part_len; i + 63 < input.size(); i += 64) {
                std::array<std::uint8_t, 64> block{};
                std::copy_n(input.begin() + i, 64, block.begin());
                transform(block);
            }
            index = 0;
        }
        
        // 缓存剩余字节
        if (i < input.size()) {
            std::copy_n(input.begin() + i, input.size() - i, m_buffer.begin() + index);
        }
        
        return {};
    }
    
    // 便捷接口：字符串视图
    [[nodiscard]] constexpr std::expected<void, Error> update(std::string_view str) noexcept {
        return update(std::as_bytes(std::span(str)));
    }
    
    // 最终化并获取摘要
    [[nodiscard]] constexpr std::expected<Digest, Error> finalize() noexcept {
        if (m_finalized) return std::unexpected(Error::StateError);
        
        std::size_t index = (m_count >> 3) & 0x3F;
        std::size_t pad_len = (index < 56) ? (56 - index) : (120 - index);
        
        // 构造填充数据：0x80 + 若干个 0x00
        std::array<std::uint8_t, 64> padding{};
        padding[0] = 0x80;
        
        // 追加填充（使用内部更新，不检查 finalized）
        if (auto err = update_internal(std::span(padding.data(), pad_len)); !err) {
            return std::unexpected(err.error());
        }
        
        // 追加长度（64位小端序）
        std::array<std::uint8_t, 8> bits{};
        std::uint64_t count_copy = m_count;
        for (int i = 0; i < 8; ++i) {
            bits[i] = static_cast<std::uint8_t>(count_copy >> (i * 8));
        }
        
        if (auto err = update_internal(std::as_bytes(std::span(bits))); !err) {
            return std::unexpected(err.error());
        }
        
        // 生成结果
        Digest digest{};
        for (int i = 0; i < 4; ++i) {
            digest.bytes[i * 4]     = static_cast<std::uint8_t>(m_state[i]);
            digest.bytes[i * 4 + 1] = static_cast<std::uint8_t>(m_state[i] >> 8);
            digest.bytes[i * 4 + 2] = static_cast<std::uint8_t>(m_state[i] >> 16);
            digest.bytes[i * 4 + 3] = static_cast<std::uint8_t>(m_state[i] >> 24);
        }
        
        m_finalized = true;
        return digest;
    }
    
    // ==================== 便捷静态方法 ====================
    
    // 一次性计算哈希
    [[nodiscard]] static constexpr std::expected<Digest, Error> hash(std::span<const std::byte> data) noexcept {
        MD5 ctx;
        auto result = ctx.update(data);
        if (!result) return std::unexpected(result.error());
        return ctx.finalize();
    }
    
    [[nodiscard]] static constexpr std::expected<Digest, Error> hash(std::string_view str) noexcept {
        return hash(std::as_bytes(std::span(str)));
    }
    
    // 计算文件哈希（非 constexpr）
    [[nodiscard]] static std::expected<Digest, Error> hash_file(const std::string& path) noexcept;
    
private:
    // 状态变量
    std::array<std::uint32_t, 4> m_state{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    std::uint64_t m_count{0};
    std::array<std::uint8_t, 64> m_buffer{};
    bool m_finalized{false};
    
    // 内部更新（不检查 finalized 标志，用于 finalize 中的填充）
    [[nodiscard]] constexpr std::expected<void, Error> update_internal(std::span<const std::byte> data) noexcept {
        const auto input = std::span<const std::uint8_t>(
            reinterpret_cast<const std::uint8_t*>(data.data()), 
            data.size()
        );
        
        std::size_t index = (m_count >> 3) & 0x3F;
        m_count += static_cast<std::uint64_t>(input.size()) << 3;
        
        std::size_t part_len = 64 - index;
        std::size_t i = 0;
        
        if (input.size() >= part_len) {
            std::copy_n(input.begin(), part_len, m_buffer.begin() + index);
            transform(m_buffer);
            
            for (i = part_len; i + 63 < input.size(); i += 64) {
                std::array<std::uint8_t, 64> block{};
                std::copy_n(input.begin() + i, 64, block.begin());
                transform(block);
            }
            index = 0;
        }
        
        if (i < input.size()) {
            std::copy_n(input.begin() + i, input.size() - i, m_buffer.begin() + index);
        }
        
        return {};
    }
    
    // MD5 变换函数 (64轮)
    constexpr void transform(std::span<const std::uint8_t, 64> block) noexcept {
        std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        std::array<std::uint32_t, 16> x{};
        
        // 解码 64 字节块为 16 个 32 位整数（小端序）
        for (int i = 0; i < 16; ++i) {
            x[i] = static_cast<std::uint32_t>(block[i * 4]) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 8) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 3]) << 24);
        }
        
        // MD5 基本函数
        auto F = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) -> std::uint32_t {
            return (x & y) | (~x & z);
        };
        
        auto G = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) -> std::uint32_t {
            return (x & z) | (y & ~z);
        };
        
        auto H = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) -> std::uint32_t {
            return x ^ y ^ z;
        };
        
        auto I = [](std::uint32_t x, std::uint32_t y, std::uint32_t z) -> std::uint32_t {
            return y ^ (x | ~z);
        };
        
        auto rotate_left = [](std::uint32_t value, std::uint32_t bits) -> std::uint32_t {
            return (value << bits) | (value >> (32 - bits));
        };
        
        // 使用宏或模板减少重复（这里为了 constexpr 清晰，保持展开）
        // 注意：C++23 中 lambda 不能捕获 x 数组用于 constexpr，需要改为参数传递
        
        auto FF = [&](std::uint32_t& a, std::uint32_t b, std::uint32_t c, std::uint32_t d, 
                      std::uint32_t k, std::uint32_t s, std::uint32_t ac) {
            a = rotate_left(a + F(b, c, d) + x[k] + ac, s) + b;
        };
        
        auto GG = [&](std::uint32_t& a, std::uint32_t b, std::uint32_t c, std::uint32_t d, 
                      std::uint32_t k, std::uint32_t s, std::uint32_t ac) {
            a = rotate_left(a + G(b, c, d) + x[k] + ac, s) + b;
        };
        
        auto HH = [&](std::uint32_t& a, std::uint32_t b, std::uint32_t c, std::uint32_t d, 
                      std::uint32_t k, std::uint32_t s, std::uint32_t ac) {
            a = rotate_left(a + H(b, c, d) + x[k] + ac, s) + b;
        };
        
        auto II = [&](std::uint32_t& a, std::uint32_t b, std::uint32_t c, std::uint32_t d, 
                      std::uint32_t k, std::uint32_t s, std::uint32_t ac) {
            a = rotate_left(a + I(b, c, d) + x[k] + ac, s) + b;
        };
        
        // 第1轮
        FF(a, b, c, d,  0,  7, 0xD76AA478);
        FF(d, a, b, c,  1, 12, 0xE8C7B756);
        FF(c, d, a, b,  2, 17, 0x242070DB);
        FF(b, c, d, a,  3, 22, 0xC1BDCEEE);
        FF(a, b, c, d,  4,  7, 0xF57C0FAF);
        FF(d, a, b, c,  5, 12, 0x4787C62A);
        FF(c, d, a, b,  6, 17, 0xA8304613);
        FF(b, c, d, a,  7, 22, 0xFD469501);
        FF(a, b, c, d,  8,  7, 0x698098D8);
        FF(d, a, b, c,  9, 12, 0x8B44F7AF);
        FF(c, d, a, b, 10, 17, 0xFFFF5BB1);
        FF(b, c, d, a, 11, 22, 0x895CD7BE);
        FF(a, b, c, d, 12,  7, 0x6B901122);
        FF(d, a, b, c, 13, 12, 0xFD987193);
        FF(c, d, a, b, 14, 17, 0xA679438E);
        FF(b, c, d, a, 15, 22, 0x49B40821);
        
        // 第2轮
        GG(a, b, c, d,  1,  5, 0xF61E2562);
        GG(d, a, b, c,  6,  9, 0xC040B340);
        GG(c, d, a, b, 11, 14, 0x265E5A51);
        GG(b, c, d, a,  0, 20, 0xE9B6C7AA);
        GG(a, b, c, d,  5,  5, 0xD62F105D);
        GG(d, a, b, c, 10,  9, 0x02441453);
        GG(c, d, a, b, 15, 14, 0xD8A1E681);
        GG(b, c, d, a,  4, 20, 0xE7D3FBC8);
        GG(a, b, c, d,  9,  5, 0x21E1CDE6);
        GG(d, a, b, c, 14,  9, 0xC33707D6);
        GG(c, d, a, b,  3, 14, 0xF4D50D87);
        GG(b, c, d, a,  8, 20, 0x455A14ED);
        GG(a, b, c, d, 13,  5, 0xA9E3E905);
        GG(d, a, b, c,  2,  9, 0xFCEFA3F8);
        GG(c, d, a, b,  7, 14, 0x676F02D9);
        GG(b, c, d, a, 12, 20, 0x8D2A4C8A);
        
        // 第3轮
        HH(a, b, c, d,  5,  4, 0xFFFA3942);
        HH(d, a, b, c,  8, 11, 0x8771F681);
        HH(c, d, a, b, 11, 16, 0x6D9D6122);
        HH(b, c, d, a, 14, 23, 0xFDE5380C);
        HH(a, b, c, d,  1,  4, 0xA4BEEA44);
        HH(d, a, b, c,  4, 11, 0x4BDECFA9);
        HH(c, d, a, b,  7, 16, 0xF6BB4B60);
        HH(b, c, d, a, 10, 23, 0xBEBFBC70);
        HH(a, b, c, d, 13,  4, 0x289B7EC6);
        HH(d, a, b, c,  0, 11, 0xEAA127FA);
        HH(c, d, a, b,  3, 16, 0xD4EF3085);
        HH(b, c, d, a,  6, 23, 0x04881D05);
        HH(a, b, c, d,  9,  4, 0xD9D4D039);
        HH(d, a, b, c, 12, 11, 0xE6DB99E5);
        HH(c, d, a, b, 15, 16, 0x1FA27CF8);
        HH(b, c, d, a,  2, 23, 0xC4AC5665);
        
        // 第4轮
        II(a, b, c, d,  0,  6, 0xF4292244);
        II(d, a, b, c,  7, 10, 0x432AFF97);
        II(c, d, a, b, 14, 15, 0xAB9423A7);
        II(b, c, d, a,  5, 21, 0xFC93A039);
        II(a, b, c, d, 12,  6, 0x655B59C3);
        II(d, a, b, c,  3, 10, 0x8F0CCC92);
        II(c, d, a, b, 10, 15, 0xFFEFF47D);
        II(b, c, d, a,  1, 21, 0x85845DD1);
        II(a, b, c, d,  8,  6, 0x6FA87E4F);
        II(d, a, b, c, 15, 10, 0xFE2CE6E0);
        II(c, d, a, b,  6, 15, 0xA3014314);
        II(b, c, d, a, 13, 21, 0x4E0811A1);
        II(a, b, c, d,  4,  6, 0xF7537E82);
        II(d, a, b, c, 11, 10, 0xBD3AF235);
        II(c, d, a, b,  2, 15, 0x2AD7D2BB);
        II(b, c, d, a,  9, 21, 0xEB86D391);
        
        m_state[0] += a;
        m_state[1] += b;
        m_state[2] += c;
        m_state[3] += d;
    }
};

// 文件哈希实现（非 constexpr）
inline std::expected<MD5::Digest, MD5::Error> MD5::hash_file(const std::string& path) noexcept {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::unexpected(Error::InvalidInput);
    
    MD5 ctx;
    std::array<std::byte, 8192> buffer{};
    
    while (file.good()) {
        file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        auto bytes_read = file.gcount();
        if (bytes_read > 0) {
            auto result = ctx.update(std::span(buffer.data(), static_cast<std::size_t>(bytes_read)));
            if (!result) return std::unexpected(result.error());
        }
    }
    
    return ctx.finalize();
}

} // namespace crypto

// ==================== fmt 库格式化支持 ====================

template<>
struct fmt::formatter<shine::algorithm::MD5Digest> {
    char spec = 'x'; // 默认小写
    
    constexpr auto parse(fmt::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            if (*it == 'x' || *it == 'X') spec = *it++;
        }
        if (it != ctx.end() && *it != '}') {
            throw fmt::format_error("Invalid format");
        }
        return it;
    }
    
    auto format(const shine::algorithm::MD5Digest& digest, fmt::format_context& ctx) const {
        if (spec == 'X') {
            return fmt::format_to(ctx.out(), "{}", digest.to_hex_upper());
        }
        return fmt::format_to(ctx.out(), "{}", digest.to_hex());
    }
};

template<>
struct fmt::formatter<shine::algorithm::MD5::Error> : fmt::formatter<std::string_view> {
    auto format(const shine::algorithm::MD5::Error err, fmt::format_context& ctx) const {
        std::string_view msg;
        switch (err) {
            case shine::algorithm::MD5::Error::None: msg = "none"; break;
            case shine::algorithm::MD5::Error::InvalidInput: msg = "invalid input"; break;
            case shine::algorithm::MD5::Error::Overflow: msg = "message too long"; break;
            case shine::algorithm::MD5::Error::StateError: msg = "invalid state"; break;
            default: msg = "unknown"; break;
        }
        return fmt::formatter<std::string_view>::format(msg, ctx);
    }
};

#endi