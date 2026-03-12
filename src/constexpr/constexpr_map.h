#pragma once

#include <array>
#include <utility>
#include <type_traits>
#include <optional>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <concepts>
#include <functional>

#include "iterator.h"
#include "compiler_hints.h"

// Forward declare constexpr_vector for keys()/values() methods
namespace shine {
namespace constexpr_ {
template <typename T, std::size_t N>
class constexpr_vector;
}
}

namespace shine {


namespace constexpr_ {

// ============================================================
// constexpr_map_value - 键值对
// ============================================================
template <typename Key, typename Value>
struct constexpr_map_value {
    using key_type = Key;
    using mapped_type = Value;
    Key key{};
    Value value{};

    constexpr constexpr_map_value() = default;
    constexpr constexpr_map_value(Key const& k, Value const& v) : key(k), value(v) {}
    constexpr constexpr_map_value(Key const& k, Value&& v) : key(k), value(std::move(v)) {}
    constexpr constexpr_map_value(Key&& k, Value const& v) : key(std::move(k)), value(v) {}
    constexpr constexpr_map_value(Key&& k, Value&& v) : key(std::move(k)), value(std::move(v)) {}

    // 比较运算符
    constexpr bool operator==(constexpr_map_value const& other) const {
        return key == other.key && value == other.value;
    }

    constexpr auto operator<=>(constexpr_map_value const& other) const
        requires std::three_way_comparable<Key> {
        return key <=> other.key;
    }
};

// ============================================================
// constexpr_map - 有序映射（保持插入顺序）
// ============================================================
template <typename Key, typename Value, std::size_t N>
class constexpr_map {
public:
    using value_type = constexpr_map_value<Key, Value>;
    using key_type = Key;
    using mapped_type = Value;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using iterator = pointer;
    using const_iterator = const_pointer;

private:
    std::array<value_type, N> storage_{};
    size_type current_size_{0};
    bool sorted_{false}; // 是否已按键排序

public:
    // ==================== 构造函数 ====================

    constexpr constexpr_map() = default;

    template <std::convertible_to<value_type>... Ts>
        requires(sizeof...(Ts) <= N)
    constexpr explicit constexpr_map(Ts&&... vals)
        : storage_{std::forward<Ts>(vals)...}, current_size_{sizeof...(Ts)} {}

    // 从初始化列表构造
    constexpr constexpr_map(std::initializer_list<value_type> init) {
        for (auto it = init.begin(); it != init.end() && current_size_ < N; ++it) {
            storage_[current_size_++] = *it;
        }
    }

    // ==================== 迭代器 ====================

    constexpr iterator begin() noexcept { return storage_.data(); }
    constexpr iterator end() noexcept { return storage_.data() + current_size_; }
    constexpr const_iterator begin() const noexcept { return storage_.data(); }
    constexpr const_iterator end() const noexcept { return storage_.data() + current_size_; }
    constexpr const_iterator cbegin() const noexcept { return storage_.data(); }
    constexpr const_iterator cend() const noexcept { return storage_.data() + current_size_; }

    constexpr auto rbegin() noexcept { return std::reverse_iterator<iterator>(end()); }
    constexpr auto rend() noexcept { return std::reverse_iterator<iterator>(begin()); }
    constexpr auto rbegin() const noexcept { return std::reverse_iterator<const_iterator>(end()); }
    constexpr auto rend() const noexcept { return std::reverse_iterator<const_iterator>(begin()); }
    constexpr auto crbegin() const noexcept { return std::reverse_iterator<const_iterator>(cend()); }
    constexpr auto crend() const noexcept { return std::reverse_iterator<const_iterator>(cbegin()); }

    // ==================== 容量 ====================

    constexpr size_type size() const noexcept { return current_size_; }
    constexpr static std::integral_constant<size_type, N> capacity{};
    constexpr bool full() const noexcept { return current_size_ >= N; }
    constexpr bool empty() const noexcept { return current_size_ == 0; }
    constexpr size_type available() const noexcept { return N - current_size_; }

    // ==================== 排序状态 ====================

    constexpr bool is_sorted() const noexcept { return sorted_; }

    // 按键排序
    constexpr void sort()
        requires std::totally_ordered<Key> {
        if (!sorted_) {
            std::sort(begin(), end(), [](value_type const& a, value_type const& b) {
                return a.key < b.key;
            });
            sorted_ = true;
        }
    }

    // 编译期排序
    consteval auto sorted() const
        requires std::totally_ordered<Key> {
        constexpr_map result = *this;
        result.sort();
        return result;
    }

