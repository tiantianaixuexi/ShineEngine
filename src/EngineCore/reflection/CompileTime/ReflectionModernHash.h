#pragma once
#include <cstdint>
#include <string_view>
#include "../../../constexpr/constexpr_map.h"
#include "../../../constexpr/constexpr_vector.h"
#include "../../../constexpr/constexpr_str.h"
#include "../constexpr/simple_constexpr_containers.h"

namespace shine::reflection::compile_time {

// 利用现有编译期容器的哈希系统
using HashMap = shine::constexpr_map<uint32_t, std::string_view, 256>;
using HashVector = shine::constexpr_vector<std::string_view, 256>;

// 现代编译期哈希算法
consteval uint32_t FNV1aHash(std::string_view str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash ? hash : 1; // 避免返回0
}

// 类型哈希生成
template<typename T>
consteval uint32_t TypeHash() {
    return FNV1aHash(__FUNCSIG__);
}

// 编译期字符串缓存 - 使用现有constexpr容器
struct StringCache {
private:
    static consteval HashMap BuildCache() {
        HashMap map{};
        // 预填充常用字符串
        map.insert(FNV1aHash("id"), "id");
        map.insert(FNV1aHash("name"), "name");
        map.insert(FNV1aHash("value"), "value");
        map.insert(FNV1aHash("data"), "data");
        map.insert(FNV1aHash("config"), "config");
        map.insert(FNV1aHash("editable"), "editable");
        map.insert(FNV1aHash("readonly"), "readonly");
        map.insert(FNV1aHash("script_readable"), "script_readable");
        map.insert(FNV1aHash("script_writable"), "script_writable");
        return map;
    }
    
    static constexpr HashMap cache = BuildCache();
    
public:
    consteval static std::string_view Find(uint32_t hash) {
        auto it = cache.find(hash);
        if (it != nullptr) {
            return *it;
        }
        return "";
    }
    
    consteval static bool Contains(uint32_t hash) {
        return cache.contains(hash);
    }
    
    consteval static size_t Size() {
        return cache.size();
    }
    
    // 添加新字符串到缓存
    template<size_t NewCapacity = 512>
    consteval static auto AddString(std::string_view str) {
        HashMap new_cache{};
        // 复制现有缓存
        for (const auto& entry : cache) {
            new_cache.insert(entry.first, entry.second);
        }
        // 添加新字符串
        new_cache.insert(FNV1aHash(str), str);
        return new_cache;
    }
};

// 编译期类型名称注册
struct TypeNameRegistry {
private:
    static consteval HashMap BuildTypeNames() {
        HashMap map{};
        // 预注册基本类型
        map.insert(TypeHash<int>(), "int");
        map.insert(TypeHash<unsigned int>(), "unsigned int");
        map.insert(TypeHash<long>(), "long");
        map.insert(TypeHash<unsigned long>(), "unsigned long");
        map.insert(TypeHash<long long>(), "long long");
        map.insert(TypeHash<unsigned long long>(), "unsigned long long");
        map.insert(TypeHash<float>(), "float");
        map.insert(TypeHash<double>(), "double");
        map.insert(TypeHash<long double>(), "long double");
        map.insert(TypeHash<bool>(), "bool");
        map.insert(TypeHash<char>(), "char");
        map.insert(TypeHash<unsigned char>(), "unsigned char");
        map.insert(TypeHash<short>(), "short");
        map.insert(TypeHash<unsigned short>(), "unsigned short");
        map.insert(TypeHash<std::string>(), "std::string");
        map.insert(TypeHash<std::string_view>(), "std::string_view");
        return map;
    }
    
    static constexpr HashMap type_names = BuildTypeNames();
    
public:
    template<typename T>
    consteval static std::string_view GetName() {
        constexpr auto hash = TypeHash<T>();
        if (auto it = type_names.find(hash); it != type_names.end()) {
            return it->second;
        }
        return __FUNCSIG__; // fallback to compiler generated name
    }
    
    consteval static bool Contains(uint32_t type_hash) {
        return type_names.contains(type_hash);
    }
};

// 编译期属性标志
enum class PropertyFlags : uint32_t {
    None = 0,
    Editable = 1 << 0,
    ReadOnly = 1 << 1,
    ScriptReadable = 1 << 2,
    ScriptWritable = 1 << 3,
    Transient = 1 << 4,
    Deprecated = 1 << 5,
    Required = 1 << 6,
    Hidden = 1 << 7
};

// 启用位运算操作
constexpr PropertyFlags operator|(PropertyFlags a, PropertyFlags b) {
    return static_cast<PropertyFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr PropertyFlags operator&(PropertyFlags a, PropertyFlags b) {
    return static_cast<PropertyFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

constexpr PropertyFlags& operator|=(PropertyFlags& a, PropertyFlags b) {
    a = a | b;
    return a;
}

} // namespace shine::reflection::compile_time