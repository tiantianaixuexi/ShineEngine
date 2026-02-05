#pragma once
#include <cstdint>
#include <string_view>
#include <vector>
#include "../ReflectionFlags.h"

namespace shine::reflection {

// 字段和方法的元数据容器
struct FieldMeta {
    std::string_view name;
    uint32_t offset = 0;
    uint32_t size = 0;
    PropertyFlags flags = PropertyFlags::None;
    uint64_t typeId = 0;  // 类型标识符
    
    // 存储任意元数据
    struct MetadataEntry {
        std::string_view key;
        std::string_view value;
    };
    std::vector<MetadataEntry> metadata;
    
    // 添加元数据的便捷方法
    void AddMetadata(std::string_view key, std::string_view value) {
        metadata.push_back({key, value});
    }
    
    // 获取元数据
    std::string_view GetMetadata(std::string_view key) const {
        for (const auto& entry : metadata) {
            if (entry.key == key) {
                return entry.value;
            }
        }
        return {};
    }
};

struct MethodMeta {
    std::string_view name;
    uint64_t signatureHash = 0;
    FunctionFlags flags = FunctionFlags::None;
    uint64_t returnType = 0;
    std::vector<uint64_t> paramTypes;
    
    // 方法元数据
    struct MetadataEntry {
        std::string_view key;
        std::string_view value;
    };
    std::vector<MetadataEntry> metadata;
    
    void AddMetadata(std::string_view key, std::string_view value) {
        metadata.push_back({key, value});
    }
    
    std::string_view GetMetadata(std::string_view key) const {
        for (const auto& entry : metadata) {
            if (entry.key == key) {
                return entry.value;
            }
        }
        return {};
    }
};

} // namespace shine::reflection