#pragma once

#include <array>
#include <string_view>
#include <type_traits>
#include <concepts>

#include "../Core/ReflectionCore.h"
#include "../../constexpr/constexpr_vector.h"
#include "../../constexpr/constexpr_map.h"

namespace shine::reflection {

    // =============================================================================
    // 编译期计算核心系统
    // =============================================================================

    namespace compile_time {

        // =============================================================================
        // 编译期哈希系统
        // =============================================================================

        class HashSystem {
        public:
            // 基础哈希函数
            template<size_t N>
            consteval static HashValue HashString(const char(&str)[N]) {
                HashValue hash = REFLECTION_HASH_SEED;
                for (size_t i = 0; i < N - 1; ++i) {
                    hash ^= static_cast<uint8_t>(str[i]);
                    hash *= REFLECTION_HASH_MULTIPLIER;
                }
                return hash;
            }

            consteval static HashValue HashString(std::string_view str) {
                HashValue hash = REFLECTION_HASH_SEED;
                for (char c : str) {
                    hash ^= static_cast<uint8_t>(c);
                    hash *= REFLECTION_HASH_MULTIPLIER;
                }
                return hash;
            }

            // 类型名称哈希
            template<typename T>
            consteval static HashValue HashTypeName() {
                constexpr std::string_view name = GetTypeName<T>();
                return HashString(name);
            }

            // 组合哈希
            template<typename... Args>
            consteval static HashValue CombineHashes(Args... hashes) {
                HashValue result = REFLECTION_HASH_SEED;
                ((result ^= static_cast<HashValue>(hashes), 
                  result *= REFLECTION_HASH_MULTIPLIER), ...);
                return result;
            }

            // 哈希分布质量检查
            template<size_t N>
            consteval static bool ValidateHashDistribution(const std::array<std::string_view, N>& strings) {
                std::array<HashValue, N> hashes{};
                for (size_t i = 0; i < N; ++i) {
                    hashes[i] = HashString(strings[i]);
                }
                
                // 检查冲突
                for (size_t i = 0; i < N; ++i) {
                    for (size_t j = i + 1; j < N; ++j) {
                        if (hashes[i] == hashes[j]) {
                            return false; // 发现冲突
                        }
                    }
                }
                return true;
            }
        };

        // =============================================================================
        // 编译期类型信息系统
        // =============================================================================

        template<typename T>
        struct TypeInfoComputer {
            // 基本类型信息
            static constexpr std::string_view name = GetTypeName<T>();
            static constexpr TypeId id = HashSystem::HashTypeName<T>();
            static constexpr size_t size = sizeof(T);
            static constexpr size_t alignment = alignof(T);
            
            // 类型特征
            static constexpr bool is_pod = std::is_trivially_copyable_v<T>;
            static constexpr bool is_empty = std::is_empty_v<T>;
            static constexpr bool is_abstract = std::is_abstract_v<T>;
            static constexpr bool is_final = std::is_final_v<T>;
            static constexpr bool is_polymorphic = std::is_polymorphic_v<T>;
            
            // 容器类型检测
            static constexpr ContainerType container_type = []() consteval {
                if constexpr (requires { typename T::value_type; }) {
                    if constexpr (requires(T t) { t.size(); t.begin(); t.end(); }) {
                        return ContainerType::Sequence;
                    } else if constexpr (requires(T t, typename T::key_type key) { 
                        t.find(key); t.end(); 
                    }) {
                        return ContainerType::Associative;
                    }
                } else if constexpr (std::is_array_v<T>) {
                    return ContainerType::Array;
                }
                return ContainerType::None;
            }();

            // 获取基类信息（如果有）
            template<typename Base>
            static constexpr bool is_derived_from = std::is_base_of_v<Base, T>;
            
            // 成员数量计算（对于聚合类型）
            static constexpr size_t member_count = []() consteval {
                if constexpr (std::is_aggregate_v<T> && !std::is_empty_v<T>) {
                    // 这是一个简化的实现，实际项目中可能需要更复杂的逻辑
                    return sizeof(T) / sizeof(void*); // 估算
                }
                return 0;
            }();
        };

        // =============================================================================
        // 编译期字符串处理系统
        // =============================================================================

        class StringProcessor {
        public:
            // 编译期字符串长度
            template<size_t N>
            consteval static size_t Length(const char(&str)[N]) {
                size_t len = 0;
                while (len < N - 1 && str[len] != '\0') {
                    ++len;
                }
                return len;
            }

            // 编译期字符串比较
            template<size_t N, size_t M>
            consteval static bool Equal(const char(&str1)[N], const char(&str2)[M]) {
                if (N != M) return false;
                for (size_t i = 0; i < N - 1; ++i) {
                    if (str1[i] != str2[i]) return false;
                    if (str1[i] == '\0') break;
                }
                return true;
            }

            // 编译期字符串查找
            template<size_t N, size_t M>
            consteval static size_t Find(const char(&haystack)[N], const char(&needle)[M]) {
                constexpr size_t haystack_len = N - 1;
                constexpr size_t needle_len = M - 1;
                
                if (needle_len == 0 || needle_len > haystack_len) return N; // 未找到
                
                for (size_t i = 0; i <= haystack_len - needle_len; ++i) {
                    bool found = true;
                    for (size_t j = 0; j < needle_len; ++j) {
                        if (haystack[i + j] != needle[j]) {
                            found = false;
                            break;
                        }
                    }
                    if (found) return i;
                }
                return N; // 未找到
            }

