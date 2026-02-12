#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

namespace shine {
namespace constexpr_ {

// ============================================================
// constexpr_bitset - 编译期位集
// 
// 特性：
// - 完全 constexpr 支持
// - 编译期位操作
// - 与枚举标志无缝集成
// - 高性能位运算
// ============================================================

namespace detail {

// 计算需要的存储单元数量
template <std::size_t Bits, std::size_t BitsPerUnit = 64>
constexpr std::size_t units_needed = (Bits + BitsPerUnit - 1) / BitsPerUnit;

// 掩码计算
constexpr std::uint64_t make_mask(std::size_t start, std::size_t count) {
    if (start >= 64) return 0;
    count = (std::min)(count, 64ULL - start);
    if (count == 0) return 0;
    return ((~0ULL) << start) & ((~0ULL) >> (64 - start - count));
}

} // namespace detail

// ============================================================
// constexpr_bitset 主模板
// ============================================================
template <std::size_t N>
class constexpr_bitset {
    static_assert(N > 0, "constexpr_bitset size must be greater than 0");

public:
    // ==================== 类型定义 ====================

    using self_type = constexpr_bitset<N>;
    using storage_type = std::uint64_t;
    static constexpr std::size_t bits_per_unit = 64;
    static constexpr std::size_t storage_size = detail::units_needed<N>;
    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

private:
    std::array<storage_type, storage_size> data_{};

    // 内部索引计算
    static constexpr std::size_t unit_index(std::size_t pos) { return pos / bits_per_unit; }
    static constexpr std::size_t bit_index(std::size_t pos) { return pos % bits_per_unit; }

public:
    // ==================== 构造函数 ====================

    constexpr constexpr_bitset() = default;

    // 从整数值构造
    constexpr explicit constexpr_bitset(std::uint64_t value) {
        if constexpr (storage_size > 0) {
            data_[0] = value;
            for (std::size_t i = 1; i < storage_size; ++i) {
                data_[i] = 0;
            }
        }
    }

    // 从字符串构造
    constexpr explicit constexpr_bitset(std::string_view str) {
        std::size_t pos = 0;
        for (char c : str) {
            if (pos >= N) break;
            if (c == '1') {
                set(pos);
            } else if (c == '0') {
                reset(pos);
            }
            ++pos;
        }
    }

    // 从数组构造
    constexpr explicit constexpr_bitset(std::array<storage_type, storage_size> const& arr)
        : data_(arr) {}

    // ==================== 元素访问 ====================

    // 检查指定位是否设置
    [[msvc::forceinline]]
    constexpr bool test(std::size_t pos) const noexcept {
        if (pos >= N) return false;
        return (data_[unit_index(pos)] >> bit_index(pos)) & 1;
    }

    // 安全检查版本
    constexpr bool test_checked(std::size_t pos) const {
        if (pos >= N) {
            throw std::out_of_range("constexpr_bitset::test: position out of range");
        }
        return test(pos);
    }

    // 全部为 1
    constexpr bool all() const noexcept {
        // 检查完整单元
        for (std::size_t i = 0; i < storage_size - 1; ++i) {
            if (data_[i] != ~storage_type{0}) return false;
        }
        // 检查最后一个单元
        constexpr std::size_t last_bits = N % bits_per_unit;
        if constexpr (last_bits == 0) {
            return data_[storage_size - 1] == ~storage_type{0};
        } else {
            storage_type mask = (storage_type{1} << last_bits) - 1;
            return (data_[storage_size - 1] & mask) == mask;
        }
    }

    // 任意位为 1
    constexpr bool any() const noexcept {
        for (auto unit : data_) {
            if (unit != 0) return true;
        }
        return false;
    }

    // 全部为 0
    constexpr bool none() const noexcept {
        return !any();
    }

    // 统计 1 的数量
    constexpr std::size_t count() const noexcept {
        std::size_t cnt = 0;
        for (auto unit : data_) {
            // popcount 算法
            while (unit) {
                unit &= unit - 1;
                ++cnt;
            }
        }
        return cnt;
    }

