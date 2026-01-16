#pragma once

#include <array>
#include <concepts>

#include "iterator.h"

namespace shine
{
    namespace constexpr_{
    // 用于反射的编译期数组
    template<typename T, size_t N> class constexpr_vector
    {
        std::array<T,N> storage{};
        size_t          current_size{};

    public:
        using difference_type        = std::ptrdiff_t;
        using reference              = T &;
        using const_reference        = T const &;
        using pointer                = T *;
        using const_pointer          = T const *;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = std::reverse_iterator<iterator>;       // 反向迭代器
        using const_reverse_iterator = std::reverse_iterator<const_iterator>; // 反向迭代器

        constexpr constexpr_vector() = default;

        // 最多 N 个参数，且每个参数都能转换成 T / value_type
        template <std::convertible_to<T>... Ts>
            requires(sizeof...(Ts) <= N)
        constexpr explicit constexpr_vector(Ts const &...args)
            : storage{static_cast<T>(args)...}, current_size{sizeof...(Ts)} {
        }

        constexpr static std::integral_constant<size_t, N> capacity{};

        // 统一返回迭代器的初始地址
        constexpr auto begin() noexcept        -> iterator               { return std::data(storage); }
        constexpr auto begin() const noexcept  -> const_iterator         { return std::data(storage); }

        constexpr auto end() const noexcept    -> const_iterator         { return std::data(storage) + current_size; }
        constexpr auto end() noexcept          -> iterator               { return std::data(storage) + current_size; }

        constexpr auto rbegin() noexcept       -> reverse_iterator       { return end();}
        constexpr auto rbegin() const noexcept -> const_reverse_iterator { return end();}

        constexpr auto rend() noexcept         -> reverse_iterator       { return begin(); }
        constexpr auto rend() const noexcept   -> const_reverse_iterator { return begin(); }


        constexpr auto front() noexcept        -> reference              { return storage[0]; }
        constexpr auto front() const noexcept  -> const_reference        { return storage[0]; }

        constexpr auto back() noexcept         -> reference              { return storage[current_size - 1]; }
        constexpr auto back() const noexcept   -> const_reference        { return storage[current_size - 1]; }

        constexpr auto size() noexcept         -> size_t                 { return current_size; }



        constexpr auto operator[](size_t index) noexcept       -> reference       { return storage[index]; }
        constexpr auto operator[](size_t index) const noexcept -> const_reference { return storage[index]; }

        
        template<size_t Index>
        constexpr reference get() noexcept
        {
            static_assert(Index < N, "Index out of bounds");
            return std::get<Index>(storage);               // std::get 是编译期获取 元素的模板
        }

        template<size_t Index>
        constexpr const_reference get() const noexcept
        {
            static_assert(Index < N, "Index out of bounds");
            return std::get<Index>(storage);               // std::get 是编译期获取 元素的模板
        }

        constexpr bool full() const noexcept    {   return current_size == N;  }

        constexpr bool empty() const noexcept   {   return current_size == 0;  }

        constexpr void clear() {current_size = 0;}

        constexpr reference push_back(T const& value)   {  return storage[current_size++] = value;  }

        constexpr reference push_back(T&& value)        { return storage[current_size++] = std::move(value);  }

        constexpr T pop_back()                          { return storage[--current_size];  }

    private:


        template<typename F>
        friend constexpr void resize_and_overwrite(constexpr_vector& v,F && f){
            v.current_size = std::forward<F>(f)(std::data(v.storage),v.current_size);
        }

        friend constexpr bool operator==(constexpr_vector const& lhs,constexpr_vector const& rhs){
            if(lhs.size() != rhs.size()){
                return false;
            }
            for(size_t i=0;i<lhs.size();++i){
                if(!(lhs[i] == rhs[i])){
                    return false;
                }
            }
            return true;
        }

        friend constexpr bool operator!=(constexpr_vector const& lhs,constexpr_vector const& rhs){
            return !(lhs == rhs);
        }

    };



    // 从初始化参数推导出 constexpr_vector 的类型和容量
    // constexpr_vector v(1, 2, 3); 会推导为 constexpr_vector<int, 3>
    template<typename T,typename... Ts>
    constexpr_vector(T,Ts...)->constexpr_vector<T,1+sizeof...(Ts)>;


    // 非 const 版本的 get 函数：通过编译时索引获取向量中的元素
    // decltype(auto) 用于完美转发返回类型（const/非const）
    template<size_t I,typename T, size_t N>
    decltype(auto) get(constexpr_vector<T,N>& v) {
        return v.template get<I>();
    }

    // const 版本的 get 函数：允许从 const constexpr_vector 中获取元素
    // 返回 const 引用以确保类型安全
    template<size_t I,typename T, size_t N>
    decltype(auto) get(constexpr_vector<T,N> const &v) {
        return v.template get<I>();
    }

}


    // 模板特化：为 constexpr_vector 特化 capacity_v 变量模板
    // 用于编译时获取向量的容量（类似于 std::tuple_size）
    template<typename T, size_t N>
    constexpr size_t ct_capacity_v<constexpr_::constexpr_vector<T, N>> = N;


};
