#pragma once

#include <array>
#include <concepts>
#include <utility>
#include <iterator>
#include <stdexcept>
#include <type_traits>
#include <compare>
#include <algorithm>

namespace shine
{
    namespace constexpr_
    {
        // ============================================================
        // C++23 MSVC Optimized constexpr_vector
        // 用于反射的编译期动态数组
        // ============================================================

        template<typename T, std::size_t N>
        class constexpr_vector
        {
            static_assert(N > 0, "constexpr_vector capacity must be greater than 0");

        public:
            // ==================== 类型别名 ====================
            using value_type             = T;
            using size_type              = std::size_t;
            using difference_type        = std::ptrdiff_t;
            using reference              = T&;
            using const_reference        = T const&;
            using pointer                = T*;
            using const_pointer          = T const*;
            using iterator               = pointer;
            using const_iterator         = const_pointer;
            using reverse_iterator       = std::reverse_iterator<iterator>;
            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        private:
            // ==================== 数据成员 ====================
            // 使用 [[no_unique_address]] 优化空类型存储
            // MSVC: 可能的空基类优化 (EBO)
            alignas(T) std::array<T, N> storage_{};
            size_type current_size_{0};

        public:
            // ==================== 静态常量 ====================
            static constexpr size_type capacity_v = N;
            static constexpr size_type max_size_v = N;

            // ==================== 构造函数 ====================

            // 默认构造函数
            constexpr constexpr_vector() = default;

            // 可变参数构造函数 - 使用 C++23 改进的折叠表达式
            template <std::convertible_to<T>... Ts>
                requires(sizeof...(Ts) <= N)
            constexpr explicit constexpr_vector(Ts const&... args)
                : storage_{static_cast<T>(args)...}, current_size_{sizeof...(Ts)}
            {
            }

            // 迭代器范围构造函数 (C++23 ranges 兼容)
            template <std::input_iterator InputIt>
            constexpr constexpr_vector(InputIt first, InputIt last)
            {
                for (; first != last && current_size_ < N; ++first)
                {
                    storage_[current_size_++] = *first;
                }
            }

            // 拷贝构造函数 (trivially copyable 时隐式生成更高效)
            constexpr constexpr_vector(constexpr_vector const&) = default;

            // 移动构造函数
            constexpr constexpr_vector(constexpr_vector&&) = default;

            // 从初始化列表构造
            constexpr constexpr_vector(std::initializer_list<T> init)
            {
                for (auto it = init.begin(); it != init.end() && current_size_ < N; ++it)
                {
                    storage_[current_size_++] = *it;
                }
            }

            // ==================== 赋值运算符 ====================

            constexpr constexpr_vector& operator=(constexpr_vector const&) = default;
            constexpr constexpr_vector& operator=(constexpr_vector&&) = default;

            constexpr constexpr_vector& operator=(std::initializer_list<T> init)
            {
                clear();
                for (auto it = init.begin(); it != init.end() && current_size_ < N; ++it)
                {
                    storage_[current_size_++] = *it;
                }
                return *this;
            }

            // ==================== 元素访问 ====================

            // MSVC: __forceinline 关键热路径
            [[msvc::forceinline]]
            constexpr auto data() noexcept -> pointer
            {
                return std::data(storage_);
            }

            [[msvc::forceinline]]
            constexpr auto data() const noexcept -> const_pointer
            {
                return std::data(storage_);
            }

            // 使用 C++23 deducing this 简化 const/non-const 重载
            // 注意: MSVC 19.34+ 支持 deducing this
#if _MSVC_LANG >= 202302L
            template<typename Self>
            [[msvc::forceinline]]
            constexpr auto operator[](this Self&& self, size_type index) noexcept
                -> decltype(auto)
            {
                // MSVC: __assume 帮助优化器
                __assume(index < self.current_size_);
                return std::forward<Self>(self).storage_[index];
            }
#else
            [[msvc::forceinline]]
            constexpr auto operator[](size_type index) noexcept -> reference
            {
                __assume(index < current_size_);
                return storage_[index];
            }

            [[msvc::forceinline]]
            constexpr auto operator[](size_type index) const noexcept -> const_reference
            {
                __assume(index < current_size_);
                return storage_[index];
            }
#endif

