#pragma once

#include <array>
#include <string_view>

#include "../Core/ReflectionCore.h"

namespace shine::reflection {

    using HashValue = uint32_t;

    // =============================================================================
    // 现代化哈希系统
    // =============================================================================

    namespace hash {

        // =============================================================================
        // 哈希算法实现
        // =============================================================================

        // FNV-1a哈希算法（默认）
        class Fnv1aHasher {
        public:
            template<size_t N>
            consteval static HashValue Hash(const char(&str)[N]) {
                HashValue hash = 2166136261u; // FNV offset basis
                for (size_t i = 0; i < N - 1; ++i) {
                    hash ^= static_cast<uint8_t>(str[i]);
                    hash *= 16777619u; // FNV prime
                }
                return hash;
            }

            consteval static HashValue Hash(std::string_view str) {
                HashValue hash = 2166136261u;
                for (char c : str) {
                    hash ^= static_cast<uint8_t>(c);
                    hash *= 16777619u;
                }
                return hash;
            }
        };

        // DJB2哈希算法
        class Djb2Hasher {
        public:
            template<size_t N>
            consteval static HashValue Hash(const char(&str)[N]) {
                HashValue hash = 5381u;
                for (size_t i = 0; i < N - 1; ++i) {
                    hash = ((hash << 5) + hash) + static_cast<uint8_t>(str[i]);
                }
                return hash;
            }

            consteval static HashValue Hash(std::string_view str) {
                HashValue hash = 5381u;
                for (char c : str) {
                    hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
                }
                return hash;
            }
        };

        // MurmurHash3的简化版（适用于短字符串）
        class SimpleMurmurHasher {
        public:
            template<size_t N>
            consteval static HashValue Hash(const char(&str)[N]) {
                constexpr uint32_t seed = 2166136261u;
                constexpr uint32_t c1 = 0xcc9e2d51;
                constexpr uint32_t c2 = 0x1b873593;
                
                HashValue hash = seed;
                size_t len = N - 1;
                size_t i = 0;
                
                // 处理4字节块
                while (i + 4 <= len) {
                    uint32_t k1 = *reinterpret_cast<const uint32_t*>(str + i);
                    k1 *= c1;
                    k1 = (k1 << 15) | (k1 >> 17);
                    k1 *= c2;
                    
                    hash ^= k1;
                    hash = (hash << 13) | (hash >> 19);
                    hash = hash * 5 + 0xe6546b64;
                    
                    i += 4;
                }
                
                // 处理剩余字节
                uint32_t k1 = 0;
                switch (len & 3) {
                    case 3: k1 ^= static_cast<uint32_t>(str[i + 2]) << 16; [[fallthrough]];
                    case 2: k1 ^= static_cast<uint32_t>(str[i + 1]) << 8;  [[fallthrough]];
                    case 1:
                    {
                        k1 ^= static_cast<uint32_t>(str[i]);
                        k1 *= c1;
                        k1 = (k1 << 15) | (k1 >> 17);
                        k1 *= c2;
                        hash ^= k1;
                    }
                }
                
                // 最终混合
                hash ^= len;
                hash ^= hash >> 16;
                hash *= 0x85ebca6b;
                hash ^= hash >> 13;
                hash *= 0xc2b2ae35;
                hash ^= hash >> 16;
                
                return hash;
            }

            consteval static HashValue Hash(std::string_view str) {
                // 对于运行时字符串，使用相同的算法但运行时计算
                constexpr uint32_t seed = 2166136261u;
                constexpr uint32_t c1 = 0xcc9e2d51;
                constexpr uint32_t c2 = 0x1b873593;
                
                HashValue hash = seed;
                size_t len = str.length();
                size_t i = 0;
                
                while (i + 4 <= len) {
                    uint32_t k1 = *reinterpret_cast<const uint32_t*>(str.data() + i);
                    k1 *= c1;
                    k1 = (k1 << 15) | (k1 >> 17);
                    k1 *= c2;
                    
                    hash ^= k1;
                    hash = (hash << 13) | (hash >> 19);
                    hash = hash * 5 + 0xe6546b64;
                    
                    i += 4;
                }
                
                uint32_t k1 = 0;
                switch (len & 3) {
                    case 3: k1 ^= static_cast<uint32_t>(str[i + 2]) << 16; [[fallthrough]];
                    case 2: k1 ^= static_cast<uint32_t>(str[i + 1]) << 8;  [[fallthrough]];
                    case 1: k1 ^= static_cast<uint32_t>(str[i]);
                            k1 *= c1;
                            k1 = (k1 << 15) | (k1 >> 17);
                            k1 *= c2;
                            hash ^= k1;
                }
                
                hash ^= len;
                hash ^= hash >> 16;
                hash *= 0x85ebca6b;
                hash ^= hash >> 13;
                hash *= 0xc2b2ae35;
                hash ^= hash >> 16;
                
                return hash;
            }
        };

        // =============================================================================
        // 哈希选择器（根据用途选择最佳算法）
        // =============================================================================

        template<typename Tag = void>
        struct HashPolicy {
            using hasher = Fnv1aHasher; // 默认使用FNV-1a
        };

        // 为不同类型定义不同的哈希策略
        template<>
        struct HashPolicy<struct TypeNameTag> {
            using hasher = Fnv1aHasher; // 类型名称使用FNV-1a
        };

        template<>
        struct HashPolicy<struct FieldNameTag> {
            using hasher = Djb2Hasher; // 字段名称使用DJB2
        };