    // ==================== 查找 ====================

    // 线性查找（通用）
    constexpr iterator find(key_type const& key) noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) {
                return &storage_[i];
            }
        }
        return end();
    }

    constexpr const_iterator find(key_type const& key) const noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) {
                return &storage_[i];
            }
        }
        return end();
    }

    // 二分查找（仅适用于已排序映射）
    constexpr iterator binary_find(key_type const& key)
        requires std::totally_ordered<Key> {
        if (!sorted_) {
            // 如果未排序，先排序
            sort();
        }
        auto it = std::lower_bound(begin(), end(), key,
            [](value_type const& elem, key_type const& k) {
                return elem.key < k;
            });
        if (it != end() && it->key == key) {
            return it;
        }
        return end();
    }

    constexpr const_iterator binary_find(key_type const& key) const
        requires std::totally_ordered<Key> {
        if (!sorted_) {
            return find(key); // const 版本不能排序
        }
        auto it = std::lower_bound(begin(), end(), key,
            [](value_type const& elem, key_type const& k) {
                return elem.key < k;
            });
        if (it != end() && it->key == key) {
            return it;
        }
        return end();
    }

    constexpr bool contains(key_type const& key) const noexcept {
        return find(key) != end();
    }

    // 统计键出现次数
    constexpr size_type count(key_type const& key) const noexcept {
        size_type cnt = 0;
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) ++cnt;
        }
        return cnt;
    }

    // ==================== 访问 ====================

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

    // 直接访问（不安全，用于性能关键路径）
    constexpr mapped_type& get(key_type const& key) noexcept {
        auto it = find(key);
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
        if (current_size_ < N) {
            storage_[current_size_].key = key;
            sorted_ = false; // 插入可能破坏排序
            return storage_[current_size_++].value;
        }
        throw std::out_of_range("constexpr_map::operator[]: map is full");
    }

    // 带默认值的访问
    constexpr mapped_type const& get_or(key_type const& key, mapped_type const& default_value) const noexcept {
        auto it = find(key);
        return it != end() ? it->value : default_value;
    }

    // ==================== 插入 ====================

    constexpr bool insert(key_type const& key, mapped_type const& value) {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = value_type(key, value);
        sorted_ = false;
        return true;
    }

    constexpr bool insert(key_type&& key, mapped_type&& value) {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = value_type(std::move(key), std::move(value));
        sorted_ = false;
        return true;
    }

    // 插入或更新
    template <typename K, typename V>
    constexpr bool put(K&& key, V&& value) {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) {
                storage_[i].value = std::forward<V>(value);
                return false; // 更新现有项
            }
        }
        if (current_size_ < N) {
            storage_[current_size_++] = value_type(std::forward<K>(key), std::forward<V>(value));
            sorted_ = false;
            return true; // 新增项
        }
        return false;
    }

    // 就地构造
    template <typename K, typename... Args>
    constexpr bool emplace(K&& key, Args&&... args) {
        if (current_size_ >= N) return false;
        storage_[current_size_].key = std::forward<K>(key);
        storage_[current_size_].value = mapped_type(std::forward<Args>(args)...);
        ++current_size_;
        sorted_ = false;
        return true;
    }

    // 尝试插入（键不存在时才插入）
    template <typename K, typename V>
    constexpr bool try_insert(K&& key, V&& value) {
        if (contains(key)) return false;
        return insert(std::forward<K>(key), std::forward<V>(value));
    }

    // ==================== 删除 ====================

    constexpr size_type erase(key_type const& key) noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) {
                // 将最后一个元素移到被删除位置（保持O(1)删除）
                if (i != current_size_ - 1) {
                    storage_[i] = std::move(storage_[current_size_ - 1]);
                    sorted_ = false; // 移动可能破坏排序
                }
                --current_size_;
                return 1u;
            }
        }
        return 0u;
    }

    // 删除所有匹配键的元素
    constexpr size_type erase_all(key_type const& key) noexcept {
        size_type removed = 0;
        size_type write_idx = 0;
        for (size_type read_idx = 0; read_idx < current_size_; ++read_idx) {
            if (storage_[read_idx].key != key) {
                if (write_idx != read_idx) {
                    storage_[write_idx] = std::move(storage_[read_idx]);
                }
                ++write_idx;
            } else {
                ++removed;
            }
        }
        current_size_ = write_idx;
        if (removed > 0) sorted_ = false;
        return removed;
    }

    // 按迭代器删除
    constexpr iterator erase(const_iterator pos) noexcept {
        size_type index = pos - cbegin();
        ASSUME(index < current_size_);
        if (index != current_size_ - 1) {
            storage_[index] = std::move(storage_[current_size_ - 1]);
            sorted_ = false;
        }
        --current_size_;
        return begin() + index;
    }

    // 弹出最后一个元素
    constexpr std::optional<value_type> pop_back() noexcept {
        if (current_size_ == 0) return std::nullopt;
        return storage_[--current_size_];
    }

    // 清空
    constexpr void clear() noexcept {
        current_size_ = 0;
        sorted_ = false;
    }

    // ==================== 编译期查找 ====================

    template <key_type Key>
    constexpr bool contains_ct() const noexcept {
        return contains(Key);
    }

    template <key_type Key>
    constexpr mapped_type& get_ct() noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == Key) {
                return storage_[i].value;
            }
        }
        return storage_[0].value;
    }

    template <key_type Key>
    constexpr mapped_type const& get_ct() const noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == Key) {
                return storage_[i].value;
            }
        }
        return storage_[0].value;
    }

    // ==================== 键/值集合 ====================

    // 获取所有键
    constexpr auto keys() const noexcept {
        constexpr_vector<key_type, N> result;
        for (size_type i = 0; i < current_size_; ++i) {
            result.push_back(storage_[i].key);
        }
        return result;
    }

    // 获取所有值
    constexpr auto values() const noexcept {
        constexpr_vector<mapped_type, N> result;
        for (size_type i = 0; i < current_size_; ++i) {
            result.push_back(storage_[i].value);
        }
        return result;
    }

    // ==================== 遍历 ====================

    // 遍历所有键值对
    template <typename Func>
    constexpr void for_each(Func&& func) const {
        for (size_type i = 0; i < current_size_; ++i) {
            std::forward<Func>(func)(storage_[i].key, storage_[i].value);
        }
    }

    // 过滤
    template <typename Predicate>
    constexpr size_type filter(Predicate pred) {
        size_type write_idx = 0;
        for (size_type read_idx = 0; read_idx < current_size_; ++read_idx) {
            if (pred(storage_[read_idx].key, storage_[read_idx].value)) {
                if (write_idx != read_idx) {
                    storage_[write_idx] = std::move(storage_[read_idx]);
                }
                ++write_idx;
            }
        }
        current_size_ = write_idx;
        return current_size_;
    }

    // ==================== 比较运算符 ====================

    constexpr bool operator==(constexpr_map const& other) const {
        if (current_size_ != other.current_size_) return false;
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i] != other.storage_[i]) return false;
        }
        return true;
    }

    // ==================== 交换 ====================

    constexpr void swap(constexpr_map& other) noexcept {
        using std::swap;
        swap(storage_, other.storage_);
        swap(current_size_, other.current_size_);
        swap(sorted_, other.sorted_);
    }

};

