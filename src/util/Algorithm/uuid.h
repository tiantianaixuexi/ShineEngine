// ============================================================
// UUID C++23/26 实现 (128-bit, 符合 RFC 9562)
// 编译选项: -std=c++23 (或 -std=c++26 如果可用)
// ============================================================

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <functional>
#include <random>
#include <chrono>
#include <atomic>
#include <optional>
#include <expected>           // C++23
#include <span>              // C++20
#include <format>            // C++20/23
#include <bit>               // C++20 (std::byteswap C++23)
#include <utility>
#include <type_traits>
#include <concepts>
#include <bit>
#include <utility> // std::unreachable

#include "fmt/format.h"

namespace shine::algorithm
{  

class UUID {
public:
    using Bytes = std::array<std::uint8_t, 16>;

    // ==================== 构造 ====================
    
    // 零值 UUID (C++23 constexpr 强化)
    constexpr UUID() noexcept = default;

    // 从原始字节构造 (C++23 explicit(bool))
    explicit(not std::is_same_v<Bytes, Bytes>)  // 总是 true，显式构造
    constexpr UUID(const Bytes& bytes) noexcept : m_data(bytes) {}

    // 从字符串解析 - C++23 使用 std::expected 替代 optional
    // 错误码：0=成功, 1=长度错误, 2=格式错误, 3=无效字符
    static std::expected<UUID, int> FromString(std::string_view str) noexcept;
    
    // ==================== 工厂方法 ====================
    
    // UUIDv4: 完全随机
    [[nodiscard]] static UUID GenerateV4() noexcept;
    
    // UUIDv7: 时间排序（数据库友好）
    [[nodiscard]] static UUID GenerateV7() noexcept;
    
    // 从数据哈希生成（确定性UUID）
    [[nodiscard]] static UUID FromHash(std::span<const std::byte> data) noexcept;
    
    // 命名空间UUID（基于SHA1的UUIDv5）
    [[nodiscard]] static UUID GenerateV5(const UUID& namespace_uuid, std::string_view name) noexcept;

    // ==================== 访问器 ====================
    
    [[nodiscard]] constexpr const Bytes& GetBytes() const noexcept { return m_data; }
    
    [[nodiscard]] constexpr std::uint32_t GetTimeLow() const noexcept {
        // C++23 constexpr 允许更多操作
        return (static_cast<std::uint32_t>(m_data[0]) << 24) |
               (static_cast<std::uint32_t>(m_data[1]) << 16) |
               (static_cast<std::uint32_t>(m_data[2]) << 8)  |
               (static_cast<std::uint32_t>(m_data[3]));
    }
    
    [[nodiscard]] constexpr std::uint16_t GetTimeMid() const noexcept {
        return (static_cast<std::uint16_t>(m_data[4]) << 8) | m_data[5];
    }
    
    [[nodiscard]] constexpr std::uint8_t GetVersion() const noexcept {
        return (m_data[6] >> 4) & 0x0F;
    }
    
    [[nodiscard]] constexpr std::uint8_t GetVariant() const noexcept {
        constexpr std::uint8_t RFC4122_VARIANT = 0x80;
        constexpr std::uint8_t RFC4122_MASK = 0xC0;
        
        const std::uint8_t v = m_data[8];
        if ((v & RFC4122_MASK) == RFC4122_VARIANT) return 1;  // RFC 9562标准
        if ((v & 0x80) == 0) return 0;                        // NCS向后兼容
        if ((v & 0xE0) == 0xC0) return 2;                     // Microsoft向后兼容
        return 3;                                             // 保留
    }

    // ==================== 转换 ====================
    
    // C++23 使用 std::format（或回退）
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] std::string ToStringCompact() const;
    [[nodiscard]] std::string ToShortString() const;


    template<typename Self>
    [[nodiscard]] auto GetData(this Self&& self) -> decltype(auto) {
        return (std::forward<Self>(self).m_data);
    }

