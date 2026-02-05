#pragma once

#include <cstring>
#include <type_traits>
#include <algorithm>
#include <bit>

#include "ReflectionModernTypes.h"
#include "ReflectionConcepts.h"
#include "../../../util/EnumFlags.h"

namespace shine::reflection {

    // =============================================================================
    // 标志检查工具
    // =============================================================================
    
    // 使用 EnumFlags 系统的标志检查
    template<EnumFlags E>
    inline constexpr bool HasAllFlags(E value, E flags) noexcept {
        return HasFlag(value, flags);
    }
    
    template<EnumFlags E>
    inline constexpr bool HasAnyFlag(E value, E flags) noexcept {
        return (ToUnderlying(value) & ToUnderlying(flags)) != 0;
    }
    
    template<EnumFlags E>
    inline constexpr bool HasNoneFlags(E value, E flags) noexcept {
        return (ToUnderlying(value) & ToUnderlying(flags)) == 0;
    }

    // 高效的内存复制
    inline void FastMemcpy(void* dest, const void* src, size_t size) noexcept {
        if (size <= sizeof(uint64_t)) {
            // 小数据使用整数复制
            if (size <= sizeof(uint8_t)) {
                *static_cast<uint8_t*>(dest) = *static_cast<const uint8_t*>(src);
            } else if (size <= sizeof(uint16_t)) {
                *static_cast<uint16_t*>(dest) = *static_cast<const uint16_t*>(src);
            } else if (size <= sizeof(uint32_t)) {
                *static_cast<uint32_t*>(dest) = *static_cast<const uint32_t*>(src);
            } else {
                *static_cast<uint64_t*>(dest) = *static_cast<const uint64_t*>(src);
            }
        } else {
            // 大数据使用标准memcpy
            std::memcpy(dest, src, size);
        }
    }

    // 高效的内存比较
    inline bool FastMemcmp(const void* a, const void* b, size_t size) noexcept {
        if (size <= sizeof(uint64_t)) {
            // 小数据使用整数比较
            if (size <= sizeof(uint8_t)) {
                return *static_cast<const uint8_t*>(a) == *static_cast<const uint8_t*>(b);
            } else if (size <= sizeof(uint16_t)) {
                return *static_cast<const uint16_t*>(a) == *static_cast<const uint16_t*>(b);
            } else if (size <= sizeof(uint32_t)) {
                return *static_cast<const uint32_t*>(a) == *static_cast<const uint32_t*>(b);
            } else {
                return *static_cast<const uint64_t*>(a) == *static_cast<const uint64_t*>(b);
            }
        } else {
            // 大数据使用标准memcmp
            return std::memcmp(a, b, size) == 0;
        }
    }

    // 内存清零
    inline void FastMemzero(void* ptr, size_t size) noexcept {
        if (size <= sizeof(uint64_t)) {
            if (size <= sizeof(uint8_t)) {
                *static_cast<uint8_t*>(ptr) = 0;
            } else if (size <= sizeof(uint16_t)) {
                *static_cast<uint16_t*>(ptr) = 0;
            } else if (size <= sizeof(uint32_t)) {
                *static_cast<uint32_t*>(ptr) = 0;
            } else {
                *static_cast<uint64_t*>(ptr) = 0;
            }
        } else {
            std::memset(ptr, 0, size);
        }
    }

    // =============================================================================
    // 对齐工具
    // =============================================================================

    // 计算对齐后的大小
    template<size_t Alignment>
    consteval size_t AlignSize(size_t size) {
        static_assert(std::has_single_bit(Alignment), "Alignment must be a power of 2");
        return (size + Alignment - 1) & ~(Alignment - 1);
    }

    // 检查是否已对齐
    template<size_t Alignment>
    consteval bool IsAligned(size_t value) {
        static_assert(std::has_single_bit(Alignment), "Alignment must be a power of 2");
        return (value & (Alignment - 1)) == 0;
    }

    // 获取对齐偏移
    template<size_t Alignment>
    consteval size_t GetAlignmentOffset(size_t current_offset) {
        static_assert(std::has_single_bit(Alignment), "Alignment must be a power of 2");
        return AlignSize<Alignment>(current_offset) - current_offset;
    }

    // =============================================================================
    // 位操作工具
    // =============================================================================

    // 检查是否为2的幂
    consteval bool IsPowerOfTwo(size_t value) {
        return value > 0 && (value & (value - 1)) == 0;
    }

    // 获取下一个2的幂
    consteval size_t NextPowerOfTwo(size_t value) {
        if (value == 0) return 1;
        --value;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        if constexpr (sizeof(size_t) > 4) {
            value |= value >> 32;
        }
        return value + 1;
    }

    // 计算log2
    consteval size_t Log2(size_t value) {
        size_t result = 0;
        while (value > 1) {
            value >>= 1;
            ++result;
        }
        return result;
    }

    // =============================================================================
    // 类型萃取工具
    // =============================================================================