// ============================================================
// constexpr_sorted_map - 始终保持有序的映射
// ============================================================
template <typename Key, typename Value, std::size_t N>
class constexpr_sorted_map {
public:
    using value_type = constexpr_map_value<Key, Value>;
    using key_type = Key;
    using mapped_type = Value;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;
    using iterator = pointer;
    using const_iterator = const_pointer;

private:
    std::array<value_type, N> storage_{};
    size_type current_size_{0};

public:
    constexpr constexpr_sorted_map() = default;

    // 从已排序数据构造（编译期）
    template <std::convertible_to<value_type>... Ts>
        requires(sizeof...(Ts) <= N)
    consteval explicit constexpr_sorted_map(Ts&&... vals) {
        value_type arr[] = {static_cast<value_type>(std::forward<Ts>(vals))...};
        // 编译期排序
        constexpr size_t count = sizeof...(Ts);
        for (size_t i = 0; i < count; ++i) {
            for (size_t j = i + 1; j < count; ++j) {
                if (arr[j].key < arr[i].key) {
                    value_type tmp = std::move(arr[i]);
                    arr[i] = std::move(arr[j]);
                    arr[j] = std::move(tmp);
                }
            }
        }
        for (size_t i = 0; i < count; ++i) {
            storage_[i] = std::move(arr[i]);
        }
        current_size_ = count;
    }

    constexpr iterator begin() noexcept { return storage_.data(); }
    constexpr iterator end() noexcept { return storage_.data() + current_size_; }
    constexpr const_iterator begin() const noexcept { return storage_.data(); }
    constexpr const_iterator end() const noexcept { return storage_.data() + current_size_; }
    constexpr const_iterator cbegin() const noexcept { return storage_.data(); }
    constexpr const_iterator cend() const noexcept { return storage_.data() + current_size_; }

