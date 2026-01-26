#pragma once
#include <list>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <vector>


template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

// vector
template <typename T>
inline constexpr bool IsVectorV = false;

template <typename T, typename A>
inline constexpr bool IsVectorV<std::vector<T, A>> = true;

template <typename T>
concept IsVector = IsVectorV<remove_cvref_t<T>>;

// list
template <typename T>
inline constexpr bool IsListV = false;

template <typename T, typename A>
inline constexpr bool IsListV<std::list<T, A>> = true;

template <typename T>
concept IsList = IsListV<remove_cvref_t<T>>;

// map
template <typename T>
inline constexpr bool IsMapV = false;

template <typename K, typename V>
inline constexpr bool IsMapV<std::map<K, V>> = true;

template<typename K>
concept IsMap = IsMapV<remove_cvref_t<K>>;


// unordered_map
template <typename T>
inline constexpr bool IsUnorderedMapV = false;

template <typename K, typename V>
inline constexpr bool IsUnorderedMapV<
    std::unordered_map<K, V>> = true;

template <typename T>
concept IsUnorderedMap = IsUnorderedMapV<remove_cvref_t<T>>;
