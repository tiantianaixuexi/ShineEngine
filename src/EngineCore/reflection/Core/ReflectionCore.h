#pragma once

// =============================================================================
// Reflection Core - 反射系统核心基础设施
// =============================================================================

#include "ReflectionConfig.h"
#include "ReflectionModernTypes.h"  
#include "ReflectionConcepts.h"
#include "ReflectionUtils.h"

// 内存管理模块
#include "../Memory/ReflectionMemorySystem.h"

namespace shine::reflection {

    // =============================================================================
    // 核心基础设施命名空间别名
    // =============================================================================

    namespace core {
        using namespace shine::reflection;
    }

    // =============================================================================
    // 版本信息访问器
    // =============================================================================

    inline constexpr uint32_t GetMajorVersion() { return REFLECTION_VERSION_MAJOR; }
    inline constexpr uint32_t GetMinorVersion() { return REFLECTION_VERSION_MINOR; }
    inline constexpr uint32_t GetPatchVersion() { return REFLECTION_VERSION_PATCH; }
    inline constexpr const char* GetVersionString() { return REFLECTION_VERSION_STRING; }

    // =============================================================================
    // 系统状态查询
    // =============================================================================

    inline constexpr bool IsInlineOptimizationEnabled() { 
        return REFLECTION_ENABLE_INLINE_OPTIMIZATION; 
    }
    
    inline constexpr bool IsCompileTimeHashEnabled() { 
        return REFLECTION_ENABLE_COMPILE_TIME_HASH; 
    }
    
    inline constexpr bool IsMemoryPoolEnabled() { 
        return REFLECTION_ENABLE_MEMORY_POOL; 
    }
    
    inline constexpr bool IsDebugChecksEnabled() { 
        return REFLECTION_ENABLE_DEBUG_CHECKS; 
    }

    // =============================================================================
    // 编译期系统信息
    // =============================================================================

    inline constexpr const char* GetCompilerName() { 
        return REFLECTION_COMPILER; 
    }
    
    inline constexpr bool IsMSVC() { 
        return REFLECTION_IS_MSVC; 
    }

    // =============================================================================
    // 配置查询接口
    // =============================================================================

    inline constexpr size_t GetSmallObjectThreshold() { 
        return REFLECTION_SMALL_OBJECT_THRESHOLD; 
    }
    
    inline constexpr size_t GetMaxFieldsPerType() { 
        return REFLECTION_MAX_FIELDS_PER_TYPE; 
    }
    
    inline constexpr size_t GetMaxMethodsPerType() { 
        return REFLECTION_MAX_METHODS_PER_TYPE; 
    }
    
    inline constexpr size_t GetDefaultAlignment() { 
        return REFLECTION_DEFAULT_ALIGNMENT; 
    }

    // =============================================================================
    // 内存池配置查询
    // =============================================================================

    inline constexpr size_t GetNumMemoryPools() { 
        return REFLECTION_NUM_POOLS; 
    }
    
    template<size_t Index>
    inline constexpr size_t GetMemoryPoolSize() {
        static_assert(Index < REFLECTION_NUM_POOLS, "Pool index out of range");
        return REFLECTION_POOL_SIZES[Index];
    }

    // =============================================================================
    // 哈希配置查询
    // =============================================================================

    inline constexpr uint32_t GetHashSeed() { 
        return REFLECTION_HASH_SEED; 
    }
    
    inline constexpr uint32_t GetHashMultiplier() { 
        return REFLECTION_HASH_MULTIPLIER; 
    }

    // =============================================================================
    // 系统初始化和验证
    // =============================================================================

    class SystemValidator {
    public:
        // 验证编译期配置的一致性
        static consteval bool ValidateConfiguration() {
            // 检查基本约束
            if constexpr (REFLECTION_SMALL_OBJECT_THRESHOLD == 0) return false;
            if constexpr (REFLECTION_MAX_FIELDS_PER_TYPE == 0) return false;
            if constexpr (REFLECTION_HASH_MULTIPLIER == 0) return false;
            
            // 检查内存池大小递增性
            for (size_t i = 1; i < REFLECTION_NUM_POOLS; ++i) {
                if (REFLECTION_POOL_SIZES[i] <= REFLECTION_POOL_SIZES[i-1]) {
                    return false;
                }
            }
            
            return true;
        }
        