    // 返回位数
    static constexpr std::size_t size() noexcept { return N; }

    // ==================== 修改操作 ====================

    // 设置所有位
    constexpr constexpr_bitset& set() noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] = ~storage_type{0};
        }
        // 清除超出 N 的位
        constexpr std::size_t last_bits = N % bits_per_unit;
        if constexpr (last_bits != 0) {
            data_[storage_size - 1] &= (storage_type{1} << last_bits) - 1;
        }
        return *this;
    }

    // 设置指定位
    constexpr constexpr_bitset& set(std::size_t pos) noexcept {
        if (pos < N) {
            data_[unit_index(pos)] |= (storage_type{1} << bit_index(pos));
        }
        return *this;
    }

    constexpr constexpr_bitset& set(std::size_t pos, bool value) noexcept {
        if (pos < N) {
            if (value) {
                data_[unit_index(pos)] |= (storage_type{1} << bit_index(pos));
            } else {
                data_[unit_index(pos)] &= ~(storage_type{1} << bit_index(pos));
            }
        }
        return *this;
    }

    constexpr constexpr_bitset& set_checked(std::size_t pos, bool value = true) {
        if (pos >= N) {
            throw std::out_of_range("constexpr_bitset::set: position out of range");
        }
        return set(pos, value);
    }

    // 重置所有位
    constexpr constexpr_bitset& reset() noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] = 0;
        }
        return *this;
    }

    // 重置指定位
    constexpr constexpr_bitset& reset(std::size_t pos) noexcept {
        if (pos < N) {
            data_[unit_index(pos)] &= ~(storage_type{1} << bit_index(pos));
        }
        return *this;
    }

    // 翻转所有位
    constexpr constexpr_bitset& flip() noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] = ~data_[i];
        }
        // 清除超出 N 的位
        constexpr std::size_t last_bits = N % bits_per_unit;
        if constexpr (last_bits != 0) {
            data_[storage_size - 1] &= (storage_type{1} << last_bits) - 1;
        }
        return *this;
    }

    // 翻转指定位
    constexpr constexpr_bitset& flip(std::size_t pos) noexcept {
        if (pos < N) {
            data_[unit_index(pos)] ^= (storage_type{1} << bit_index(pos));
        }
        return *this;
    }

    // ==================== 位运算 ====================

    constexpr constexpr_bitset operator~() const noexcept {
        constexpr_bitset result = *this;
        result.flip();
        return result;
    }

    constexpr constexpr_bitset& operator&=(constexpr_bitset const& other) noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] &= other.data_[i];
        }
        return *this;
    }

    constexpr constexpr_bitset& operator|=(constexpr_bitset const& other) noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] |= other.data_[i];
        }
        return *this;
    }

    constexpr constexpr_bitset& operator^=(constexpr_bitset const& other) noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            data_[i] ^= other.data_[i];
        }
        return *this;
    }

    // 移位操作
    constexpr constexpr_bitset& operator<<=(std::size_t shift) noexcept {
        if (shift >= N) {
            reset();
            return *this;
        }
        if (shift == 0) return *this;

        std::size_t unit_shift = shift / bits_per_unit;
        std::size_t bit_shift = shift % bits_per_unit;

        if (bit_shift == 0) {
            for (std::size_t i = storage_size - 1; i >= unit_shift; --i) {
                data_[i] = data_[i - unit_shift];
            }
            for (std::size_t i = 0; i < unit_shift; ++i) {
                data_[i] = 0;
            }
        } else {
            for (std::size_t i = storage_size - 1; i > unit_shift; --i) {
                data_[i] = (data_[i - unit_shift] << bit_shift) |
                           (data_[i - unit_shift - 1] >> (bits_per_unit - bit_shift));
            }
            data_[unit_shift] = data_[0] << bit_shift;
            for (std::size_t i = 0; i < unit_shift; ++i) {
                data_[i] = 0;
            }
        }

        // 清除超出 N 的位
        constexpr std::size_t last_bits = N % bits_per_unit;
        if constexpr (last_bits != 0) {
            data_[storage_size - 1] &= (storage_type{1} << last_bits) - 1;
        }

        return *this;
    }

    constexpr constexpr_bitset& operator>>=(std::size_t shift) noexcept {
        if (shift >= N) {
            reset();
            return *this;
        }
        if (shift == 0) return *this;

        std::size_t unit_shift = shift / bits_per_unit;
        std::size_t bit_shift = shift % bits_per_unit;

        if (bit_shift == 0) {
            for (std::size_t i = 0; i < storage_size - unit_shift; ++i) {
                data_[i] = data_[i + unit_shift];
            }
            for (std::size_t i = storage_size - unit_shift; i < storage_size; ++i) {
                data_[i] = 0;
            }
        } else {
            for (std::size_t i = 0; i < storage_size - unit_shift - 1; ++i) {
                data_[i] = (data_[i + unit_shift] >> bit_shift) |
                           (data_[i + unit_shift + 1] << (bits_per_unit - bit_shift));
            }
            data_[storage_size - unit_shift - 1] = data_[storage_size - 1] >> bit_shift;
            for (std::size_t i = storage_size - unit_shift; i < storage_size; ++i) {
                data_[i] = 0;
            }
        }

        return *this;
    }

    constexpr constexpr_bitset operator<<(std::size_t shift) const noexcept {
        constexpr_bitset result = *this;
        result <<= shift;
        return result;
    }

    constexpr constexpr_bitset operator>>(std::size_t shift) const noexcept {
        constexpr_bitset result = *this;
        result >>= shift;
        return result;
    }

    // ==================== 运算符重载 ====================

    constexpr constexpr_bitset operator&(constexpr_bitset const& other) const noexcept {
        constexpr_bitset result = *this;
        result &= other;
        return result;
    }

    constexpr constexpr_bitset operator|(constexpr_bitset const& other) const noexcept {
        constexpr_bitset result = *this;
        result |= other;
        return result;
    }

    constexpr constexpr_bitset operator^(constexpr_bitset const& other) const noexcept {
        constexpr_bitset result = *this;
        result ^= other;
        return result;
    }

    // 索引访问
    constexpr bool operator[](std::size_t pos) const noexcept {
        return test(pos);
    }

    // ==================== 查找操作 ====================

    // 查找第一个设置的位
    constexpr std::size_t find_first() const noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            if (data_[i] != 0) {
                for (std::size_t j = 0; j < bits_per_unit; ++j) {
                    if ((data_[i] >> j) & 1) {
                        std::size_t pos = i * bits_per_unit + j;
                        return pos < N ? pos : npos;
                    }
                }
            }
        }
        return npos;
    }

    // 查找下一个设置的位
    constexpr std::size_t find_next(std::size_t prev) const noexcept {
        ++prev;
        if (prev >= N) return npos;

        std::size_t i = unit_index(prev);
        std::size_t j = bit_index(prev);

        // 在当前单元中搜索
        storage_type mask = ~((storage_type{1} << j) - 1);
        storage_type bits = data_[i] & mask;
        if (bits != 0) {
            while ((bits & 1) == 0) {
                bits >>= 1;
                ++j;
            }
            std::size_t pos = i * bits_per_unit + j;
            return pos < N ? pos : npos;
        }

        // 在后续单元中搜索
        for (++i; i < storage_size; ++i) {
            if (data_[i] != 0) {
                for (j = 0; j < bits_per_unit; ++j) {
                    if ((data_[i] >> j) & 1) {
                        std::size_t pos = i * bits_per_unit + j;
                        return pos < N ? pos : npos;
                    }
                }
            }
        }
        return npos;
    }

    // ==================== 转换操作 ====================

    // 转换为字符串
    constexpr std::string to_string() const {
        std::string result;
        result.reserve(N);
        for (std::size_t i = 0; i < N; ++i) {
            result.push_back(test(N - 1 - i) ? '1' : '0');
        }
        return result;
    }

    // 转换为无符号整数
    constexpr std::uint64_t to_ullong() const noexcept {
        static_assert(N <= 64, "constexpr_bitset too large for to_ullong");
        return data_[0];
    }

    // 获取原始数据
    constexpr std::array<storage_type, storage_size> const& data() const noexcept {
        return data_;
    }

    // ==================== 比较运算符 ====================

    constexpr bool operator==(constexpr_bitset const& other) const noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            if (data_[i] != other.data_[i]) return false;
        }
        return true;
    }

    constexpr auto operator<=>(constexpr_bitset const& other) const noexcept {
        for (std::size_t i = storage_size; i > 0; --i) {
            if (data_[i - 1] != other.data_[i - 1]) {
                return data_[i - 1] <=> other.data_[i - 1];
            }
        }
        return std::strong_ordering::equal;
    }

    // ==================== 交换 ====================

    constexpr void swap(constexpr_bitset& other) noexcept {
        for (std::size_t i = 0; i < storage_size; ++i) {
            storage_type tmp = data_[i];
            data_[i] = other.data_[i];
            other.data_[i] = tmp;
        }
    }
};