            // 编译期字符串替换
            template<size_t N, size_t M, size_t R>
            consteval static auto Replace(const char(&str)[N], 
                                        const char(&from)[M], 
                                        const char(&to)[R]) {
                constexpr size_t str_len = N - 1;
                constexpr size_t from_len = M - 1;
                constexpr size_t to_len = R - 1;
                
                if (from_len == 0) {
                    std::array<char, N> result{};
                    for (size_t i = 0; i < str_len; ++i) {
                        result[i] = str[i];
                    }
                    result[str_len] = '\0';
                    return result;
                }
                
                // 计算结果长度
                size_t result_len = str_len;
                size_t pos = 0;
                while ((pos = Find(str + pos, from)) < N) {
                    result_len = result_len - from_len + to_len;
                    pos += from_len;
                }
                
                std::array<char, result_len + 1> result{};
                size_t result_pos = 0;
                size_t str_pos = 0;
                
                while (str_pos < str_len) {
                    size_t found_pos = Find(str + str_pos, from);
                    if (found_pos >= N) {
                        // 复制剩余字符
                        for (size_t i = str_pos; i < str_len; ++i) {
                            result[result_pos++] = str[i];
                        }
                        break;
                    }
                    
                    // 复制匹配位置之前的字符
                    for (size_t i = str_pos; i < str_pos + found_pos; ++i) {
                        result[result_pos++] = str[i];
                    }
                    
                    // 插入替换字符串
                    for (size_t i = 0; i < to_len; ++i) {
                        result[result_pos++] = to[i];
                    }
                    
                    str_pos += found_pos + from_len;
                }
                
                result[result_pos] = '\0';
                return result;
            }
        };

        // =============================================================================
        // 编译期数学计算系统
        // =============================================================================

        class MathProcessor {
        public:
            // 编译期幂运算
            template<typename T>
            consteval static T Power(T base, unsigned exp) {
                T result = T{1};
                while (exp > 0) {
                    if (exp & 1) result *= base;
                    base *= base;
                    exp >>= 1;
                }
                return result;
            }

            // 编译期对数运算（以2为底）
            consteval static unsigned Log2(unsigned value) {
                unsigned result = 0;
                while (value > 1) {
                    value >>= 1;
                    ++result;
                }
                return result;
            }

            // 编译期平方根（牛顿法）
            template<typename T>
            consteval static T Sqrt(T value) {
                if (value <= T{0}) return T{0};
                if (value <= T{1}) return value;
                
                T x = value;
                T y = (x + T{1}) / T{2};
                
                while (y < x) {
                    x = y;
                    y = (x + value / x) / T{2};
                }
                
                return x;
            }

            // 编译期最大公约数
            template<typename T>
            consteval static T Gcd(T a, T b) {
                while (b != T{0}) {
                    T temp = b;
                    b = a % b;
                    a = temp;
                }
                return a;
            }

            // 编译期最小公倍数
            template<typename T>
            consteval static T Lcm(T a, T b) {
                return (a * b) / Gcd(a, b);
            }
        };

        // =============================================================================
        // 编译期缓存系统
        // =============================================================================

        template<typename Key, typename Value, size_t Capacity>
        class CompileTimeCache {
        private:
            constexpr_::constexpr_map<Key, Value, Capacity> storage_;

        public:
            constexpr CompileTimeCache() = default;

            // 获取或计算值
            template<typename ComputeFunc>
            consteval const Value& GetOrCompute(const Key& key, ComputeFunc compute_func) {
                if (storage_.contains(key)) {
                    return storage_.get(key);
                }
                
                if (!storage_.full()) {
                    Value value = compute_func(key);
                    storage_.put(key, value);
                    return storage_.get(key);
                }
                
                // 缓存满时的行为
                static Value default_value{};
                return default_value;
            }

            // 预填充缓存
            template<size_t N>
            consteval void Prepopulate(const std::array<std::pair<Key, Value>, N>& data) {
                static_assert(N <= Capacity, "Prepopulation data exceeds cache capacity");
                for (const auto& [key, value] : data) {
                    if (!storage_.full()) {
                        storage_.put(key, value);
                    }
                }
            }

            consteval size_t Size() const { return storage_.size(); }
            consteval bool Contains(const Key& key) const { return storage_.contains(key); }
        };

        // =============================================================================
        // 编译期元编程工具
        // =============================================================================

        // 类型列表
        template<typename... Types>
        struct TypeList {};

        // 类型列表操作
        template<typename List>
        struct ListSize;

        template<typename... Types>
        struct ListSize<TypeList<Types...>> {
            static constexpr size_t value = sizeof...(Types);
        };

        // 索引序列生成
        template<size_t... Indices>
        struct IndexSequence {};

        template<size_t N, size_t... Indices>
        struct MakeIndexSequence : MakeIndexSequence<N-1, N-1, Indices...> {};

        template<size_t... Indices>
        struct MakeIndexSequence<0, Indices...> {
            using type = IndexSequence<Indices...>;
        };

        template<size_t N>
        using MakeIndexSequenceT = typename MakeIndexSequence<N>::type;

        // 条件类型选择
        template<bool Condition, typename TrueType, typename FalseType>
        struct Conditional {
            using type = TrueType;
        };

        template<typename TrueType, typename FalseType>
        struct Conditional<false, TrueType, FalseType> {
            using type = FalseType;
        };

        template<bool Condition, typename TrueType, typename FalseType>
        using ConditionalT = typename Conditional<Condition, TrueType, FalseType>::type;

    } // namespace compile_time

    // 便利的命名空间别名
    namespace ct = compile_time;

} // namespace shine::reflection