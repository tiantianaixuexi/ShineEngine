---
name: constexpr-map-skill
description: ShineEngine 编译期映射容器，提供 constexpr_map、constexpr_sorted_map、constexpr_multimap、constexpr_flat_map 等多种编译期键值存储方案
---

# constexpr_map 技能清单

## 描述
`constexpr_map` 是一组编译期映射容器，提供键值对的编译期存储和查找能力。包含四种实现：普通映射（保持插入顺序）、有序映射（自动排序）、多重映射（允许重复键）、扁平映射（缓存友好），适用于不同的编译期元数据存储场景。

## 使用场景
- 当需要在编译期存储类型 ID 到类型信息的映射时
- 当需要编译期字段名到字段信息的查找表时
- 当需要编译期元数据键值对存储时
- 当实现编译期类型注册表时

## 详情

### 1. constexpr_map（普通映射）

**文件位置**: `src/constexpr/constexpr_map.h`

**技术要点**:
- 模板参数 `<Key, Value, N>` 确定键类型、值类型和容量
- 保持插入顺序，支持显式排序
- 线性查找 O(n)，排序后支持二分查找 O(log n)
- 编译期/运行期双模式支持

**核心接口**:

#### 构造与容量
| 接口 | 说明 |
|------|------|
| `constexpr_map()` | 默认构造 |
| `constexpr_map({{k,v}, ...})` | 初始化列表构造 |
| `size()` | 当前元素数 |
| `capacity` | 编译期容量常量 |
| `empty()/full()` | 空/满检查 |
| `available()` | 剩余容量 |

#### 查找操作
| 接口 | 说明 |
|------|------|
| `find(key)` | 线性查找 |
| `binary_find(key)` | 二分查找（自动排序） |
| `contains(key)` | 包含检查 |
| `count(key)` | 键出现次数 |
| `at(key)` | 带边界检查访问 |
| `try_get(key)` | 返回 `std::optional<Value>` |
| `get(key)` | 直接访问（不安全） |
| `operator[]` | 查找或插入默认值 |
| `get_or(key, default)` | 带默认值的访问 |

#### 插入操作
| 接口 | 说明 |
|------|------|
| `insert(key, value)` | 插入键值对 |
| `put(key, value)` | 插入或更新 |
| `emplace(key, args...)` | 就地构造值 |
| `try_insert(key, value)` | 键不存在时才插入 |

#### 删除操作
| 接口 | 说明 |
|------|------|
| `erase(key)` | 删除指定键（O(1) 与末尾交换） |
| `erase_all(key)` | 删除所有匹配键 |
| `erase(pos)` | 按迭代器删除 |
| `pop_back()` | 弹出末尾元素 |
| `clear()` | 清空 |

#### 排序
| 接口 | 说明 |
|------|------|
| `sort()` | 按键排序 |
| `sorted()` | 返回排序后的新映射（编译期） |
| `is_sorted()` | 检查排序状态 |

#### 遍历与过滤
| 接口 | 说明 |
|------|------|
| `for_each(func)` | 遍历所有键值对 |
| `filter(pred)` | 按谓词过滤 |
| `keys()` | 获取所有键的 vector |
| `values()` | 获取所有值的 vector |

#### 编译期查找（模板参数版本）
| 接口 | 说明 |
|------|------|
| `contains_ct<Key>()` | 编译期包含检查 |
| `get_ct<Key>()` | 编译期值获取 |

---

### 2. constexpr_sorted_map（有序映射）

**技术要点**:
- 始终保持按键有序
- 二分查找 O(log n)
- 插入 O(n)（需移动元素保持有序）
- 编译期构造时自动排序

**核心接口**:
| 接口 | 说明 |
|------|------|
| `find(key)` | 二分查找 |
| `contains(key)` | 包含检查 |
| `at(key)` | 带边界检查访问 |
| `operator[]` | 只读访问 |
| `insert(key, value)` | 有序插入 |
| `contains_ct<Key>()` | 编译期包含检查 |
| `get_ct<Key>()` | 编译期值获取 |

