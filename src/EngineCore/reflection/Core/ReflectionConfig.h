#pragma once

#include <cstdint>
#include <cstddef>

namespace shine::reflection {

    // =============================================================================
    // 编译期配置常量
    // =============================================================================

    // 内存相关配置
    inline constexpr size_t REFLECTION_SMALL_OBJECT_THRESHOLD = 256;
    inline constexpr size_t REFLECTION_DEFAULT_ALIGNMENT = alignof(std::max_align_t);
    
    // 容器大小限制
    inline constexpr size_t REFLECTION_MAX_FIELDS_PER_TYPE = 128;
    inline constexpr size_t REFLECTION_MAX_METHODS_PER_TYPE = 64;
    inline constexpr size_t REFLECTION_MAX_ENUM_VALUES = 256;
    inline constexpr size_t REFLECTION_MAX_METADATA_ENTRIES = 32;
    
    // 字符串缓冲区大小
    inline constexpr size_t REFLECTION_STRING_BUFFER_SIZE = 256;
    inline constexpr size_t REFLECTION_TEMP_BUFFER_SIZE = 64;
    
    // 哈希相关配置
    inline constexpr uint32_t REFLECTION_HASH_SEED = 2166136261u;
    inline constexpr uint32_t REFLECTION_HASH_MULTIPLIER = 16777619u;
    
    // 内存池配置
    inline constexpr size_t REFLECTION_POOL_SIZES[] = { 16, 32, 64, 128, 256 };
    inline constexpr size_t REFLECTION_NUM_POOLS = sizeof(REFLECTION_POOL_SIZES) / sizeof(REFLECTION_POOL_SIZES[0]);
    
    // 性能优化开关
    inline constexpr bool REFLECTION_ENABLE_INLINE_OPTIMIZATION = true;
    inline constexpr bool REFLECTION_ENABLE_COMPILE_TIME_HASH = true;
    inline constexpr bool REFLECTION_ENABLE_MEMORY_POOL = true;
    
    // 调试和诊断配置
    inline constexpr bool REFLECTION_ENABLE_DEBUG_CHECKS = true;
    inline constexpr bool REFLECTION_ENABLE_PERFORMANCE_COUNTERS = true;
    
    // 类型系统配置
    inline constexpr bool REFLECTION_ENABLE_RTTI_FALLBACK = false;
    inline constexpr bool REFLECTION_ENABLE_STATIC_ASSERTIONS = true;
    
    // =============================================================================
    // 编译期特征检测
    // =============================================================================
    
    template<typename T>
    inline constexpr bool is_reflection_enabled_v = 
        std::is_class_v<T> && 
        !std::is_union_v<T> && 
        std::is_standard_layout_v<T>;
        
    template<typename T>
    inline constexpr bool is_trivial_reflection_v =
        std::is_trivially_copyable_v<T> &&
        std::is_standard_layout_v<T> &&
        sizeof(T) <= REFLECTION_SMALL_OBJECT_THRESHOLD;
        
    // =============================================================================
    // 版本和兼容性信息
    // =============================================================================
    
    inline constexpr uint32_t REFLECTION_VERSION_MAJOR = 2;
    inline constexpr uint32_t REFLECTION_VERSION_MINOR = 0;
    inline constexpr uint32_t REFLECTION_VERSION_PATCH = 0;
    
    inline constexpr const char* REFLECTION_VERSION_STRING = "2.0.0";
    
    // =============================================================================
    // 平台特定优化
    // =============================================================================
    
#ifdef _MSC_VER
    inline constexpr bool REFLECTION_IS_MSVC = true;
    inline constexpr const char* REFLECTION_COMPILER = "MSVC";
#elif defined(__GNUC__)
    inline constexpr bool REFLECTION_IS_MSVC = false;
    inline constexpr const char* REFLECTION_COMPILER = "GCC";
#elif defined(__clang__)
    inline constexpr bool REFLECTION_IS_MSVC = false;
    inline constexpr const char* REFLECTION_COMPILER = "Clang";
#else
    inline constexpr bool REFLECTION_IS_MSVC = false;
    inline constexpr const char* REFLECTION_COMPILER = "Unknown";
#endif

    // =============================================================================
    // 编译期断言和约束
    // =============================================================================
    
    static_assert(REFLECTION_SMALL_OBJECT_THRESHOLD > 0, "Small object threshold must be positive");
    static_assert(REFLECTION_MAX_FIELDS_PER_TYPE > 0, "Max fields per type must be positive");
    static_assert(REFLECTION_HASH_MULTIPLIER > 0, "Hash multiplier must be positive");
    
    // 确保内存池大小递增
    static_assert([]() constexpr {
        for (size_t i = 1; i < REFLECTION_NUM_POOLS; ++i) {
            if (REFLECTION_POOL_SIZES[i] <= REFLECTION_POOL_SIZES[i-1]) {
                return false;
            }
        }
        return true;
    }(), "Memory pool sizes must be strictly increasing");

} // namespace shine::reflection