#pragma once

#include <array>
#include <concepts>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <compare>
#include <algorithm>
#include <memory>
#include <ranges>

#include "iterator.h"

namespace shine {
namespace constexpr_ {

// ============================================================
// constexpr_vector - 改进版
// 新增：小缓冲优化、算法支持、内存优化、ranges兼容
// ============================================================

template <typename T, std::size_t N>
class constexpr_vector {
    static_assert(N > 0, "constexpr_vector capacity must be greater than 0");
    static_assert(std::is_destructible_v<T>, "T must be destructible");

public:
    // ==================== 类型别名 ====================
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = T const&;
    using pointer = T*;
    using const_pointer = T const*;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    // ==================== 数据成员 ====================
    // 使用 alignas 确保正确对齐
    alignas(alignof(T)) std::array<T, N> storage_{};
    size_type current_size_{0};

    // 辅助函数：检查是否在编译期
    static consteval bool is_consteval() { return true; }
    static constexpr bool is_runtime() { return true; }

public:
    // ==================== 静态常量 ====================
    static constexpr size_type capacity_v = N;
    static constexpr size_type max_size_v = N;

    // ==================== 构造函数 ====================

    // 默认构造函数
    constexpr constexpr_vector() = default;

    // 可变参数构造函数
    template <std::convertible_to<T>... Ts>
        requires(sizeof...(Ts) <= N)
    constexpr explicit constexpr_vector(Ts const&... args)
        : storage_{static_cast<T>(args)...}, current_size_{sizeof...(Ts)} {}

    // 迭代器范围构造函数 (C++20 ranges 兼容)
    template <std::input_iterator InputIt>
    constexpr constexpr_vector(InputIt first, InputIt last) {
        for (; first != last && current_size_ < N; ++first) {
            storage_[current_size_++] = *first;
        }
    }

    // 从 range 构造 (C++20)
    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T>
    constexpr explicit constexpr_vector(R&& range) {
        for (auto&& elem : std::forward<R>(range)) {
            if (current_size_ >= N) break;
            storage_[current_size_++] = static_cast<T>(std::forward<decltype(elem)>(elem));
        }
    }

    // 拷贝/移动构造函数
    constexpr constexpr_vector(constexpr_vector const&) = default;
    constexpr constexpr_vector(constexpr_vector&&) = default;

    // 从初始化列表构造
    constexpr constexpr_vector(std::initializer_list<T> init) {
        for (auto it = init.begin(); it != init.end() && current_size_ < N; ++it) {
            storage_[current_size_++] = *it;
        }
    }

    // 填充构造函数
    constexpr explicit constexpr_vector(size_type count, T const& value = T{}) 
        : current_size_{(std::min)(count, N)} {
        for (size_type i = 0; i < current_size_; ++i) {
            storage_[i] = value;
        }
    }

    // ==================== 赋值运算符 ====================

    constexpr constexpr_vector& operator=(constexpr_vector const&) = default;
    constexpr constexpr_vector& operator=(constexpr_vector&&) = default;

    constexpr constexpr_vector& operator=(std::initializer_list<T> init) {
        clear();
        for (auto it = init.begin(); it != init.end() && current_size_ < N; ++it) {
            storage_[current_size_++] = *it;
        }
        return *this;
    }

    // 从 range 赋值
    template <std::ranges::input_range R>
        requires std::convertible_to<std::ranges::range_reference_t<R>, T>
    constexpr constexpr_vector& operator=(R&& range) {
        clear();
        for (auto&& elem : std::forward<R>(range)) {
            if (current_size_ >= N) break;
            storage_[current_size_++] = static_cast<T>(std::forward<decltype(elem)>(elem));
        }
        return *this;
    }

    // ==================== 元素访问 ====================

    [[msvc::forceinline]]
    constexpr auto data() noexcept -> pointer {
        return std::data(storage_);
    }

    [[msvc::forceinline]]
    constexpr auto data() const noexcept -> const_pointer {
        return std::data(storage_);
    }

#if _MSVC_LANG >= 202302L
    // C++23 deducing this
    template <typename Self>
    [[msvc::forceinline]]
    constexpr auto operator[](this Self&& self, size_type index) noexcept
        -> decltype(auto) {
        __assume(index < self.current_size_);
        return std::forward<Self>(self).storage_[index];
    }
#else
    [[msvc::forceinline]]
    constexpr auto operator[](size_type index) noexcept -> reference {
        __assume(index < current_size_);
        return storage_[index];
    }

