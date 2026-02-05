#pragma once
#include <array>
#include <cstddef>
#include <type_traits>

namespace shine {
namespace reflection {
namespace constexpr_detail {

// 简化的编译期向量实现
template<typename T, size_t Capacity>
class simple_constexpr_vector {
private:
    std::array<T, Capacity> data_{};
    size_t size_ = 0;

public:
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    
    static constexpr size_t capacity_value = Capacity;

    constexpr simple_constexpr_vector() = default;
    
    template<typename... Args>
    constexpr simple_constexpr_vector(Args&&... args) 
        : size_(sizeof...(Args)) {
        if constexpr (sizeof...(Args) > 0) {
            size_t index = 0;
            ((data_[index++] = args), ...);
        }
    }
    
    constexpr size_type size() const { return size_; }
    constexpr size_type max_size() const { return Capacity; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr size_type capacity() const { return capacity_value; }
    
    constexpr reference operator[](size_type pos) { return data_[pos]; }
    constexpr const_reference operator[](size_type pos) const { return data_[pos]; }
    
    constexpr reference at(size_type pos) {
        if (pos >= size_) throw "out of range";
        return data_[pos];
    }
    
    constexpr const_reference at(size_type pos) const {
        if (pos >= size_) throw "out of range";
        return data_[pos];
    }
    
    constexpr reference front() { return data_[0]; }
    constexpr const_reference front() const { return data_[0]; }
    
    constexpr reference back() { return data_[size_ - 1]; }
    constexpr const_reference back() const { return data_[size_ - 1]; }
    
    constexpr pointer data() { return data_.data(); }
    constexpr const_pointer data() const { return data_.data(); }
    
    constexpr void push_back(const T& value) {
        if (size_ < capacity_value) {
            data_[size_++] = value;
        }
    }
    
    constexpr void push_back(T&& value) {
        if (size_ < capacity_value) {
            data_[size_++] = std::move(value);
        }
    }
    
    constexpr void clear() { size_ = 0; }
    
    constexpr auto begin() { return data_.begin(); }
    constexpr auto end() { return data_.begin() + size_; }
    constexpr auto begin() const { return data_.begin(); }
    constexpr auto end() const { return data_.begin() + size_; }
    constexpr auto cbegin() const { return data_.cbegin(); }
    constexpr auto cend() const { return data_.cbegin() + size_; }
};

// 简化的编译期映射实现
template<typename Key, typename Value, size_t Capacity>
class simple_constexpr_map {
private:
    struct entry_type {
        Key key{};
        Value value{};
        bool occupied = false;
    };
    
    std::array<entry_type, Capacity> data_{};
    size_t size_ = 0;

public:
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = size_t;
    
    static constexpr size_t capacity_value = Capacity;

    constexpr simple_constexpr_map() = default;
    
    template<typename... Args>
    constexpr simple_constexpr_map(Args&&... args) {
        if constexpr (sizeof...(Args) > 0) {
            // 这里简化处理，实际应该解析pair参数
            size_ = 0;
        }
    }
    
    constexpr size_type size() const { return size_; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr size_type capacity() const { return capacity_value; }
    
    constexpr bool contains(const Key& key) const {
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i].occupied && data_[i].key == key) {
                return true;
            }
        }
        return false;
    }
    
    constexpr Value* find(const Key& key) {
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i].occupied && data_[i].key == key) {
                return &data_[i].value;
            }
        }
        return nullptr;
    }
    
    constexpr const Value* find(const Key& key) const {
        for (size_t i = 0; i < size_; ++i) {
            if (data_[i].occupied && data_[i].key == key) {
                return &data_[i].value;
            }
        }
        return nullptr;
    }
    
    constexpr std::pair<Value*, bool> insert(const std::pair<Key, Value>& pair) {
        if (size_ >= capacity_value) return {nullptr, false};
        
        // 检查是否已存在
        if (auto* existing = find(pair.first)) {
            return {existing, false};
        }
        
        // 插入新元素
        data_[size_] = {pair.first, pair.second, true};
        return {&data_[size_++].value, true};
    }
    
    // 重载版本：接受键值对作为独立参数
    constexpr std::pair<Value*, bool> insert(const Key& key, const Value& value) {
        return insert(std::pair<Key, Value>{key, value});
    }
    
    constexpr void clear() {
        for (size_t i = 0; i < size_; ++i) {
            data_[i].occupied = false;
        }
        size_ = 0;
    }
    
    // 简化的迭代器支持
    struct iterator {
        entry_type* ptr;
        size_t index;
        size_t size;
        
        constexpr iterator& operator++() {
            do {
                ++index;
            } while (index < size && !ptr[index].occupied);
            return *this;
        }
        
        constexpr bool operator!=(const iterator& other) const {
            return index != other.index;
        }
        
        constexpr value_type operator*() const {
            return {ptr[index].key, ptr[index].value};
        }
    };
    
    constexpr iterator begin() {
        size_t start = 0;
        while (start < size_ && !data_[start].occupied) ++start;
        return {data_.data(), start, size_};
    }
    
    constexpr iterator end() {
        return {data_.data(), size_, size_};
    }
};

} // namespace constexpr_detail

// 便利的别名
template<typename T, size_t Capacity>
using constexpr_vector = constexpr_detail::simple_constexpr_vector<T, Capacity>;

template<typename Key, typename Value, size_t Capacity>
using constexpr_map = constexpr_detail::simple_constexpr_map<Key, Value, Capacity>;

} // namespace reflection
} // namespace shine

// 全局便利别名
namespace shine {
    template<typename T, size_t Capacity>
    using constexpr_vector = reflection::constexpr_detail::simple_constexpr_vector<T, Capacity>;
    
    template<typename Key, typename Value, size_t Capacity>
    using constexpr_map = reflection::constexpr_detail::simple_constexpr_map<Key, Value, Capacity>;
}