---

### 3. constexpr_multimap（多重映射）

**技术要点**:
- 允许重复键
- 支持 `equal_range` 查找所有匹配项
- 线性查找，O(n) 复杂度

**核心接口**:
| 接口 | 说明 |
|------|------|
| `equal_range(key)` | 查找所有匹配键的范围 |
| `count(key)` | 统计键出现次数 |
| `insert(key, value)` | 插入（允许重复） |
| `erase(key)` | 删除所有匹配键 |

---

### 4. constexpr_flat_map（扁平映射）

**技术要点**:
- 键值分开存储，缓存更友好
- 独立的 `keys_` 和 `values_` 数组
- 适合高性能遍历场景
- 支持直接访问底层数组

**核心接口**:
| 接口 | 说明 |
|------|------|
| `find(key)` | 返回 `Value*` 指针 |
| `contains(key)` | 包含检查 |
| `operator[]` | 查找或插入 |
| `insert(key, value)` | 插入 |
| `keys_data()` / `values_data()` | 直接访问底层数组 |
| `keys_size()` | 键数组大小 |

---

### 5. 使用示例

#### 基础用法
```cpp
#include "constexpr/constexpr_map.h"

// 定义容量为 16 的编译期映射
shine::constexpr_::constexpr_map<int, const char*, 16> map;
map.insert(1, "one");
map.insert(2, "two");
map.insert(3, "three");

// 查找
const char* val = map.get(2);  // "two"
bool has_5 = map.contains(5);  // false

// 安全访问
auto opt = map.try_get(1);  // std::optional<const char*>
if (opt) { /* use opt.value() */ }
```

#### 编译期类型注册表
```cpp
struct TypeInfo {
    const char* name;
    size_t size;
    size_t alignment;
};

// 编译期构建类型注册表
template <size_t N>
struct TypeRegistry {
    shine::constexpr_::constexpr_map<uint32_t, TypeInfo, N> types;
    
    consteval auto register_type(uint32_t id, const char* name, size_t size, size_t align) {
        types.insert(id, {name, size, align});
        return *this;
    }
    
    constexpr const TypeInfo* find(uint32_t id) const {
        auto it = types.find(id);
        return it != types.end() ? &it->value : nullptr;
    }
};

// 编译期构建
constexpr auto registry = TypeRegistry<32>{}
    .register_type(1, "int", sizeof(int), alignof(int))
    .register_type(2, "float", sizeof(float), alignof(float))
    .register_type(3, "double", sizeof(double), alignof(double));

// 编译期查找
static_assert(registry.find(1)->size == sizeof(int));
```

#### 编译期字段元数据
```cpp
struct FieldMetadata {
    uint64_t flags;
    uint32_t offset;
};

// 使用哈希值作为键（编译期字符串哈希）
template <size_t N>
struct FieldMetadataStorage {
    shine::constexpr_::constexpr_map<uint64_t, FieldMetadata, N> metadata;
    
    consteval auto add(const char* name, uint64_t flags, uint32_t offset) {
        uint64_t hash = shine::constexpr_::detail::fnv1a_hash<char>::compute(
            name, std::strlen(name));
        metadata.insert(hash, {flags, offset});
        return *this;
    }
    
    constexpr const FieldMetadata* get(const char* name) const {
        uint64_t hash = shine::constexpr_::detail::fnv1a_hash<char>::compute(
            name, std::strlen(name));
        auto it = metadata.find(hash);
        return it != metadata.end() ? &it->value : nullptr;
    }
};

constexpr auto fields = FieldMetadataStorage<16>{}
    .add("id", 1, 0)
    .add("name", 2, 4)
    .add("active", 4, 36);
```

#### 有序映射与二分查找
```cpp
// 编译期有序映射
constexpr shine::constexpr_::constexpr_sorted_map<int, const char*, 8> sorted_map =
    {{{1, "one"}, {2, "two"}, {3, "three"}, {5, "five"}}};

// 二分查找
static_assert(sorted_map.contains(2));  // true
static_assert(sorted_map.at(3) == "three");

// 编译期查找
static_assert(sorted_map.contains_ct<2>());  // true
```

