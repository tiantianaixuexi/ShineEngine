#pragma once
#include "../Core/ReflectionModernTypes.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../../../constexpr/constexpr_map.h"
#include "../constexpr/simple_constexpr_containers.h"

namespace shine::reflection::container {

// 容器访问接口
template<typename Container>
struct ContainerInterface {
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    
    static constexpr size_type size(const Container& c) {
        return c.size();
    }
    
    static constexpr auto begin(Container& c) {
        return c.begin();
    }
    
    static constexpr auto end(Container& c) {
        return c.end();
    }
};

// Vector容器特质 - 利用constexpr_vector
template<VectorLike Container>
struct VectorTraits : ContainerInterface<Container> {
    static constexpr void push_back(Container& c, const typename Container::value_type& value) {
        c.push_back(value);
    }
    
    static constexpr auto& at(Container& c, size_t index) {
        return c.at(index);
    }
    
    static constexpr void clear(Container& c) {
        c.clear();
    }
    
    static constexpr bool empty(const Container& c) {
        return c.empty();
    }
    
    // 提供编译期容量信息
    static constexpr size_t max_compile_time_size = 128; // 可配置
};

// Map容器特质 - 利用constexpr_map
template<MapLike Container>
struct MapTraits : ContainerInterface<Container> {
    using key_type = typename Container::key_type;
    using mapped_type = typename Container::mapped_type;
    
    static constexpr auto insert(Container& c, const key_type& key, const mapped_type& value) {
        return c.insert({key, value});
    }
    
    static constexpr auto find(Container& c, const key_type& key) {
        return c.find(key);
    }
    
    static constexpr bool contains(const Container& c, const key_type& key) {
        return c.find(key) != c.end();
    }
    
    static constexpr void clear(Container& c) {
        c.clear();
    }
    
    static constexpr bool empty(const Container& c) {
        return c.empty();
    }
    
    // 编译期键值对数量限制
    static constexpr size_t max_compile_time_pairs = 64;
};

// Pair特质
template<PairLike Pair>
struct PairTraits {
    using first_type = typename Pair::first_type;
    using second_type = typename Pair::second_type;
    
    static constexpr const first_type& first(const Pair& p) {
        return p.first;
    }
    
    static constexpr const second_type& second(const Pair& p) {
        return p.second;
    }
    
    static constexpr void set_first(Pair& p, const first_type& value) {
        p.first = value;
    }
    
    static constexpr void set_second(Pair& p, const second_type& value) {
        p.second = value;
    }
};

// 编译期容器包装器
template<typename T, size_t Capacity>
using CompileTimeVector = shine::constexpr_vector<T, Capacity>;

template<typename Key, typename Value, size_t Capacity>
using CompileTimeMap = shine::constexpr_map<Key, Value, Capacity>;

// 容器类型判断
template<typename T>
struct IsCompileTimeContainer : std::false_type {};

template<typename T, size_t Capacity>
struct IsCompileTimeContainer<shine::constexpr_vector<T, Capacity>> : std::true_type {};

template<typename Key, typename Value, size_t Capacity>
struct IsCompileTimeContainer<shine::constexpr_map<Key, Value, Capacity>> : std::true_type {};

template<typename T>
constexpr bool is_compile_time_container_v = IsCompileTimeContainer<T>::value;

// 容器元素类型提取
template<typename Container>
struct ContainerElementType {
    using type = typename Container::value_type;
};

template<typename Key, typename Value, size_t Capacity>
struct ContainerElementType<shine::constexpr_map<Key, Value, Capacity>> {
    using type = std::pair<Key, Value>;
};

template<typename Container>
using container_element_type_t = typename ContainerElementType<Container>::type;

} // namespace shine::reflection::container