    [[msvc::forceinline]]
    constexpr auto operator[](size_type index) const noexcept -> const_reference {
        __assume(index < current_size_);
        return storage_[index];
    }
#endif

    // 带边界检查的访问
    constexpr auto at(size_type index) -> reference {
        if (index >= current_size_) [[unlikely]] {
            throw std::out_of_range("constexpr_vector index out of range");
        }
        return storage_[index];
    }

    constexpr auto at(size_type index) const -> const_reference {
        if (index >= current_size_) [[unlikely]] {
            throw std::out_of_range("constexpr_vector index out of range");
        }
        return storage_[index];
    }

    [[msvc::forceinline]]
    constexpr auto front() noexcept -> reference {
        __assume(current_size_ > 0);
        return storage_[0];
    }

    [[msvc::forceinline]]
    constexpr auto front() const noexcept -> const_reference {
        __assume(current_size_ > 0);
        return storage_[0];
    }

    [[msvc::forceinline]]
    constexpr auto back() noexcept -> reference {
        __assume(current_size_ > 0);
        return storage_[current_size_ - 1];
    }

    [[msvc::forceinline]]
    constexpr auto back() const noexcept -> const_reference {
        __assume(current_size_ > 0);
        return storage_[current_size_ - 1];
    }

    // 编译期索引访问
    template <size_type Index>
    [[msvc::forceinline]]
    constexpr auto get() noexcept -> reference {
        static_assert(Index < N, "Index out of bounds");
        return std::get<Index>(storage_);
    }

    template <size_type Index>
    [[msvc::forceinline]]
    constexpr auto get() const noexcept -> const_reference {
        static_assert(Index < N, "Index out of bounds");
        return std::get<Index>(storage_);
    }

    // ==================== 迭代器 ====================

    [[msvc::forceinline]]
    constexpr auto begin() noexcept -> iterator { return data(); }

    [[msvc::forceinline]]
    constexpr auto begin() const noexcept -> const_iterator { return data(); }

    [[msvc::forceinline]]
    constexpr auto cbegin() const noexcept -> const_iterator { return data(); }

    [[msvc::forceinline]]
    constexpr auto end() noexcept -> iterator { return data() + current_size_; }

    [[msvc::forceinline]]
    constexpr auto end() const noexcept -> const_iterator { return data() + current_size_; }

    [[msvc::forceinline]]
    constexpr auto cend() const noexcept -> const_iterator { return data() + current_size_; }

    [[msvc::forceinline]]
    constexpr auto rbegin() noexcept -> reverse_iterator { return reverse_iterator(end()); }

    [[msvc::forceinline]]
    constexpr auto rbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(end());
    }