        template<>
        struct HashPolicy<struct StringContentTag> {
            using hasher = SimpleMurmurHasher; // 字符串内容使用MurmurHash
        };

        // =============================================================================
        // 统一哈希接口
        // =============================================================================

        template<typename PolicyTag = void>
        class UnifiedHasher {
        private:
            using Hasher = typename HashPolicy<PolicyTag>::hasher;

        public:
            template<size_t N>
            consteval static HashValue Hash(const char(&str)[N]) {
                return Hasher::Hash(str);
            }

            consteval static HashValue Hash(std::string_view str) {
                return Hasher::Hash(str);
            }

            // 类型哈希
            template<typename T>
            consteval static HashValue HashType() {
                constexpr std::string_view name = GetTypeName<T>();
                return Hasher::Hash(name);
            }

            // 组合哈希
            template<typename... Args>
            consteval static HashValue Combine(Args... hashes) {
                HashValue result = 2166136261u; // FNV offset basis
                ((result ^= static_cast<HashValue>(hashes), 
                  result *= 16777619u), ...);
                return result;
            }
        };

        // =============================================================================
        // 编译期哈希表
        // =============================================================================

        template<size_t TableSize, typename Key = HashValue, typename Value = size_t>
        class CompileTimeHashTable {
        private:
            struct Entry {
                Key key{};
                Value value{};
                bool occupied = false;
            };

            std::array<Entry, TableSize> table_{};

        public:
            constexpr CompileTimeHashTable() = default;

            // 插入键值对
            consteval bool Insert(const Key& key, const Value& value) {
                size_t index = HashKey(key) % TableSize;
                size_t original_index = index;
                
                // 线性探测解决冲突
                while (table_[index].occupied) {
                    if (table_[index].key == key) {
                        // 更新现有值
                        table_[index].value = value;
                        return false;
                    }
                    index = (index + 1) % TableSize;
                    if (index == original_index) {
                        return false; // 表满
                    }
                }
                
                // 插入新值
                table_[index].key = key;
                table_[index].value = value;
                table_[index].occupied = true;
                return true;
            }

            // 查找值
            consteval const Value* Find(const Key& key) const {
                size_t index = HashKey(key) % TableSize;
                size_t original_index = index;
                
                while (table_[index].occupied) {
                    if (table_[index].key == key) {
                        return &table_[index].value;
                    }
                    index = (index + 1) % TableSize;
                    if (index == original_index) {
                        break; // 回到起点，未找到
                    }
                }
                
                return nullptr;
            }

            // 检查是否存在键
            consteval bool Contains(const Key& key) const {
                return Find(key) != nullptr;
            }

            // 获取负载因子
            consteval float LoadFactor() const {
                size_t occupied = 0;
                for (const auto& entry : table_) {
                    if (entry.occupied) {
                        ++occupied;
                    }
                }
                return static_cast<float>(occupied) / TableSize;
            }

            consteval size_t Size() const {
                size_t count = 0;
                for (const auto& entry : table_) {
                    if (entry.occupied) {
                        ++count;
                    }
                }
                return count;
            }

        private:
            consteval static size_t HashKey(const Key& key) {
                // 简单的哈希函数，实际项目中可能需要更好的分布
                if constexpr (std::is_same_v<Key, HashValue>) {
                    return key;
                } else {
                    // 对于其他类型，使用统一哈希器
                    return UnifiedHasher<>::Hash(std::string_view(
                        reinterpret_cast<const char*>(&key), sizeof(Key)));
                }
            }
        };

        // =============================================================================
        // 完美哈希生成器
        // =============================================================================

        template<size_t MaxKeys>
        class PerfectHashGenerator {
        public:
            struct HashFunction {
                uint32_t multiplier;
                uint32_t offset;
                
                constexpr uint32_t operator()(uint32_t key) const {
                    return (key * multiplier + offset) % MaxKeys;
                }
            };

            template<size_t N>
            consteval static HashFunction Generate(const std::array<uint32_t, N>& keys) {
                // 简化的完美哈希生成算法
                uint32_t best_multiplier = 2654435761U; // 黄金比例
                uint32_t best_offset = 0;
                
                // 简单的冲突检测
                std::array<bool, MaxKeys> used{};
                bool hasConflict = false;
                
                do {
                    hasConflict = false;
                    used.fill(false);
                    
                    for (uint32_t key : keys) {
                        uint32_t index = (key * best_multiplier + best_offset) % MaxKeys;
                        if (used[index]) {
                            hasConflict = true;
                            best_multiplier = (best_multiplier * 16807) % 2147483647; // 线性同余生成器
                            break;
                        }
                        used[index] = true;
                    }
                } while (hasConflict && best_multiplier != 2654435761U); // 避免无限循环
                
                return HashFunction{best_multiplier, best_offset};
            }
        };

        // =============================================================================
        // 预计算的常用哈希值
        // =============================================================================

        namespace precomputed {
            
            // 常用类型名称的哈希值
            template<typename T>
            consteval HashValue TypeHash() {
                return UnifiedHasher<TypeNameTag>::HashType<T>();
            }

            // 常用字符串的哈希值
            template<size_t N>
            consteval HashValue StringHash(const char(&str)[N]) {
                return UnifiedHasher<StringContentTag>::Hash(str);
            }

            // 字段名称哈希
            template<size_t N>
            consteval HashValue FieldHash(const char(&fieldName)[N]) {
                return UnifiedHasher<FieldNameTag>::Hash(fieldName);
            }

        } // namespace precomputed

    } // namespace hash

    // 便利的命名空间别名
    namespace h = hash;

} // namespace shine::reflection