#pragma once

// =============================================================================
// Reflection CompileTime - 编译期计算系统
// =============================================================================

#include "ReflectionConstexpr.h"
#include "ReflectionHash.h"
#include "ReflectionMeta.h"

namespace shine::reflection {

    // =============================================================================
    // 编译期系统统一接口
    // =============================================================================

    namespace compile_time {
        
        // 统一的编译期计算访问点
        using Constexpr = compile_time::HashSystem;
        using Hash = hash::UnifiedHasher<>;
        using Meta = meta::MetaDataBuilder<>;
        
        // 便利的类型别名
        template<typename T>
        using TypeInfo = compile_time::TypeInfoComputer<T>;
        
        template<size_t N>
        using StringCache = compile_time::CompileTimeCache<std::string_view, HashValue, N>;

    } // namespace compile_time

    // =============================================================================
    // 编译期系统预计算数据
    // =============================================================================

    namespace precomputed {
        
        // 常用哈希值预计算
        namespace hashes {
            inline constexpr HashValue NullHash = 0;
            inline constexpr HashValue TrueHash = compile_time::Hash::Hash("true");
            inline constexpr HashValue FalseHash = compile_time::Hash::Hash("false");
            inline constexpr HashValue ZeroHash = compile_time::Hash::Hash("0");
            inline constexpr HashValue OneHash = compile_time::Hash::Hash("1");
        }
        
        // 常用字符串预计算
        namespace strings {
            inline constexpr std::string_view Empty = "";
            inline constexpr std::string_view True = "true";
            inline constexpr std::string_view False = "false";
            inline constexpr std::string_view Null = "null";
            inline constexpr std::string_view Undefined = "undefined";
        }
        
        // 常用类型信息预计算
        namespace types {
            template<typename T>
            inline constexpr auto Info = compile_time::TypeInfo<T>{};
        }

    } // namespace precomputed

    // =============================================================================
    // 编译期系统配置和状态
    // =============================================================================

    class CompileTimeSystem {
    public:
        // 系统能力查询
        static consteval bool SupportsExtendedHashing() {
            return true; // 我们的系统支持多种哈希算法
        }
        
        static consteval bool SupportsCompileTimeCaching() {
            return true; // 支持编译期缓存
        }
        
        static consteval bool SupportsPerfectHashing() {
            return true; // 支持完美哈希生成
        }
        
        static consteval bool SupportsMetaProgramming() {
            return true; // 支持丰富的元编程功能
        }

        // 获取系统特性摘要
        static consteval auto GetFeatures() {
            struct Features {
                bool extended_hashing = SupportsExtendedHashing();
                bool compile_time_caching = SupportsCompileTimeCaching();
                bool perfect_hashing = SupportsPerfectHashing();
                bool meta_programming = SupportsMetaProgramming();
                size_t max_cache_entries = REFLECTION_MAX_METADATA_ENTRIES;
                size_t hash_table_size = 256; // 示例值
            };
            
            return Features{};
        }

        // 性能基准信息
        static consteval auto GetPerformanceInfo() {
            struct PerformanceInfo {
                const char* hash_algorithm = "Multiple (FNV-1a, DJB2, Murmur)";
                size_t typical_hash_time_ns = 1; // 编译期计算，运行时接近0
                size_t memory_overhead_bytes = 0; // 编译期无运行时内存开销
                bool constant_time_lookups = true;
            };
            
            return PerformanceInfo{};
        }
    };

    // =============================================================================
    // 编译期断言和验证
    // =============================================================================

    namespace validation {
        
        // 编译期哈希质量验证
        static_assert(compile_time::HashSystem::ValidateHashDistribution(
            std::array<std::string_view, 5>{"id", "name", "value", "data", "config"}
        ), "Hash distribution quality check failed");

        // 编译期缓存功能验证
        static_assert([]() consteval {
            compile_time::CompileTimeCache<std::string_view, HashValue, 10> cache;
            return cache.Size() == 0 && !cache.Contains("test");
        }(), "Compile-time cache validation failed");

        // 元数据系统验证
        static_assert([]() consteval {
            meta::MetaDataBuilder<> builder;
            auto metadata = builder.DisplayName("Test").Editable(true).Build();
            return metadata.Contains(meta::keys::DisplayName) && 
                   metadata.Contains(meta::keys::Editable);
        }(), "Metadata system validation failed");

    } // namespace validation

} // namespace shine::reflection

// =============================================================================
// 便利的宏定义
// =============================================================================

#define REFLECTION_CT_HASH(str) \
    ::shine::reflection::compile_time::Hash::Hash(str)

#define REFLECTION_CT_TYPE_HASH(T) \
    ::shine::reflection::compile_time::Hash::HashType<T>()

#define REFLECTION_CT_META_BUILDER \
    ::shine::reflection::meta::MetaDataBuilder<>()

#define REFLECTION_CT_PRECOMPUTED_HASH(name) \
    ::shine::reflection::precomputed::hashes::name##Hash

// 类型信息快捷访问
#define REFLECTION_CT_TYPE_INFO(T) \
    ::shine::reflection::precomputed::types::Info<T>

// 便利的命名空间别名
namespace rfc = shine::reflection::compile_time;
namespace rfp = shine::reflection::precomputed;
namespace rfv = shine::reflection::validation;