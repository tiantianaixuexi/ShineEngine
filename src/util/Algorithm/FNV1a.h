#pragma once

#include <cstdint>
#include <string_view>

namespace shine::algorithm
{
    // ============================================================
// FNV-1a 64-bit 实现 (ISO C++17)
// ============================================================


    // FNV-1a 64-bit 参数
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    
    // 核心算法：逐字节处理
    [[nodiscard]] constexpr std::uint64_t fnv1a(const std::uint8_t* data, std::size_t len, std::uint64_t hash = FNV_OFFSET_BASIS) noexcept {
        for (size_t i = 0; i < len; ++i) {
            hash ^= static_cast<std::uint64_t>(data[i]);
            hash *= FNV_PRIME;
        }
        return hash;
    }

    // 编译期计算版本（C++17 constexpr）
    [[nodiscard]] constexpr uint64_t hash64(std::string_view str) noexcept {
        return fnv1a(
            reinterpret_cast<const uint8_t*>(str.data()), 
            str.size()
        );
    }

    // 运行时版本（支持二进制数据）
    [[nodiscard]] inline uint64_t hash64_bytes(const void* data, std::size_t len) noexcept {
        return fnv1a(
            static_cast<const uint8_t*>(data), 
            len
        );
    }

    // 字符串字面量操作符（C++20 可用，C++17用函数）
    namespace literals {
        [[nodiscard]] constexpr std::uint64_t operator""_hash(const char* str, std::size_t len) noexcept {
            return fnv1a(reinterpret_cast<const std::uint8_t*>(str), len);
        }
    }
}