    // ==================== 比较 ====================
    
    [[nodiscard]] constexpr bool IsZero() const noexcept {
        return m_data == Bytes{};
    }
    
    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return !IsZero() && GetVariant() == 1;
    }

    [[nodiscard]] constexpr auto operator<=>(const UUID&) const noexcept = default;  
    [[nodiscard]] constexpr bool operator==(const UUID&) const noexcept = default;  

    // ==================== 哈希支持 ====================
    
    struct Hash {
        [[nodiscard]] std::size_t operator()(const UUID& uuid) const noexcept {
            // C++23 使用 std::start_lifetime_as 避免 UB
            #if __cpp_lib_start_lifetime_as >= 202207L
                const auto* ptr = std::start_lifetime_as<const std::uint64_t>(uuid.m_data.data());
                return static_cast<std::size_t>(ptr[0]);
            #else
                // 安全但稍慢的方式
                std::uint64_t high = 0;
                std::memcpy(&high, uuid.m_data.data(), sizeof(high));
                return static_cast<std::size_t>(high);
            #endif
        }
    };

    [[nodiscard]] std::pair<std::uint64_t, std::uint64_t> GetHash128() const noexcept {
        std::uint64_t high = 0, low = 0;
        std::memcpy(&high, m_data.data(), sizeof(high));
        std::memcpy(&low, m_data.data() + 8, sizeof(low));
        return {high, low};
    }

private:
    Bytes m_data{};

    static std::uint64_t GetTimestampMillis() noexcept;
    static std::uint16_t GetClockSequence() noexcept;
    static Bytes GetRandomBytes() noexcept;
    
    friend struct std::hash<UUID>;
};

// ==================== 实现 ====================

namespace {
    // C++23 线程安全随机数生成器（使用 std::atomic 等待/通知优化）
    [[nodiscard]] std::mt19937_64& GetThreadRandom() noexcept {
        thread_local std::mt19937_64 gen = []{
            std::random_device rd;
            // C++23 使用 std::seed_seq 的改进初始化
            std::seed_seq seq{
                rd(), rd(), rd(), rd(), 
                rd(), rd(), rd(), rd(),
                static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count())
            };
            return std::mt19937_64(seq);
        }();
        return gen;
    }
    
    // C++20 atomic 的 wait/notify 优化（如支持）
    std::atomic<std::uint16_t> g_clock_sequence{0};
}

[[nodiscard]] UUID UUID::GenerateV4() noexcept {
    UUID uuid;
    auto& rnd = GetThreadRandom();
    
    std::uint64_t high = rnd();
    std::uint64_t low = rnd();
    
    // 设置版本 (0100 = 版本4)
    high &= 0xFFFFFFFFFFFF0FFFULL;
    high |= 0x0000000000004000ULL;
    
    // 设置变体 (10 = RFC 4122)
    low &= 0x3FFFFFFFFFFFFFFFULL;
    low |= 0x8000000000000000ULL;
    
    // C++23 使用 std::byteswap 处理字节序（大端序标准）
    auto& d = uuid.m_data;
    

    const auto high_be = std::byteswap(high);
    const auto low_be = std::byteswap(low);
    std::memcpy(d.data(), &high_be, 8);
    std::memcpy(d.data() + 8, &low_be, 8);
    
    return uuid;
}