    [[msvc::forceinline]]
    constexpr auto crbegin() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cend());
    }

    [[msvc::forceinline]]
    constexpr auto rend() noexcept -> reverse_iterator { return reverse_iterator(begin()); }

    [[msvc::forceinline]]
    constexpr auto rend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(begin());
    }

    [[msvc::forceinline]]
    constexpr auto crend() const noexcept -> const_reverse_iterator {
        return const_reverse_iterator(cbegin());
    }

    // ==================== 容量 ====================

    [[msvc::forceinline]]
    constexpr auto size() const noexcept -> size_type { return current_size_; }

    [[msvc::forceinline]]
    constexpr auto empty() const noexcept -> bool { return current_size_ == 0; }

    [[msvc::forceinline]]
    constexpr auto full() const noexcept -> bool { return current_size_ == N; }

    [[msvc::forceinline]]
    constexpr auto capacity() const noexcept -> size_type { return N; }

    [[msvc::forceinline]]
    constexpr auto max_size() const noexcept -> size_type { return N; }

    // 剩余容量
    [[msvc::forceinline]]
    constexpr auto available() const noexcept -> size_type { return N - current_size_; }

    // ==================== 修改器 ====================

    // push_back - 左值引用版本
    constexpr auto push_back(T const& value) -> reference {
        if (current_size_ >= N) [[unlikely]] {
            throw std::out_of_range("constexpr_vector is full");
        }
        __assume(current_size_ < N);
        return storage_[current_size_++] = value;
    }

    // push_back - 右值引用版本
    constexpr auto push_back(T&& value) -> reference {
        if (current_size_ >= N) [[unlikely]] {
            throw std::out_of_range("constexpr_vector is full");
        }
        __assume(current_size_ < N);
        return storage_[current_size_++] = std::move(value);
    }

    // try_push_back - 不抛异常版本
    constexpr bool try_push_back(T const& value) noexcept {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = value;
        return true;
    }

    constexpr bool try_push_back(T&& value) noexcept {
        if (current_size_ >= N) return false;
        storage_[current_size_++] = std::move(value);
        return true;
    }

    // emplace_back - C++23 风格的就地构造
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr auto emplace_back(Args&&... args) -> reference {
        if (current_size_ >= N) [[unlikely]] {
            throw std::out_of_range("constexpr_vector is full");
        }
        __assume(current_size_ < N);
        if consteval {
            std::construct_at(std::addressof(storage_[current_size_]), std::forward<Args>(args)...);
        } else {
            storage_[current_size_] = T(std::forward<Args>(args)...);
        }
        return storage_[current_size_++];
    }

    // try_emplace_back - 不抛异常版本
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr bool try_emplace_back(Args&&... args) noexcept {
        if (current_size_ >= N) return false;
        if consteval {
            std::construct_at(std::addressof(storage_[current_size_]), std::forward<Args>(args)...);
        } else {
            storage_[current_size_] = T(std::forward<Args>(args)...);
        }
        ++current_size_;
        return true;
    }

    // pop_back
    constexpr auto pop_back() -> T {
        if (current_size_ == 0) [[unlikely]] {
            throw std::out_of_range("constexpr_vector is empty");
        }
        __assume(current_size_ > 0);
        return std::move(storage_[--current_size_]);
    }

    // try_pop_back - 返回 optional
    constexpr std::optional<T> try_pop_back() noexcept {
        if (current_size_ == 0) return std::nullopt;
        return std::move(storage_[--current_size_]);
    }

    // pop_back_discard - 无返回值版本
    constexpr void pop_back_discard() {
        if (current_size_ == 0) [[unlikely]] {
            throw std::out_of_range("constexpr_vector is empty");
        }
        __assume(current_size_ > 0);
        --current_size_;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(std::addressof(storage_[current_size_]));
        }
    }

    // clear
    constexpr void clear() noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (size_type i = 0; i < current_size_; ++i) {
                std::destroy_at(std::addressof(storage_[i]));
            }
        }
        current_size_ = 0;
    }

    // resize
    constexpr void resize(size_type new_size)
        requires std::default_initializable<T> {
        if (new_size > N) [[unlikely]] {
            throw std::out_of_range("constexpr_vector resize exceeds capacity");
        }

        if (new_size > current_size_) {
            for (size_type i = current_size_; i < new_size; ++i) {
                std::construct_at(std::addressof(storage_[i]));
            }
        } else if (new_size < current_size_) {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                for (size_type i = new_size; i < current_size_; ++i) {
                    std::destroy_at(std::addressof(storage_[i]));
                }
            }
        }
        current_size_ = new_size;
    }

    constexpr void resize(size_type new_size, T const& value) {
        if (new_size > N) [[unlikely]] {
            throw std::out_of_range("constexpr_vector resize exceeds capacity");
        }

        if (new_size > current_size_) {
            for (size_type i = current_size_; i < new_size; ++i) {
                storage_[i] = value;
            }
        }
        current_size_ = new_size;
    }

    // fill - 填充所有元素
    constexpr void fill(T const& value) {
        std::fill_n(data(), N, value);
        current_size_ = N;
    }

    // assign - 赋值范围
    template <std::input_iterator InputIt>
    constexpr void assign(InputIt first, InputIt last) {
        clear();
        for (; first != last && current_size_ < N; ++first) {
            storage_[current_size_++] = *first;
        }
    }

    constexpr void assign(size_type count, T const& value) {
        clear();
        count = (std::min)(count, N);
        for (size_type i = 0; i < count; ++i) {
            storage_[current_size_++] = value;
        }
    }

    // swap
    constexpr void swap(constexpr_vector& other) noexcept
        requires std::is_swappable_v<T> {
        using std::swap;
        size_type const min_size = (std::min)(current_size_, other.current_size_);
        for (size_type i = 0; i < min_size; ++i) {
            swap(storage_[i], other.storage_[i]);
        }

        if (current_size_ > other.current_size_) {
            for (size_type i = other.current_size_; i < current_size_; ++i) {
                other.storage_[i] = std::move(storage_[i]);
            }
        } else if (other.current_size_ > current_size_) {
            for (size_type i = current_size_; i < other.current_size_; ++i) {
                storage_[i] = std::move(other.storage_[i]);
            }
        }

        swap(current_size_, other.current_size_);
    }

    // insert
    constexpr iterator insert(const_iterator pos, T const& value) {
        size_type const index = pos - cbegin();
        __assume(index <= current_size_);

        if (full()) [[unlikely]] {
            return begin() + index;
        }

        for (size_type i = current_size_; i > index; --i) {
            storage_[i] = std::move(storage_[i - 1]);
        }

        storage_[index] = value;
        ++current_size_;
        return begin() + index;
    }

    constexpr iterator insert(const_iterator pos, T&& value) {
        size_type const index = pos - cbegin();
        __assume(index <= current_size_);

        if (full()) [[unlikely]] {
            return begin() + index;
        }

        for (size_type i = current_size_; i > index; --i) {
            storage_[i] = std::move(storage_[i - 1]);
        }

        storage_[index] = std::move(value);
        ++current_size_;
        return begin() + index;
    }

    // 批量插入
    constexpr iterator insert(const_iterator pos, size_type count, T const& value) {
        size_type const index = pos - cbegin();
        __assume(index <= current_size_);

        count = (std::min)(count, N - current_size_);
        if (count == 0) return begin() + index;

        // 移动现有元素
        for (size_type i = current_size_ + count - 1; i >= index + count; --i) {
            storage_[i] = std::move(storage_[i - count]);
        }

        // 插入新元素
        for (size_type i = 0; i < count; ++i) {
            storage_[index + i] = value;
        }

        current_size_ += count;
        return begin() + index;
    }

    // erase
    constexpr iterator erase(const_iterator pos) {
        size_type const index = pos - cbegin();
        __assume(index < current_size_);

        for (size_type i = index; i < current_size_ - 1; ++i) {
            storage_[i] = std::move(storage_[i + 1]);
        }

        --current_size_;
        return begin() + index;
    }

    constexpr iterator erase(const_iterator first, const_iterator last) {
        size_type const first_index = first - cbegin();
        size_type const last_index = last - cbegin();
        __assume(first_index <= last_index && last_index <= current_size_);

        size_type const range_size = last_index - first_index;

        for (size_type i = first_index; i + range_size < current_size_; ++i) {
            storage_[i] = std::move(storage_[i + range_size]);
        }

        current_size_ -= range_size;
        return begin() + first_index;
    }

    // 快速删除（与末尾交换，不保持顺序）
    constexpr void erase_unordered(size_type index) {
        __assume(index < current_size_);
        if (index != current_size_ - 1) {
            storage_[index] = std::move(storage_[current_size_ - 1]);
        }
        --current_size_;
    }

    // ==================== 查找操作 ====================

    constexpr bool contains(T const& value) const
        requires std::equality_comparable<T> {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i] == value) return true;
        }
        return false;
    }

    constexpr iterator find(T const& value) {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i] == value) return begin() + i;
        }
        return end();
    }

    constexpr const_iterator find(T const& value) const {
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i] == value) return cbegin() + i;
        }
        return cend();
    }

    constexpr size_type count(T const& value) const
        requires std::equality_comparable<T> {
        size_type cnt = 0;
        for (size_type i = 0; i < current_size_; ++i) {
            if (storage_[i] == value) ++cnt;
        }
        return cnt;
    }

    // ==================== 算法支持 ====================

    // 排序
    constexpr void sort()
        requires std::totally_ordered<T> {
        std::sort(begin(), end());
    }

    template <typename Compare>
    constexpr void sort(Compare comp) {
        std::sort(begin(), end(), comp);
    }

    // 稳定排序
    constexpr void stable_sort()
        requires std::totally_ordered<T> {
        std::stable_sort(begin(), end());
    }

    template <typename Compare>
    constexpr void stable_sort(Compare comp) {
        std::stable_sort(begin(), end(), comp);
    }

    // 去重（需要先排序）
    constexpr size_type unique()
        requires std::equality_comparable<T> {
        if (current_size_ < 2) return current_size_;

        size_type new_size = 1;
        for (size_type i = 1; i < current_size_; ++i) {
            if (storage_[i] != storage_[new_size - 1]) {
                storage_[new_size++] = std::move(storage_[i]);
            }
        }
        current_size_ = new_size;
        return current_size_;
    }

    // 反转
    constexpr void reverse() noexcept {
        for (size_type i = 0; i < current_size_ / 2; ++i) {
            using std::swap;
            swap(storage_[i], storage_[current_size_ - 1 - i]);
        }
    }

    // 变换（原地）
    template <typename UnaryOp>
        requires std::invocable<UnaryOp, T&>
    constexpr void transform_inplace(UnaryOp op) {
        for (size_type i = 0; i < current_size_; ++i) {
            storage_[i] = op(storage_[i]);
        }
    }

    // 过滤（移除不满足条件的元素）
    template <typename Predicate>
        requires std::predicate<Predicate, T const&>
    constexpr size_type filter(Predicate pred) {
        size_type new_size = 0;
        for (size_type i = 0; i < current_size_; ++i) {
            if (pred(storage_[i])) {
                if (new_size != i) {
                    storage_[new_size] = std::move(storage_[i]);
                }
                ++new_size;
            }
        }
        current_size_ = new_size;
        return current_size_;
    }

    // 是否全部满足
    template <typename Predicate>
        requires std::predicate<Predicate, T const&>
    constexpr bool all_of(Predicate pred) const {
        for (size_type i = 0; i < current_size_; ++i) {
            if (!pred(storage_[i])) return false;
        }
        return true;
    }

    // 是否任一满足
    template <typename Predicate>
        requires std::predicate<Predicate, T const&>
    constexpr bool any_of(Predicate pred) const {
        for (size_type i = 0; i < current_size_; ++i) {
            if (pred(storage_[i])) return true;
        }
        return false;
    }

    // 是否全不满足
    template <typename Predicate>
        requires std::predicate<Predicate, T const&>
    constexpr bool none_of(Predicate pred) const {
        return !any_of(std::move(pred));
    }

    // 折叠/归约
    template <typename BinaryOp, typename U = T>
    constexpr auto fold(U init, BinaryOp op) const -> U {
        U result = std::move(init);
        for (size_type i = 0; i < current_size_; ++i) {
            result = op(std::move(result), storage_[i]);
        }
        return result;
    }

    // 求和
    constexpr auto sum() const -> T
        requires requires(T a, T b) { a + b; } {
        T result{};
        for (size_type i = 0; i < current_size_; ++i) {
            result += storage_[i];
        }
        return result;
    }

    // 查找最小/最大
    constexpr iterator min_element()
        requires std::totally_ordered<T> {
        if (current_size_ == 0) return end();
        iterator result = begin();
        for (size_type i = 1; i < current_size_; ++i) {
            if (storage_[i] < *result) {
                result = begin() + i;
            }
        }
        return result;
    }

    constexpr iterator max_element()
        requires std::totally_ordered<T> {
        if (current_size_ == 0) return end();
        iterator result = begin();
        for (size_type i = 1; i < current_size_; ++i) {
            if (storage_[i] > *result) {
                result = begin() + i;
            }
        }
        return result;
    }

    // 二分查找（需要已排序）
    constexpr bool binary_search(T const& value) const
        requires std::totally_ordered<T> {
        return std::binary_search(begin(), end(), value);
    }

    constexpr iterator lower_bound(T const& value)
        requires std::totally_ordered<T> {
        return std::lower_bound(begin(), end(), value);
    }

    constexpr const_iterator lower_bound(T const& value) const
        requires std::totally_ordered<T> {
        return std::lower_bound(cbegin(), cend(), value);
    }

    // ==================== 比较运算符 ====================

    constexpr auto operator<=>(constexpr_vector const& other) const
        requires std::three_way_comparable<T> {
        if (auto cmp = current_size_ <=> other.current_size_; cmp != 0) {
            return cmp;
        }

        for (size_type i = 0; i < current_size_; ++i) {
            if (auto cmp = storage_[i] <=> other.storage_[i]; cmp != 0) {
                return cmp;
            }
        }

        return std::strong_ordering::equal;
    }

    constexpr bool operator==(constexpr_vector const& other) const
        requires std::equality_comparable<T> {
        if (current_size_ != other.current_size_) return false;
        for (size_type i = 0; i < current_size_; ++i) {
            if (!(storage_[i] == other.storage_[i])) return false;
        }
        return true;
    }

    // ==================== 友元函数 ====================

    template <typename F>
    friend constexpr void resize_and_overwrite(constexpr_vector& v, F&& f) {
        v.current_size_ = std::forward<F>(f)(v.data(), v.current_size_);
    }
};