// ============================================================
// 位运算符
// ============================================================

template <std::size_t N>
constexpr constexpr_bitset<N> operator&(constexpr_bitset<N> const& lhs,
                                         constexpr_bitset<N> const& rhs) noexcept {
    constexpr_bitset<N> result = lhs;
    result &= rhs;
    return result;
}

template <std::size_t N>
constexpr constexpr_bitset<N> operator|(constexpr_bitset<N> const& lhs,
                                         constexpr_bitset<N> const& rhs) noexcept {
    constexpr_bitset<N> result = lhs;
    result |= rhs;
    return result;
}

template <std::size_t N>
constexpr constexpr_bitset<N> operator^(constexpr_bitset<N> const& lhs,
                                         constexpr_bitset<N> const& rhs) noexcept {
    constexpr_bitset<N> result = lhs;
    result ^= rhs;
    return result;
}

template <std::size_t N>
constexpr void swap(constexpr_bitset<N>& lhs, constexpr_bitset<N>& rhs) noexcept {
    lhs.swap(rhs);
}

// ============================================================
// constexpr_flags - 枚举标志包装器
// ============================================================

template <typename Enum, std::size_t MaxFlags = 64>
class constexpr_flags {
    static_assert(std::is_enum_v<Enum>, "Enum must be an enumeration type");

public:
    using enum_type = Enum;
    using underlying_type = std::underlying_type_t<Enum>;
    using bitset_type = constexpr_bitset<MaxFlags>;

private:
    bitset_type bits_;