            // 带边界检查的访问
            constexpr auto at(size_type index) -> reference
            {
                if (index >= current_size_) [[unlikely]]
                {
                    throw std::out_of_range("constexpr_vector index out of range");
                }
                return storage_[index];
            }

            constexpr auto at(size_type index) const -> const_reference
            {
                if (index >= current_size_) [[unlikely]]
                {
                    throw std::out_of_range("constexpr_vector index out of range");
                }
                return storage_[index];
            }

            [[msvc::forceinline]]
            constexpr auto front() noexcept -> reference
            {
                __assume(current_size_ > 0);
                return storage_[0];
            }

            [[msvc::forceinline]]
            constexpr auto front() const noexcept -> const_reference
            {
                __assume(current_size_ > 0);
                return storage_[0];
            }

            [[msvc::forceinline]]
            constexpr auto back() noexcept -> reference
            {
                __assume(current_size_ > 0);
                return storage_[current_size_ - 1];
            }

            [[msvc::forceinline]]
            constexpr auto back() const noexcept -> const_reference
            {
                __assume(current_size_ > 0);
                return storage_[current_size_ - 1];
            }

            // 编译期索引访问
            template<size_type Index>
            [[msvc::forceinline]]
            constexpr auto get() noexcept -> reference
            {
                static_assert(Index < N, "Index out of bounds");
                return std::get<Index>(storage_);
            }