// ============================================================
// 非成员函数
// ============================================================

// CTAD 推导指引
template <typename T, typename... Ts>
constexpr_vector(T, Ts...) -> constexpr_vector<T, 1 + sizeof...(Ts)>;

template <std::input_iterator InputIt>
constexpr_vector(InputIt, InputIt) -> constexpr_vector<std::iter_value_t<InputIt>, 256>;

template <std::ranges::input_range R>
constexpr_vector(R&&) -> constexpr_vector<std::ranges::range_value_t<R>, 256>;

// get 函数 - 编译期索引访问
template <std::size_t I, typename T, std::size_t N>
[[msvc::forceinline]]
constexpr auto get(constexpr_vector<T, N>& v) -> decltype(auto) {
    static_assert(I < N, "Index out of bounds");
    return v.template get<I>();
}

template <std::size_t I, typename T, std::size_t N>
[[msvc::forceinline]]
constexpr auto get(constexpr_vector<T, N> const& v) -> decltype(auto) {
    static_assert(I < N, "Index out of bounds");
    return v.template get<I>();
}

// swap 重载
template <typename T, std::size_t N>
constexpr void swap(constexpr_vector<T, N>& lhs, constexpr_vector<T, N>& rhs) noexcept {
    lhs.swap(rhs);
}

