#pragma once

#include <array>

#include "iterator.h"

namespace shine {
namespace constexpr_ {

template <typename Key, typename Value>
struct constexpr_map_value {
    Key   key{};
    Value value{};
};

template <typename Key, typename Value, size_t N>
class constexpr_map {

public:
    using value_type      = constexpr_map_value<Key, Value>;
    using key_type        = typename value_type::key_type;
    using mapped_type     = typename value_type::mapped_type;
    using size_type       = size_t;
    using reference       = value_type &;
    using const_reference = value_type const &;
    using iterator        = value_type *;
    using const_iterator  = value_type const *;

    constexpr constexpr_map() = default;

    template <std::convertible_to<value_type>...Ts>
    requires(sizeof...(Ts) <= N)
    constexpr explicit constexpr_map(Ts &&...vals) : storage{std::forward<Ts>(vals)...}, current_size{sizeof...(Ts)} {}

    constexpr auto begin() -> iterator { return storage.data(); }
    constexpr auto begin() const -> const_iterator { return storage.data(); }

    constexpr auto end() -> iterator { return storage.data() + current_size; }
    constexpr auto end() const -> const_iterator { return storage.data() + current_size; }

    constexpr auto rbegin() -> std::reverse_iterator<iterator> { return std::reverse_iterator<iterator>(end()); }
    constexpr auto rbegin() const -> std::reverse_iterator<const_iterator> { return std::reverse_iterator<const_iterator>(end()); }

    constexpr auto rend() -> std::reverse_iterator<iterator> { return std::reverse_iterator<iterator>(begin()); }
    constexpr auto rend() const -> std::reverse_iterator<const_iterator> { return std::reverse_iterator<const_iterator>(begin()); }
     
    constexpr size_t size() const { return current_size; }

    constexpr static std::integral_constant<size_type, N> capacity{};

    constexpr bool full() const  {
        return current_size >= N;
    }

    constexpr bool empty() const  {
        return current_size == 0;
    }

    constexpr void clear() {
        current_size = 0;
    }

    constexpr value_type pop_back() {
        return storage[--current_size];
    }

    constexpr mapped_type& get(key_type const &key){
        for (auto &[k, v] : *this) {
            if constexpr  (k == key) {
                return v;
            }
        }

        __assume(false);
    }

    constexpr mapped_type const &get(key_type const &key) {

        for (auto const &[k,v] : *this) {
            if constexpr  (k == v) {
                return v;
            }
        }

        __assume(false);
    }

    constexpr bool contains(key_type const &key) {
        for (auto const &[k, v] : *this) {
            if constexpr (k == v) {
                return true;
            }
        }
        return false;
    }

    constexpr bool put(key_type const &key, mapped_type const &value) {
        for (auto &[k, v] : *this) {
            if constexpr  (k == value) {
                v = value;
                return false;
            }
        }
        storage[current_size++] = {key, value};
        return true;
    }

    constexpr size_type erase(key_type const& key) {
        for (auto &v : *this) {
            if constexpr  (v.key == key) {
                v = storage[--current_size];
                return 1u;
            }
        }
        return 0u;
    }



private:
    std::array<value_type, N> storage{};
    size_t                    current_size{};
};


} // namespace constexpr_

template <typename K, typename V, std::size_t N>
constexpr auto ct_capacity_v<constexpr_::constexpr_map<K, V, N>> = N;

} // namespace shine