            template<size_type Index>
            [[msvc::forceinline]]
            constexpr auto get() const noexcept -> const_reference
            {
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
            constexpr auto rbegin() const noexcept -> const_reverse_iterator
            {
                return const_reverse_iterator(end());
            }

            [[msvc::forceinline]]
            constexpr auto crbegin() const noexcept -> const_reverse_iterator
            {
                return const_reverse_iterator(cend());
            }

            [[msvc::forceinline]]
            constexpr auto rend() noexcept -> reverse_iterator { return reverse_iterator(begin()); }

            [[msvc::forceinline]]
            constexpr auto rend() const noexcept -> const_reverse_iterator
            {
                return const_reverse_iterator(begin());
            }

            [[msvc::forceinline]]
            constexpr auto crend() const noexcept -> const_reverse_iterator
            {
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

            // ==================== 修改器 ====================

            // push_back - 左值引用版本
            constexpr auto push_back(T const& value) -> reference
            {
                __assume(current_size_ < N);
                return storage_[current_size_++] = value;
            }

            // push_back - 右值引用版本
            constexpr auto push_back(T&& value) -> reference
            {
                __assume(current_size_ < N);
                return storage_[current_size_++] = std::move(value);
            }

            // emplace_back - C++23 风格的就地构造
            template<typename... Args>
                requires std::constructible_from<T, Args...>
            constexpr auto emplace_back(Args&&... args) -> reference
            {
                __assume(current_size_ < N);
                // 使用 std::construct_at 或 placement new
                if consteval {
                    // 编译期: 使用 construct_at
                    std::construct_at(std::addressof(storage_[current_size_]), std::forward<Args>(args)...);
                } else {
                    // 运行期: 直接赋值
                    storage_[current_size_] = T(std::forward<Args>(args)...);
                }
                return storage_[current_size_++];
            }

            // pop_back
            constexpr auto pop_back() -> T
            {
                __assume(current_size_ > 0);
                return std::move(storage_[--current_size_]);
            }

            // pop_back 无返回值版本 - 避免不必要的移动
            constexpr void pop_back_discard()
            {
                __assume(current_size_ > 0);
                --current_size_;
                // 注意: 如果 T 有非平凡析构函数，需要显式销毁
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    std::destroy_at(std::addressof(storage_[current_size_]));
                }
            }

            // clear
            constexpr void clear() noexcept
            {
                if constexpr (!std::is_trivially_destructible_v<T>)
                {
                    for (size_type i = 0; i < current_size_; ++i)
                    {
                        std::destroy_at(std::addressof(storage_[i]));
                    }
                }
                current_size_ = 0;
            }

            // resize - C++23 风格
            constexpr void resize(size_type new_size)
                requires std::default_initializable<T>
            {
                if (new_size > N) [[unlikely]]
                {
                    throw std::out_of_range("constexpr_vector resize exceeds capacity");
                }

                if (new_size > current_size_)
                {
                    // 扩展: 默认构造新元素
                    for (size_type i = current_size_; i < new_size; ++i)
                    {
                        std::construct_at(std::addressof(storage_[i]));
                    }
                }
                else if (new_size < current_size_)
                {
                    // 收缩: 销毁多余元素
                    if constexpr (!std::is_trivially_destructible_v<T>)
                    {
                        for (size_type i = new_size; i < current_size_; ++i)
                        {
                            std::destroy_at(std::addressof(storage_[i]));
                        }
                    }
                }
                current_size_ = new_size;
            }

            constexpr void resize(size_type new_size, T const& value)
            {
                if (new_size > N) [[unlikely]]
                {
                    throw std::out_of_range("constexpr_vector resize exceeds capacity");
                }

                if (new_size > current_size_)
                {
                    for (size_type i = current_size_; i < new_size; ++i)
                    {
                        storage_[i] = value;
                    }
                }
                current_size_ = new_size;
            }

            // fill - 填充所有元素
            constexpr void fill(T const& value)
            {
                std::fill_n(data(), N, value);
                current_size_ = N;
            }

            // assign - 赋值范围
            template<std::input_iterator InputIt>
            constexpr void assign(InputIt first, InputIt last)
            {
                clear();
                for (; first != last && current_size_ < N; ++first)
                {
                    storage_[current_size_++] = *first;
                }
            }

            constexpr void assign(size_type count, T const& value)
            {
                clear();
                count = (std::min)(count, N);
                for (size_type i = 0; i < count; ++i)
                {
                    storage_[current_size_++] = value;
                }
            }

            // swap - C++23 风格
            constexpr void swap(constexpr_vector& other) noexcept
                requires std::is_swappable_v<T>
            {
                using std::swap;
                // 交换公共部分
                size_type const min_size = (std::min)(current_size_, other.current_size_);
                for (size_type i = 0; i < min_size; ++i)
                {
                    swap(storage_[i], other.storage_[i]);
                }

                // 移动多余元素
                if (current_size_ > other.current_size_)
                {
                    for (size_type i = other.current_size_; i < current_size_; ++i)
                    {
                        other.storage_[i] = std::move(storage_[i]);
                    }
                }
                else if (other.current_size_ > current_size_)
                {
                    for (size_type i = current_size_; i < other.current_size_; ++i)
                    {
                        storage_[i] = std::move(other.storage_[i]);
                    }
                }

                swap(current_size_, other.current_size_);
            }

            // insert - 在指定位置插入
            constexpr iterator insert(const_iterator pos, T const& value)
            {
                size_type const index = pos - cbegin();
                __assume(index <= current_size_);

                if (full()) [[unlikely]]
                {
                    return begin() + index;
                }

                // 移动元素
                for (size_type i = current_size_; i > index; --i)
                {
                    storage_[i] = std::move(storage_[i - 1]);
                }

                storage_[index] = value;
                ++current_size_;
                return begin() + index;
            }

            constexpr iterator insert(const_iterator pos, T&& value)
            {
                size_type const index = pos - cbegin();
                __assume(index <= current_size_);

                if (full()) [[unlikely]]
                {
                    return begin() + index;
                }

                for (size_type i = current_size_; i > index; --i)
                {
                    storage_[i] = std::move(storage_[i - 1]);
                }

                storage_[index] = std::move(value);
                ++current_size_;
                return begin() + index;
            }

            // erase - 删除指定位置元素
            constexpr iterator erase(const_iterator pos)
            {
                size_type const index = pos - cbegin();
                __assume(index < current_size_);

                for (size_type i = index; i < current_size_ - 1; ++i)
                {
                    storage_[i] = std::move(storage_[i + 1]);
                }

                --current_size_;
                return begin() + index;
            }

            constexpr iterator erase(const_iterator first, const_iterator last)
            {
                size_type const first_index = first - cbegin();
                size_type const last_index = last - cbegin();
                __assume(first_index <= last_index && last_index <= current_size_);

                size_type const range_size = last_index - first_index;

                for (size_type i = first_index; i + range_size < current_size_; ++i)
                {
                    storage_[i] = std::move(storage_[i + range_size]);
                }

                current_size_ -= range_size;
                return begin() + first_index;
            }

            // ==================== 查找操作 ====================

            // contains - 检查是否包含元素 (C++23 风格)
            constexpr bool contains(T const& value) const
                requires std::equality_comparable<T>
            {
                for (size_type i = 0; i < current_size_; ++i)
                {
                    if (storage_[i] == value)
                    {
                        return true;
                    }
                }
                return false;
            }

            // find - 查找元素
            constexpr iterator find(T const& value)
            {
                for (size_type i = 0; i < current_size_; ++i)
                {
                    if (storage_[i] == value)
                    {
                        return begin() + i;
                    }
                }
                return end();
            }

            constexpr const_iterator find(T const& value) const
            {
                for (size_type i = 0; i < current_size_; ++i)
                {
                    if (storage_[i] == value)
                    {
                        return cbegin() + i;
                    }
                }
                return cend();
            }

            // ==================== 比较运算符 ====================

            // C++23 三向比较运算符
            constexpr auto operator<=>(constexpr_vector const& other) const
                requires std::three_way_comparable<T>
            {
                // 先比较大小
                if (auto cmp = current_size_ <=> other.current_size_; cmp != 0)
                {
                    return cmp;
                }

                // 然后逐元素比较
                for (size_type i = 0; i < current_size_; ++i)
                {
                    if (auto cmp = storage_[i] <=> other.storage_[i]; cmp != 0)
                    {
                        return cmp;
                    }
                }

                return std::strong_ordering::equal;
            }

            constexpr bool operator==(constexpr_vector const& other) const
                requires std::equality_comparable<T>
            {
                if (current_size_ != other.current_size_)
                {
                    return false;
                }
                for (size_type i = 0; i < current_size_; ++i)
                {
                    if (!(storage_[i] == other.storage_[i]))
                    {
                        return false;
                    }
                }
                return true;
            }

            // ==================== 友元函数 ====================

            template<typename F>
            friend constexpr void resize_and_overwrite(constexpr_vector& v, F&& f)
            {
                v.current_size_ = std::forward<F>(f)(v.data(), v.current_size_);
            }
        };