    constexpr size_type size() const noexcept { return current_size_; }
    constexpr bool empty() const noexcept { return current_size_ == 0; }
    constexpr bool full() const noexcept { return current_size_ >= N; }

    // 二分查找
    constexpr const_iterator find(key_type const& key) const noexcept {
        auto it = std::lower_bound(begin(), end(), key,
            [](value_type const& elem, key_type const& k) {
                return elem.key < k;
            });
        if (it != end() && it->key == key) {
            return it;
        }
        return end();
    }

    constexpr iterator find(key_type const& key) noexcept {
        auto it = std::lower_bound(begin(), end(), key,
            [](value_type const& elem, key_type const& k) {
                return elem.key < k;
            });
        if (it != end() && it->key == key) {
            return it;
        }
        return end();
    }

    constexpr bool contains(key_type const& key) const noexcept {
        return find(key) != end();
    }

    constexpr mapped_type const& at(key_type const& key) const {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("constexpr_sorted_map::at: key not found");
        }
        return it->value;
    }

    constexpr mapped_type& at(key_type const& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("constexpr_sorted_map::at: key not found");
        }
        return it->value;
    }

    constexpr mapped_type const& operator[](key_type const& key) const {
        return at(key);
    }

    // 插入（保持有序）- O(N)
    constexpr bool insert(key_type const& key, mapped_type const& value) {
        if (current_size_ >= N) return false;
        if (contains(key)) return false;

        // 找到插入位置
        auto pos = std::lower_bound(begin(), end(), key,
            [](value_type const& elem, key_type const& k) {
                return elem.key < k;
            });

        size_type index = pos - begin();

        // 移动元素
        for (size_type i = current_size_; i > index; --i) {
            storage_[i] = std::move(storage_[i - 1]);
        }

        storage_[index] = value_type(key, value);
        ++current_size_;
        return true;
    }

    // 编译期查找
    template <key_type Key>
    consteval bool contains_ct() const noexcept {
        return contains(Key);
    }

    template <key_type Key>
    consteval mapped_type const& get_ct() const noexcept {
        return at(Key);
    }
};

// ============================================================
// constexpr_multimap - 多重映射（允许重复键）
// ============================================================
template <typename Key, typename Value, std::size_t N>
class constexpr_multimap {
public:
    using value_type = constexpr_map_value<Key, Value>;
    using key_type = Key;
    using mapped_type = Value;
    using size_type = std::size_t;
    using iterator = value_type*;
    using const_iterator = value_type const*;

private:
    std::array<value_type, N> storage_{};
    size_type current_size_{0};

public:
    constexpr constexpr_multimap() = default;

    constexpr iterator begin() noexcept { return storage_.data(); }
    constexpr iterator end() noexcept { return storage_.data() + current_size_; }
    constexpr const_iterator begin() const noexcept { return storage_.data(); }
    constexpr const_iterator end() const noexcept { return storage_.data() + current_size_; }

    constexpr size_type size() const noexcept { return current_size_; }
    constexpr bool empty() const noexcept { return current_size_ == 0; }
    constexpr bool full() const noexcept { return current_size_ >= N; }

    // 查找所有匹配的键
    constexpr auto equal_range(key_type const& key) const noexcept {
        struct range {
            const_iterator first;
            const_iterator second;
        };
        const_iterator first = end();
        const_iterator last = end();

        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) {
                if (first == end()) {
                    first = &storage_[i];
                }
                last = &storage_[i] + 1;
            }
        }
        return range{first, last};
    }

    // 统计键出现次数
    constexpr size_type count(key_type const& key) const noexcept {
        size_type cnt = 0;
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i].key == key) ++cnt;
        }
        return cnt;
    }

    // 插入（允许重复键）
    constexpr bool insert(key_type const& key, mapped_type const& value) {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = value_type(key, value);
        return true;
    }

    constexpr bool insert(key_type&& key, mapped_type&& value) {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = value_type(std::move(key), std::move(value));
        return true;
    }

    // 删除所有匹配键的元素
    constexpr size_type erase(key_type const& key) noexcept {
        size_type write_idx = 0;
        for (size_type read_idx = 0; read_idx < current_size_; ++read_idx) {
            if (storage_[read_idx].key != key) {
                if (write_idx != read_idx) {
                    storage_[write_idx] = std::move(storage_[read_idx]);
                }
                ++write_idx;
            }
        }
        size_type removed = current_size_ - write_idx;
        current_size_ = write_idx;
        return removed;
    }

    constexpr void clear() noexcept { current_size_ = 0; }
};