    // 获取成员指针的偏移量
    template<auto MemberPtr>
    consteval size_t GetMemberOffset() {
        using MemberType = decltype(MemberPtr);
        static_assert(std::is_member_object_pointer_v<MemberType>, 
                     "MemberPtr must be a member object pointer");
        
        if constexpr (std::is_member_object_pointer_v<MemberType>) {
            return offsetof(std::remove_pointer_t<MemberType>, MemberPtr);
        } else {
            return 0;
        }
    }

    // 获取类的基类偏移（如果有的话）
    template<typename Derived, typename Base>
    consteval size_t GetBaseClassOffset() {
        static_assert(std::is_base_of_v<Base, Derived>, 
                     "Base must be a base class of Derived");
        return reinterpret_cast<size_t>(
            static_cast<Base*>(reinterpret_cast<Derived*>(0x1000))) - 0x1000;
    }

    // 检查类型关系
    template<typename T, typename U>
    consteval bool IsSameOrConvertible() {
        return std::is_same_v<T, U> || std::is_convertible_v<T, U>;
    }

    // =============================================================================
    // 编译期字符串工具
    // =============================================================================

    // 编译期字符串长度
    template<size_t N>
    consteval size_t ConstexprStrlen(const char(&str)[N]) {
        size_t len = 0;
        while (len < N - 1 && str[len] != '\0') {
            ++len;
        }
        return len;
    }

    // 编译期字符串比较
    template<size_t N, size_t M>
    consteval bool ConstexprStrcmp(const char(&str1)[N], const char(&str2)[M]) {
        if (N != M) return false;
        for (size_t i = 0; i < N - 1; ++i) {
            if (str1[i] != str2[i]) return false;
            if (str1[i] == '\0') break;
        }
        return true;
    }

    // 编译期字符串哈希
    template<size_t N>
    consteval uint32_t ConstexprHash(const char(&str)[N]) {
        uint32_t hash = 2166136261u;
        for (size_t i = 0; i < N - 1; ++i) {
            hash ^= static_cast<uint8_t>(str[i]);
            hash *= 16777619u;
        }
        return hash;
    }

    // 编译期字符串连接（返回数组）
    template<size_t N, size_t M>
    consteval auto ConstexprStrcat(const char(&str1)[N], const char(&str2)[M]) {
        constexpr size_t total_len = N + M - 1; // 减去两个null终止符，加一个连接符
        std::array<char, total_len> result{};
        
        size_t pos = 0;
        for (size_t i = 0; i < N - 1; ++i) {
            result[pos++] = str1[i];
        }
        for (size_t i = 0; i < M - 1; ++i) {
            result[pos++] = str2[i];
        }
        
        return result;
    }

    // =============================================================================
    // 性能计数器工具
    // =============================================================================

#ifdef REFLECTION_ENABLE_PERFORMANCE_COUNTERS
    class PerformanceCounter {
    private:
        uint64_t count_ = 0;
        uint64_t total_cycles_ = 0;

    public:
        void Record(uint64_t cycles) {
            ++count_;
            total_cycles_ += cycles;
        }

        uint64_t GetCount() const { return count_; }
        uint64_t GetTotalCycles() const { return total_cycles_; }
        double GetAverageCycles() const { 
            return count_ > 0 ? static_cast<double>(total_cycles_) / count_ : 0.0; 
        }

        void Reset() {
            count_ = 0;
            total_cycles_ = 0;
        }
    };
#endif

    // =============================================================================
    // 调试和诊断工具
    // =============================================================================

#ifdef REFLECTION_ENABLE_DEBUG_CHECKS
    template<typename T>
    inline void DebugCheckPointer(const T* ptr, const char* context = "") {
        if (!ptr) {
            // 在实际实现中应该使用日志系统
            // Log::Error("Null pointer detected in {}", context);
        }
    }

    template<typename T>
    inline void DebugCheckType(const char* expected_type, const char* context = "") {
        const char* actual_type = typeid(T).name();
        if (std::strcmp(expected_type, actual_type) != 0) {
            // Log::Warn("Type mismatch: expected {}, got {} in {}", 
            //           expected_type, actual_type, context);
        }
    }
#else
    template<typename T>
    inline void DebugCheckPointer(const T*, const char* = "") {}

    template<typename T>
    inline void DebugCheckType(const char*, const char* = "") {}
#endif

    // =============================================================================
    // 内存泄漏检测辅助（仅调试模式）
    // =============================================================================

#if defined(REFLECTION_ENABLE_DEBUG_CHECKS) && defined(_DEBUG)
    class MemoryTracker {
    private:
        static inline size_t allocation_count_ = 0;
        static inline size_t deallocation_count_ = 0;

    public:
        static void RecordAllocation() { ++allocation_count_; }
        static void RecordDeallocation() { ++deallocation_count_; }
        
        static size_t GetNetAllocations() { 
            return allocation_count_ - deallocation_count_; 
        }
        
        static void Reset() {
            allocation_count_ = 0;
            deallocation_count_ = 0;
        }
    };
#endif

} // namespace shine::reflection