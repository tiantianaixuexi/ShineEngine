#pragma once

#include <cstdint>
#include <string_view>

namespace shine::algorithm
{
    // ============================================================
    // FNV-1a 实现 (ISO C++17, constexpr-safe)
    // ============================================================

    // --- 64-bit 参数 ---
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME        = 1099511628211ULL;

    // 核心算法：逐字节处理二进制数据（仅运行时，reinterpret_cast 不允许出现在常量表达式中）
    [[nodiscard]] inline std::uint64_t fnv1a(const std::uint8_t* data, std::size_t len, std::uint64_t hash = FNV_OFFSET_BASIS) noexcept
    {
        for (std::size_t i = 0; i < len; ++i)
        {
            hash ^= static_cast<std::uint64_t>(data[i]);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    // 编译期/运行时均可用的 64-bit 字符串版本
    [[nodiscard]] constexpr std::uint64_t hash64(std::string_view str) noexcept
    {
        std::uint64_t h = FNV_OFFSET_BASIS;
        for (unsigned char c : str)
        {
            h ^= static_cast<std::uint64_t>(c);
            h *= FNV_PRIME;
        }
        return h;
    }

    // 运行时版本（支持任意二进制数据）
    [[nodiscard]] inline std::uint64_t hash64_bytes(const void* data, std::size_t len) noexcept
    {
        return fnv1a(static_cast<const std::uint8_t*>(data), len);
    }

    // --- 32-bit 参数 ---
    constexpr std::uint32_t FNV_OFFSET_BASIS_32 = 2166136261u;
    constexpr std::uint32_t FNV_PRIME_32        = 16777619u;

    // 编译期/运行时均可用的 32-bit 字符串版本
    [[nodiscard]] constexpr std::uint32_t hash32(std::string_view str) noexcept
    {
        std::uint32_t h = FNV_OFFSET_BASIS_32;
        for (unsigned char c : str)
        {
            h ^= static_cast<std::uint32_t>(c);
            h *= FNV_PRIME_32;
        }
        return h;
    }

    // 字符串字面量操作符
    namespace literals {
        [[nodiscard]] constexpr std::uint64_t operator""_hash(const char* str, std::size_t len) noexcept
        {
            return hash64(std::string_view(str, len));
        }
    }
}