// ============================================================
// constexpr_flat_map - 扁平化映射（缓存友好）
// ============================================================
template <typename Key, typename Value, std::size_t N>
class constexpr_flat_map {
public:
    using key_type = Key;
    using mapped_type = Value;
    using size_type = std::size_t;
    using value_type = constexpr_map_value<Key, Value>;

private:
    std::array<Key, N> keys_{};
    std::array<Value, N> values_{};
    size_type current_size_{0};

public:
    constexpr constexpr_flat_map() = default;

    constexpr size_type size() const noexcept { return current_size_; }
    constexpr bool empty() const noexcept { return current_size_ == 0; }
    constexpr bool full() const noexcept { return current_size_ >= N; }

    // 迭代器
    struct iterator {
        Key* key_ptr;
        Value* value_ptr;

        constexpr value_type operator*() const { return value_type{*key_ptr, *value_ptr}; }
        constexpr iterator& operator++() { ++key_ptr; ++value_ptr; return *this; }
        constexpr bool operator!=(iterator const& other) const { return key_ptr != other.key_ptr; }
    };

    struct const_iterator {
        Key const* key_ptr;
        Value const* value_ptr;

        constexpr value_type operator*() const { return value_type{*key_ptr, *value_ptr}; }
        constexpr const_iterator& operator++() { ++key_ptr; ++value_ptr; return *this; }
        constexpr bool operator!=(const_iterator const& other) const { return key_ptr != other.key_ptr; }
    };

    constexpr iterator begin() noexcept { return {keys_.data(), values_.data()}; }
    constexpr iterator end() noexcept { return {keys_.data() + current_size_, values_.data() + current_size_}; }
    constexpr const_iterator begin() const noexcept { return {keys_.data(), values_.data()}; }
    constexpr const_iterator end() const noexcept { return {keys_.data() + current_size_, values_.data() + current_size_}; }

    constexpr bool contains(key_type const& key) const noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (keys_[i] == key) return true;
        }
        return false;
    }

    constexpr Value* find(key_type const& key) noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (keys_[i] == key) return &values_[i];
        }
        return nullptr;
    }

    constexpr Value const* find(key_type const& key) const noexcept {
        for (size_type i = 0; i < current_size_; ++i) {
            if (keys_[i] == key) return &values_[i];
        }
        return nullptr;
    }

    constexpr Value& operator[](key_type const& key) {
        for (size_type i = 0; i < current_size_; ++i) {
            if (keys_[i] == key) return values_[i];
        }
        if (current_size_ < N) {
            keys_[current_size_] = key;
            return values_[current_size_++];
        }
        throw std::out_of_range("constexpr_flat_map is full");
    }

    constexpr bool insert(key_type const& key, mapped_type const& value) {
        if (current_size_ >= N || contains(key)) return false;
        keys_[current_size_] = key;
        values_[current_size_] = value;
        ++current_size_;
        return true;
    }

    // 直接访问数组（用于高性能遍历）
    constexpr Key const* keys_data() const noexcept { return keys_.data(); }
    constexpr Value const* values_data() const noexcept { return values_.data(); }
    constexpr size_type keys_size() const noexcept { return current_size_; }
};

// ============================================================
// 非成员函数
// ============================================================

template <typename K, typename V, size_t N>
constexpr void swap(constexpr_map<K, V, N>& lhs, constexpr_map<K, V, N>& rhs) noexcept {
    lhs.swap(rhs);
}

// 创建 constexpr_map 的工具函数
template <typename K, typename V, typename... Ts>
consteval auto make_constexpr_map(Ts&&... pairs) {
    constexpr size_t N = sizeof...(pairs);
    constexpr_map<K, V, N> result;
    (result.insert(std::forward<Ts>(pairs).first, std::forward<Ts>(pairs).second), ...);
    return result;
}

// 辅助函数：简化 pair 创建
template <typename K, typename V>
constexpr auto make_pair(K k, V v) {
    return std::pair<K, V>{k, v};
}

} // namespace constexpr_

// ct_capacity_v 特化 - 主模板在 iterator.h 中定义


template <typename K, typename V, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_map<K, V, N>> = N;

template <typename K, typename V, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_sorted_map<K, V, N>> = N;

template <typename K, typename V, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_multimap<K, V, N>> = N;

template <typename K, typename V, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_flat_map<K, V, N>> = N;

} // namespace shine
