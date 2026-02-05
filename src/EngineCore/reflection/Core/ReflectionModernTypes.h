#pragma once
#include <concepts>
#include <type_traits>
#include <string_view>
#include "../../../constexpr/constexpr_map.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../constexpr/simple_constexpr_containers.h"

namespace shine::reflection {

// 现代化的类型标识系统
template<typename T>
struct TypeIdentity {
    static constexpr std::string_view name = __FUNCSIG__;  // MSVC特有
    static constexpr uint32_t id = GenerateTypeId<T>();
    
    // 编译期计算的类型特征
    static constexpr bool is_pod = std::is_trivially_copyable_v<T>;
    static constexpr bool is_empty = std::is_empty_v<T>;
    static constexpr size_t size = sizeof(T);
    static constexpr size_t alignment = alignof(T);
};

// 容器特征检测
template<typename T>
concept VectorLike = requires(T t) {
    typename T::value_type;
    t.size();
    t.begin();
    t.end();
    t.push_back(std::declval<typename T::value_type>());
};

template<typename T>
concept MapLike = requires(T t, const typename T::key_type& k) {
    typename T::key_type;
    typename T::mapped_type;
    t.find(k);
    t.end();
    t.insert({k, std::declval<typename T::mapped_type>()});
};

template<typename T>
concept PairLike = requires(T t) {
    typename T::first_type;
    typename T::second_type;
    t.first;
    t.second;
};

// 编译期类型注册表
using TypeRegistryMap = shine::constexpr_map<uint32_t, std::string_view, 1024>;

// 类型ID生成器
template<typename T>
consteval uint32_t GenerateTypeId() {
    // 简单的哈希实现，实际项目中可以使用更复杂的算法
    uint32_t hash = 2166136261u;
    for (char c : __FUNCSIG__) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash ? hash : 1; // 避免返回0
}

} // namespace shine::reflection