#### 多重映射（多值键）
```cpp
shine::constexpr_::constexpr_multimap<std::string_view, int, 16> tags;
tags.insert("category", 1);
tags.insert("category", 2);
tags.insert("category", 3);
tags.insert("type", 10);

// 查找所有 "category" 值
auto range = tags.equal_range("category");
// range.first -> {category, 1}
// range.second 指向最后一个 category 之后

// 计数
size_t count = tags.count("category");  // 3
```

#### 扁平映射（缓存友好）
```cpp
shine::constexpr_::constexpr_flat_map<uint32_t, float, 64> params;
params.insert("speed"_hash, 100.0f);
params.insert("damage"_hash, 50.0f);
params.insert("health"_hash, 1000.0f);

// 高性能遍历（连续内存）
for (size_t i = 0; i < params.keys_size(); ++i) {
    uint32_t key = params.keys_data()[i];
    float value = params.values_data()[i];
    // process...
}
```

#### 排序与二分查找优化
```cpp
shine::constexpr_::constexpr_map<std::string_view, int, 32> config;
config.insert("z_index", 100);
config.insert("alpha", 255);
config.insert("scale", 1.0f);
config.insert("rotation", 0.0f);

// 显式排序（一次性）
config.sort();

// 后续使用二分查找
auto it = config.binary_find("alpha");
if (it != config.end()) {
    int alpha = it->value;
}
```

---

### 6. 在反射系统中的应用

#### 编译期类型 ID 映射
```cpp
// 类型 ID -> 类型信息映射
template <size_t N>
struct CompileTimeTypeRegistry {
    shine::constexpr_::constexpr_sorted_map<uint32_t, TypeInfo, N> registry;
    
    consteval auto register_type(uint32_t id, TypeInfo info) {
        registry.insert(id, info);
        return *this;
    }
    
    // 编译期查找（用于模板元编程）
    template <uint32_t ID>
    consteval bool has_type() const {
        return registry.contains_ct<ID>();
    }
    
    template <uint32_t ID>
    consteval const TypeInfo& get_type() const {
        return registry.get_ct<ID>();
    }
};

// 使用
constexpr auto type_registry = CompileTimeTypeRegistry<64>{}
    .register_type(GetTypeId<int>(), {"int", sizeof(int), alignof(int)})
    .register_type(GetTypeId<float>(), {"float", sizeof(float), alignof(float)});
```

#### 编译期字段查找表
```cpp
struct FieldDescriptor {
    uint32_t type_id;
    uint32_t offset;
    uint64_t flags;
};

template <typename T, size_t N>
struct FieldTable {
    // 字段名哈希 -> 字段描述符
    shine::constexpr_::constexpr_map<uint64_t, FieldDescriptor, N> fields;
    
    consteval auto add_field(const char* name, uint32_t type_id, uint32_t offset, uint64_t flags) {
        uint64_t hash = "name"_hash;  // 编译期哈希
        fields.insert(hash, {type_id, offset, flags});
        return *this;
    }
    
    constexpr const FieldDescriptor* find_field(const char* name) const {
        uint64_t hash = shine::constexpr_::detail::fnv1a_hash<char>::compute(
            name, std::strlen(name));
        auto it = fields.find(hash);
        return it != fields.end() ? &it->value : nullptr;
    }
};
```

---

## 构建与验证

**编译要求**:
- C++23 标准
- MSVC 19.34+ 或 GCC 13+ / Clang 16+

**验证命令**:
```bash
# 编译测试
Build.bat module constexpr

# 运行测试
Build.bat test
```

**性能指标**:
| 操作 | 复杂度 | 说明 |
|------|--------|------|
| 插入 | O(1) | 末尾插入 |
| 线性查找 | O(n) | 通用查找 |
| 二分查找 | O(log n) | 排序后 |
| 排序 | O(n log n) | 一次性 |
| 内存占用 | sizeof(Key+Value) * N + overhead | 无动态分配 |