// ============================================================
// small_vector - 小缓冲优化向量
// ============================================================

template <typename T, std::size_t SmallSize = 16>
class small_vector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = T const&;
    using pointer = T*;
    using const_pointer = T const*;
    using iterator = pointer;
    using const_iterator = const_pointer;

private:
    // 小缓冲区
    alignas(alignof(T)) std::array<std::byte, SmallSize * sizeof(T)> small_storage_{};
    pointer small_data_ = reinterpret_cast<T*>(small_storage_.data());
    pointer heap_data_ = nullptr;
    size_type current_size_ = 0;
    size_type heap_capacity_ = 0;
    bool is_small_ = true;

    constexpr bool is_small() const noexcept {
        return is_small_;
    }

    constexpr pointer data_ptr() noexcept {
        return is_small_ ? small_data_ : heap_data_;
    }

    constexpr const_pointer data_ptr() const noexcept {
        return is_small_ ? small_data_ : heap_data_;
    }

public:
    constexpr small_vector() = default;

    constexpr ~small_vector() {
        clear();
        if (!is_small_ && heap_data_) {
            ::operator delete(heap_data_);
        }
    }

    // 禁止拷贝（简化实现）
    small_vector(small_vector const&) = delete;
    small_vector& operator=(small_vector const&) = delete;

    constexpr small_vector(small_vector&& other) noexcept
        : is_small_(other.is_small_)
        , current_size_(other.current_size_)
        , heap_capacity_(other.heap_capacity_) {
        if (other.is_small_) {
            // 移动小缓冲区内容
            for (size_type i = 0; i < current_size_; ++i) {
                new (small_data_ + i) T(std::move(other.small_data_[i]));
            }
        } else {
            // 接管堆内存
            heap_data_ = other.heap_data_;
            other.heap_data_ = nullptr;
        }
        other.current_size_ = 0;
        other.heap_capacity_ = 0;
    }

    constexpr small_vector& operator=(small_vector&& other) noexcept {
        if (this != &other) {
            clear();
            if (!is_small_ && heap_data_) {
                ::operator delete(heap_data_);
            }

            is_small_ = other.is_small_;
            current_size_ = other.current_size_;
            heap_capacity_ = other.heap_capacity_;

            if (other.is_small_) {
                for (size_type i = 0; i < current_size_; ++i) {
                    new (small_data_ + i) T(std::move(other.small_data_[i]));
                }
            } else {
                heap_data_ = other.heap_data_;
                other.heap_data_ = nullptr;
            }
            other.current_size_ = 0;
            other.heap_capacity_ = 0;
        }
        return *this;
    }

    constexpr void push_back(T const& value) {
        if (current_size_ >= (is_small_ ? SmallSize : heap_capacity_)) {
            grow();
        }
        new (data_ptr() + current_size_++) T(value);
    }

    constexpr void push_back(T&& value) {
        if (current_size_ >= (is_small_ ? SmallSize : heap_capacity_)) {
            grow();
        }
        new (data_ptr() + current_size_++) T(std::move(value));
    }

    template <typename... Args>
    constexpr reference emplace_back(Args&&... args) {
        if (current_size_ >= (is_small_ ? SmallSize : heap_capacity_)) {
            grow();
        }
        new (data_ptr() + current_size_) T(std::forward<Args>(args)...);
        return data_ptr()[current_size_++];
    }

    constexpr void pop_back() {
        if (current_size_ > 0) {
            data_ptr()[--current_size_].~T();
        }
    }

    constexpr void clear() {
        for (size_type i = 0; i < current_size_; ++i) {
            data_ptr()[i].~T();
        }
        current_size_ = 0;
    }

    constexpr size_type size() const noexcept { return current_size_; }
    constexpr bool empty() const noexcept { return current_size_ == 0; }
    constexpr size_type capacity() const noexcept {
        return is_small_ ? SmallSize : heap_capacity_;
    }

    constexpr reference operator[](size_type index) noexcept { return data_ptr()[index]; }
    constexpr const_reference operator[](size_type index) const noexcept { return data_ptr()[index]; }

    constexpr iterator begin() noexcept { return data_ptr(); }
    constexpr iterator end() noexcept { return data_ptr() + current_size_; }
    constexpr const_iterator begin() const noexcept { return data_ptr(); }
    constexpr const_iterator end() const noexcept { return data_ptr() + current_size_; }
    constexpr const_iterator cbegin() const noexcept { return data_ptr(); }
    constexpr const_iterator cend() const noexcept { return data_ptr() + current_size_; }

    constexpr pointer data() noexcept { return data_ptr(); }
    constexpr const_pointer data() const noexcept { return data_ptr(); }

    constexpr reference front() noexcept { return data_ptr()[0]; }
    constexpr const_reference front() const noexcept { return data_ptr()[0]; }
    constexpr reference back() noexcept { return data_ptr()[current_size_ - 1]; }
    constexpr const_reference back() const noexcept { return data_ptr()[current_size_ - 1]; }

