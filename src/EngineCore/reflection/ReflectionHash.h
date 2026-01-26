#pragma once


#include <string_view>

namespace shine::reflection {

using TypeId = uint32_t;

constexpr uint32_t Hash(std::string_view str) {
    uint32_t hash = 2166136261u;
    for (const char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

template <typename T>
consteval std::string_view GetTypeName() {
    std::string_view name  = __FUNCSIG__;
    size_t           start = name.find("GetTypeName<") + 12;
    size_t           end   = name.find_last_of('>');
    auto             sub   = name.substr(start, end - start);
    if (sub.starts_with("struct "))
        return sub.substr(7);
    if (sub.starts_with("class "))
        return sub.substr(6);
    return sub;
}

template <typename T>
consteval TypeId GetTypeId() {
    return Hash(GetTypeName<T>());
}
} // namespace Shine::Reflection