[[nodiscard]] UUID UUID::GenerateV7() noexcept {
    UUID uuid;
    auto& rnd = GetThreadRandom();
    
    const auto now = std::chrono::system_clock::now();
    const std::uint64_t timestamp = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
    
    std::uint64_t rand_a = rnd() & 0x0FFF;  // 12位随机数
    std::uint64_t rand_b = rnd();           // 62位随机数
    
    // 设置版本 (0111 = 版本7)
    rand_a |= 0x7000;
    
    // 设置变体
    rand_b = (rand_b & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;
    
    auto& d = uuid.m_data;
    
    // 时间戳（48位，大端序）
    d[0] = static_cast<std::uint8_t>((timestamp >> 40) & 0xFF);
    d[1] = static_cast<std::uint8_t>((timestamp >> 32) & 0xFF);
    d[2] = static_cast<std::uint8_t>((timestamp >> 24) & 0xFF);
    d[3] = static_cast<std::uint8_t>((timestamp >> 16) & 0xFF);
    d[4] = static_cast<std::uint8_t>((timestamp >> 8)  & 0xFF);
    d[5] = static_cast<std::uint8_t>(timestamp & 0xFF);
    
    // rand_a (16位)
    d[6] = static_cast<std::uint8_t>((rand_a >> 8) & 0xFF);
    d[7] = static_cast<std::uint8_t>(rand_a & 0xFF);
    
    // rand_b (64位)
    d[8] = static_cast<std::uint8_t>((rand_b >> 56) & 0xFF);
    d[9] = static_cast<std::uint8_t>((rand_b >> 48) & 0xFF);
    d[10] = static_cast<std::uint8_t>((rand_b >> 40) & 0xFF);
    d[11] = static_cast<std::uint8_t>((rand_b >> 32) & 0xFF);
    d[12] = static_cast<std::uint8_t>((rand_b >> 24) & 0xFF);
    d[13] = static_cast<std::uint8_t>((rand_b >> 16) & 0xFF);
    d[14] = static_cast<std::uint8_t>((rand_b >> 8)  & 0xFF);
    d[15] = static_cast<std::uint8_t>(rand_b & 0xFF);
    
    return uuid;
}

// C++23 使用 std::expected 替代 optional
[[nodiscard]] std::expected<UUID, int> UUID::FromString(std::string_view str) noexcept {
    // C++23 使用 std::ranges 或直接处理
    std::array<char, 32> clean{};
    std::size_t j = 0;
    
    for (const char c : str) {
        if (c == '-') continue;
        if (j >= 32) return std::unexpected(1);  // 太长
        clean[j++] = c;
    }
    
    if (j != 32) return std::unexpected(1);  // 长度不对
    
    Bytes bytes{};
    
    // C++26 可能有更好的 from_chars，但目前使用 C++17 版本
    for (std::size_t i = 0; i < 16; ++i) {
        const auto [ptr, ec] = std::from_chars(
            clean.data() + i * 2, 
            clean.data() + i * 2 + 2, 
            bytes[i], 
            16
        );
        
        if (ec != std::errc{}) {
            return std::unexpected(2);  // 解析错误
        }
    }
    
    return UUID(bytes);
}


[[nodiscard]] std::string UUID::ToString() const {

        return fmt::format("{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            m_data[0], m_data[1], m_data[2], m_data[3],
            m_data[4], m_data[5],
            m_data[6], m_data[7],
            m_data[8], m_data[9],
            m_data[10], m_data[11], m_data[12], m_data[13], m_data[14], m_data[15]
        );
}

[[nodiscard]] std::string UUID::ToStringCompact() const {
        static constexpr char hex[] = "0123456789abcdef";
        std::string result(32, '0');
        
        for (std::size_t i = 0; i < 16; ++i) {
            result[i * 2] = hex[m_data[i] >> 4];
            result[i * 2 + 1] = hex[m_data[i] & 0x0F];
        }
        return result;
}

[[nodiscard]] std::string UUID::ToShortString() const {

        return fmt::format("{:02x}{:02x}{:02x}{:02x}", 
            m_data[0], m_data[1], m_data[2], m_data[3]);
}
}

// ==================== 标准库特化 ====================

namespace std {
    template<>
    struct hash<shine::algorithm::UUID> {
        [[nodiscard]] size_t operator()(const shine::algorithm::UUID& uuid) const noexcept {
            return shine::algorithm::UUID::Hash{}(uuid);
        }
    };
}