private:
    constexpr void grow() {
        size_type new_capacity = is_small_ ? SmallSize * 2 : heap_capacity_ * 2;
        T* new_data = static_cast<T*>(::operator new(new_capacity * sizeof(T)));

        // 移动现有元素
        for (size_type i = 0; i < current_size_; ++i) {
            new (new_data + i) T(std::move(data_ptr()[i]));
            data_ptr()[i].~T();
        }

        // 释放旧堆内存
        if (!is_small_ && heap_data_) {
            ::operator delete(heap_data_);
        }

        heap_data_ = new_data;
        heap_capacity_ = new_capacity;
        is_small_ = false;
    }
};

} // namespace constexpr_
} // namespace shine

// ============================================================
// 标准库特化
// ============================================================

// std::tuple_size 特化
template <typename T, std::size_t N>
struct std::tuple_size<shine::constexpr_::constexpr_vector<T, N>>
    : std::integral_constant<std::size_t, N> {};

// std::tuple_element 特化
template <std::size_t I, typename T, std::size_t N>
struct std::tuple_element<I, shine::constexpr_::constexpr_vector<T, N>> {
    using type = T;
};

// capacity_v 变量模板特化 - 主模板在 iterator.h 中定义
namespace shine {

template <typename T, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_vector<T, N>> = N;

template <typename T, std::size_t N>
inline constexpr std::size_t ct_capacity_v<constexpr_::small_vector<T, N>> = N;
}