        // ==================== 非成员函数 ====================

        // CTAD 推导指引
        template<typename T, typename... Ts>
        constexpr_vector(T, Ts...) -> constexpr_vector<T, 1 + sizeof...(Ts)>;

        // 从迭代器范围推导
        template<std::input_iterator InputIt>
        constexpr_vector(InputIt, InputIt) -> constexpr_vector<std::iter_value_t<InputIt>, 256>;

        // get 函数 - 编译期索引访问
        template<std::size_t I, typename T, std::size_t N>
        [[msvc::forceinline]]
        constexpr auto get(constexpr_vector<T, N>& v) -> decltype(auto)
        {
            static_assert(I < N, "Index out of bounds");
            return v.template get<I>();
        }

        template<std::size_t I, typename T, std::size_t N>
        [[msvc::forceinline]]
        constexpr auto get(constexpr_vector<T, N> const& v) -> decltype(auto)
        {
            static_assert(I < N, "Index out of bounds");
            return v.template get<I>();
        }

        // swap 重载
        template<typename T, std::size_t N>
        constexpr void swap(constexpr_vector<T, N>& lhs, constexpr_vector<T, N>& rhs) noexcept
        {
            lhs.swap(rhs);
        }


    } // namespace constexpr_

} // namespace shine

// ==================== 标准库特化 ====================

// std::tuple_size 特化
template<typename T, std::size_t N>
struct std::tuple_size<shine::constexpr_::constexpr_vector<T, N>>
    : std::integral_constant<std::size_t, N>
{
};

// std::tuple_element 特化
template<std::size_t I, typename T, std::size_t N>
struct std::tuple_element<I, shine::constexpr_::constexpr_vector<T, N>>
{
    using type = T;
};


// capacity_v 变量模板特化
namespace shine
{
    template<typename T>
    inline constexpr std::size_t ct_capacity_v = 0;

    template<typename T, std::size_t N>
    inline constexpr std::size_t ct_capacity_v<constexpr_::constexpr_vector<T, N>> = N;
}