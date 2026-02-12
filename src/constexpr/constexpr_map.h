#pragma once

#include <array>
#include <utility>
#include <type_traits>
#include <optional>
#include <iterator>
#include <stdexcept>

namespace shine {

template <typename T>
inline constexpr auto ct_capacity_v = 0uz;  // 主模板声明

namespace constexpr_ {

template <typename Key, typename Value>
struct constexpr_map_value {
    using key_type   = Key;
    using mapped_type = Value;
    Key   key{};
    Value value{};

    constexpr constexpr_map_value() = default;
    constexpr constexpr_map_value(Key const &k, Value const &v) : key(k), value(v) {}
    constexpr constexpr_map_value(Key const &k, Value &&v) : key(k), value(std::move(v)) {}
    constexpr constexpr_map_value(Key &&k, Value const &v) : key(std::move(k)), value(v) {}
    constexpr constexpr_map_value(Key &&k, Value &&v) : key(std::move(k)), value(std::move(v)) {}
};

template <typename Key, typename Value, size_t N>
class constexpr_map {
public:
    using value_type      = constexpr_map_value<Key, Value>;
    using key_type        = Key;
    using mapped_type     = Value;
    using size_type       = size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = value_type const&;
    using pointer         = value_type*;
    using const_pointer   = value_type const*;
    using iterator        = pointer;
    using const_iterator  = const_pointer;

    constexpr constexpr_map() = default;

    template <std::convertible_to<value_type>... Ts>
    requires(sizeof...(Ts) <= N)
    constexpr explicit constexpr_map(Ts&&... vals)
        : storage{std::forward<Ts>(vals)...}, current_size{sizeof...(Ts)} {}

    // 迭代器
    constexpr auto begin() noexcept -> iterator { return storage.data(); }
    constexpr auto begin() const noexcept -> const_iterator { return storage.data(); }
    constexpr auto end() noexcept -> iterator { return storage.data() + current_size; }
    constexpr auto end() const noexcept -> const_iterator { return storage.data() + current_size; }

    constexpr auto cbegin() const noexcept -> const_iterator { return storage.data(); }
    constexpr auto cend() const noexcept -> const_iterator { return storage.data() + current_size; }

    constexpr auto rbegin() noexcept { return std::reverse_iterator<iterator>(end()); }
    constexpr auto rbegin() const noexcept { return std::reverse_iterator<const_iterator>(end()); }
    constexpr auto rend() noexcept { return std::reverse_iterator<iterator>(begin()); }
    constexpr auto rend() const noexcept { return std::reverse_iterator<const_iterator>(begin()); }

    constexpr auto crbegin() const noexcept { return std::reverse_iterator<const_iterator>(cend()); }
    constexpr auto crend() const noexcept { return std::reverse_iterator<const_iterator>(cbegin()); }

    // 容量
    constexpr size_type size() const noexcept { return current_size; }
    constexpr static std::integral_constant<size_type, N> capacity{};
    constexpr bool full() const noexcept { return current_size >= N; }
    constexpr bool empty() const noexcept { return current_size == 0; }

    // 清空
    constexpr void clear() noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            storage[i].~value_type();
            new (&storage[i]) value_type();
        }
        current_size = 0;
    }