    static constexpr std::size_t to_index(Enum e) noexcept {
        return static_cast<std::size_t>(static_cast<underlying_type>(e));
    }

public:
    constexpr constexpr_flags() = default;
    constexpr explicit constexpr_flags(Enum e) { set(e); }

    constexpr constexpr_flags& set(Enum e) noexcept {
        bits_.set(to_index(e));
        return *this;
    }

    constexpr constexpr_flags& reset(Enum e) noexcept {
        bits_.reset(to_index(e));
        return *this;
    }

    constexpr constexpr_flags& flip(Enum e) noexcept {
        bits_.flip(to_index(e));
        return *this;
    }

    constexpr bool test(Enum e) const noexcept {
        return bits_.test(to_index(e));
    }

    constexpr bool operator[](Enum e) const noexcept {
        return test(e);
    }

    constexpr constexpr_flags& operator|=(Enum e) noexcept {
        return set(e);
    }

    constexpr constexpr_flags& operator&=(Enum e) noexcept {
        bool has = test(e);
        bits_.reset();
        if (has) bits_.set(to_index(e));
        return *this;
    }

    constexpr constexpr_flags& operator^=(Enum e) noexcept {
        return flip(e);
    }

    constexpr constexpr_flags operator|(Enum e) const noexcept {
        constexpr_flags result = *this;
        result |= e;
        return result;
    }

    constexpr constexpr_flags operator&(Enum e) const noexcept {
        constexpr_flags result = *this;
        result &= e;
        return result;
    }

    constexpr constexpr_flags operator^(Enum e) const noexcept {
        constexpr_flags result = *this;
        result ^= e;
        return result;
    }