        // 获取系统摘要信息
        static consteval auto GetSystemInfo() {
            struct SystemInfo {
                const char* version = REFLECTION_VERSION_STRING;
                const char* compiler = REFLECTION_COMPILER;
                bool is_msvc = REFLECTION_IS_MSVC;
                size_t small_object_threshold = REFLECTION_SMALL_OBJECT_THRESHOLD;
                size_t num_pools = REFLECTION_NUM_POOLS;
                bool inline_optimization = REFLECTION_ENABLE_INLINE_OPTIMIZATION;
                bool compile_time_hash = REFLECTION_ENABLE_COMPILE_TIME_HASH;
                bool memory_pool = REFLECTION_ENABLE_MEMORY_POOL;
            };
            
            return SystemInfo{};
        }
    };

    // 编译期验证
    static_assert(SystemValidator::ValidateConfiguration(), 
                  "Reflection system configuration validation failed");

    // =============================================================================
    // 便捷的类型特征访问
    // =============================================================================

    template<typename T>
    inline constexpr auto GetTypeInfo() {
        return ReflectionTypeTraits<T>{};
    }

    template<typename T>
    inline constexpr std::string_view GetTypeName() {
        return ReflectionTypeTraits<T>::name;
    }

    template<typename T>
    inline constexpr TypeId GetTypeId() {
        return ReflectionTypeTraits<T>::id;
    }

    template<typename T>
    inline constexpr size_t GetTypeSize() {
        return ReflectionTypeTraits<T>::size;
    }

    template<typename T>
    inline constexpr size_t GetTypeAlignment() {
        return ReflectionTypeTraits<T>::alignment;
    }

    template<typename T>
    inline constexpr bool IsPodType() {
        return ReflectionTypeTraits<T>::is_pod;
    }

    template<typename T>
    inline constexpr ContainerType GetContainerType() {
        return ReflectionTypeTraits<T>::container_type;
    }

    // =============================================================================
    // 编译期断言工具
    // =============================================================================

    template<bool Condition>
    struct StaticAssert {
        static_assert(Condition, "Static assertion failed");
    };

    template<>
    struct StaticAssert<true> {
        static constexpr bool value = true;
    };

    // =============================================================================
    // 系统健康检查
    // =============================================================================

    class HealthChecker {
    public:
        static constexpr bool CheckSystemHealth() {
            // 检查所有核心组件是否正确配置
            return SystemValidator::ValidateConfiguration() &&
                   (sizeof(TypeId) == 4) &&
                   (alignof(std::max_align_t) >= 8) &&
                   (REFLECTION_HASH_SEED != 0) &&
                   (REFLECTION_HASH_MULTIPLIER != 0);
        }
        
        static constexpr const char* GetHealthStatus() {
            if constexpr (CheckSystemHealth()) {
                return "Healthy";
            } else {
                return "Degraded";
            }
        }
    };

    // 编译期健康检查
    static_assert(HealthChecker::CheckSystemHealth(), 
                  "Reflection system health check failed");

} // namespace shine::reflection

// =============================================================================
// 兼容性宏定义（用于平滑过渡）
// =============================================================================

#define REFLECTION_CORE_VERSION_MAJOR shine::reflection::GetMajorVersion()
#define REFLECTION_CORE_VERSION_MINOR shine::reflection::GetMinorVersion()  
#define REFLECTION_CORE_VERSION_PATCH shine::reflection::GetPatchVersion()

#define REFLECTION_IS_MSVC_COMPILER shine::reflection::IsMSVC()
#define REFLECTION_COMPILER_NAME shine::reflection::GetCompilerName()

// 便捷的命名空间别名
namespace rf = shine::reflection;
namespace rfc = shine::reflection::core;