    // 查找
    constexpr iterator find(key_type const& key) noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == key) {
                return &storage[i];
            }
        }
        return end();
    }

    constexpr const_iterator find(key_type const& key) const noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == key) {
                return &storage[i];
            }
        }
        return end();
    }

    constexpr bool contains(key_type const& key) const noexcept {
        return find(key) != end();
    }

    // 访问 - 带 bounds checking
    constexpr mapped_type& at(key_type const& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("constexpr_map::at: key not found");
        }
        return it->value;
    }

    constexpr mapped_type const& at(key_type const& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("constexpr_map::at: key not found");
        }
        return it->value;
    }

    // 安全访问 - 返回 optional
    constexpr std::optional<mapped_type> try_get(key_type const& key) const noexcept {
        auto it = find(key);
        if (it != end()) {
            return it->value;
        }
        return std::nullopt;
    }

    // 直接访问（不安全，但兼容旧代码）- 找不到时行为未定义
    constexpr mapped_type& get(key_type const& key) noexcept {
        auto it = find(key);
        // 注意：如果找不到，行为未定义（但不会触发编译器假设优化）
        return it->value;
    }

    constexpr mapped_type const& get(key_type const& key) const noexcept {
        auto it = find(key);
        return it->value;
    }

    // operator[] - 找不到时插入默认值
    constexpr mapped_type& operator[](key_type const& key) {
        auto it = find(key);
        if (it != end()) {
            return it->value;
        }
        // 插入新元素
        if (current_size < N) {
            storage[current_size].key = key;
            return storage[current_size++].value;
        }
        throw std::out_of_range("constexpr_map::operator[]: map is full");
    }

    // 插入
    constexpr bool insert(key_type const& key, mapped_type const& value) {
        if (contains(key)) {
            return false;  // 已存在，插入失败
        }
        if (current_size >= N) {
            return false;  // 容量满
        }
        storage[current_size++] = value_type(key, value);
        return true;
    }

    constexpr bool insert(key_type&& key, mapped_type&& value) {
        if (contains(key)) {
            return false;
        }
        if (current_size >= N) {
            return false;
        }
        storage[current_size++] = value_type(std::move(key), std::move(value));
        return true;
    }

    // 插入或更新
    template <typename K, typename V>
    constexpr bool put(K&& key, V&& value) {
        // 查找是否已存在
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == key) {
                storage[i].value = std::forward<V>(value);
                return false;  // 更新现有项
            }
        }
        // 添加新项
        if (current_size < N) {
            storage[current_size++] = value_type(std::forward<K>(key), std::forward<V>(value));
            return true;  // 新增项
        }
        return false;  // 容量满
    }

    // 就地构造
    template <typename K, typename... Args>
    constexpr bool emplace(K&& key, Args&&... args) {
        if (contains(key)) {
            return false;
        }
        if (current_size >= N) {
            return false;
        }
        storage[current_size].key = std::forward<K>(key);
        storage[current_size].value = mapped_type(std::forward<Args>(args)...);
        ++current_size;
        return true;
    }

    // 删除
    constexpr size_type erase(key_type const& key) noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == key) {
                // 将最后一个元素移到被删除位置
                if (i != current_size - 1) {
                    storage[i] = std::move(storage[current_size - 1]);
                }
                --current_size;
                return 1u;
            }
        }
        return 0u;
    }

    // 弹出最后一个元素
    constexpr std::optional<value_type> pop_back() noexcept {
        if (current_size == 0) {
            return std::nullopt;
        }
        return storage[--current_size];
    }

    // 编译期查找（当 Key 可作为模板参数时）
    template <key_type Key>
    constexpr bool contains_ct() const noexcept {
        return contains(Key);
    }

    template <key_type Key>
    constexpr mapped_type& get_ct() noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == Key) {
                return storage[i].value;
            }
        }
        // 未找到时的未定义行为（保持兼容性）
        return storage[0].value;
    }

    template <key_type Key>
    constexpr mapped_type const& get_ct() const noexcept {
        for (size_t i = 0; i < current_size; ++i) {
            if (storage[i].key == Key) {
                return storage[i].value;
            }
        }
        return storage[0].value;
    }

    // 交换
    constexpr void swap(constexpr_map& other) noexcept {
        using std::swap;
        swap(storage, other.storage);
        swap(current_size, other.current_size);
    }

private:
    std::array<value_type, N> storage{};
    size_t current_size{};
};

// 非成员函数
template <typename K, typename V, size_t N>
constexpr void swap(constexpr_map<K, V, N>& lhs, constexpr_map<K, V, N>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace constexpr_

// ct_capacity_v 特化
template <typename K, typename V, std::size_t N>
inline constexpr auto ct_capacity_v<constexpr_::constexpr_map<K, V, N>> = N;

} // namespace shine