    constexpr bool any() const noexcept { return bits_.any(); }
    constexpr bool none() const noexcept { return bits_.none(); }
    constexpr bool all() const noexcept { return bits_.all(); }
    constexpr std::size_t count() const noexcept { return bits_.count(); }

    constexpr void clear() noexcept { bits_.reset(); }

    constexpr bitset_type const& bits() const noexcept { return bits_; }

    constexpr bool operator==(constexpr_flags const& other) const noexcept {
        return bits_ == other.bits_;
    }

    constexpr bool operator==(Enum e) const noexcept {
        return count() == 1 && test(e);
    }
};

// 标志运算符
template <typename Enum, std::size_t MaxFlags = 64>
constexpr constexpr_flags<Enum, MaxFlags> operator|(
    Enum lhs, Enum rhs) noexcept {
    constexpr_flags<Enum, MaxFlags> result;
    result.set(lhs);
    result.set(rhs);
    return result;
}

template <typename Enum, std::size_t MaxFlags = 64>
constexpr constexpr_flags<Enum, MaxFlags> operator|(
    constexpr_flags<Enum, MaxFlags> lhs, Enum rhs) noexcept {
    return lhs |= rhs;
}

// ============================================================
// 反射系统常用标志
// ============================================================

// 类型属性
enum class TypeFlags : uint32_t {
    None = 0,
    Abstract = 1 << 0,
    Final = 1 << 1,
    Serializable = 1 << 2,
    Cloneable = 1 << 3,
    Comparable = 1 << 4,
    ScriptVisible = 1 << 5,
    EditorVisible = 1 << 6,
    Component = 1 << 7,
    Singleton = 1 << 8,
    Interface = 1 << 9,
    POD = 1 << 10,
    TriviallyCopyable = 1 << 11,
    TriviallyDestructible = 1 << 12,
};

// 访问修饰符
enum class AccessFlags : uint32_t {
    None = 0,
    Public = 1 << 0,
    Protected = 1 << 1,
    Private = 1 << 2,
};

// 字段属性
enum class FieldFlags : uint32_t {
    None = 0,
    ReadOnly = 1 << 0,
    WriteOnly = 1 << 1,
    Transient = 1 << 2,
    Deprecated = 1 << 3,
    Required = 1 << 4,
    HasDefault = 1 << 5,
    NoSerialize = 1 << 6,
    Replicated = 1 << 7,
    BlueprintReadOnly = 1 << 8,
    BlueprintReadWrite = 1 << 9,
};

// 方法属性
enum class MethodFlags : uint32_t {
    None = 0,
    Static = 1 << 0,
    Virtual = 1 << 1,
    PureVirtual = 1 << 2,
    Const = 1 << 3,
    Final = 1 << 4,
    Override = 1 << 5,
    Inline = 1 << 6,
    NoExcept = 1 << 7,
    Callable = 1 << 8,
    BlueprintCallable = 1 << 9,
};

// 标志类型别名
using TypeFlagsBits = constexpr_flags<TypeFlags>;
using AccessFlagsBits = constexpr_flags<AccessFlags>;
using FieldFlagsBits = constexpr_flags<FieldFlags>;
using MethodFlagsBits = constexpr_flags<MethodFlags>;

} // namespace constexpr_

// ============================================================
// 简化别名
// ============================================================

template <std::size_t N>
using ct_bitset = constexpr_::constexpr_bitset<N>;

template <typename Enum, std::size_t MaxFlags = 64>
using ct_flags = constexpr_::constexpr_flags<Enum, MaxFlags>;

} // namespace shine

// ============================================================
// 标准库特化
// ============================================================

template <std::size_t N>
struct std::hash<shine::constexpr_::constexpr_bitset<N>> {
    constexpr std::size_t operator()(shine::constexpr_::constexpr_bitset<N> const& bs) const noexcept {
        auto const& data = bs.data();
        std::size_t result = 0;
        for (auto unit : data) {
            result ^= std::hash<std::uint64_t>{}(unit) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return result;
    }
};