---
name: constexpr-vector-skill
description: ShineEngine 编译期动态数组容器，提供固定容量的编译期/运行期双模式支持，含算法、排序、查找等完整操作
---

# constexpr_vector 技能清单

## 描述
`constexpr_vector` 是一个固定容量的编译期动态数组容器，支持在编译期和运行期两种模式下操作。它提供了与 `std::vector` 类似的接口，同时保证编译期可计算性，是构建编译期反射系统的核心数据结构。

## 使用场景
- 当需要在编译期收集类型字段列表时
- 当需要编译期确定大小的动态数组时
- 当实现编译期类型注册系统时
- 当需要在编译期进行算法操作（排序、查找、过滤）时

## 详情

### 1. constexpr_vector（编译期动态数组）

**文件位置**: `src/constexpr/constexpr_vector.h`

**技术要点**:
- 模板参数 `<T, N>` 确定元素类型和固定容量
- C++23 特性支持：deducing this、三向比较、概念约束
- MSVC 优化：`[[msvc::forceinline]]` 和 `__assume` 提示
- 编译期/运行期双模式：`if consteval` 分支优化

**核心接口**:

#### 构造与赋值
| 接口 | 说明 |
|------|------|
| `constexpr_vector(Ts...)` | 可变参数构造 |
| `constexpr_vector(InputIt, InputIt)` | 迭代器范围构造 |
| `constexpr_vector(R&&)` | C++20 ranges 构造 |
| `operator=(std::initializer_list<T>)` | 初始化列表赋值 |
| `operator=(R&&)` | range 赋值 |

#### 元素访问
| 接口 | 说明 |
|------|------|
| `operator[]` | 索引访问（带 `__assume` 优化） |
| `at(index)` | 带边界检查访问 |
| `front()/back()` | 首尾元素访问 |
| `data()` | 原始数据指针 |
| `get<Index>()` | 编译期索引访问 |

#### 容量管理
| 接口 | 说明 |
|------|------|
| `size()` | 当前元素数 |
| `capacity()` | 固定容量 N |
| `available()` | 剩余容量 |
| `empty()/full()` | 空/满检查 |

#### 修改操作
| 接口 | 说明 |
|------|------|
| `push_back(T)` | 尾部添加（可能抛异常） |
| `try_push_back(T)` | 不抛异常版本，返回 bool |
| `emplace_back(Args...)` | 就地构造 |
| `try_emplace_back(Args...)` | 不抛异常就地构造 |
| `pop_back()` | 弹出并返回末尾元素 |
| `try_pop_back()` | 返回 `std::optional<T>` |
| `pop_back_discard()` | 弹出不返回值 |
| `clear()` | 清空所有元素 |
| `resize(n)` / `resize(n, val)` | 调整大小 |
| `fill(val)` | 填充所有位置 |

#### 插入删除
| 接口 | 说明 |
|------|------|
| `insert(pos, T)` | 指定位置插入 |
| `insert(pos, count, T)` | 批量插入 |
| `erase(pos)` / `erase(first, last)` | 删除元素 |
| `erase_unordered(index)` | 快速删除（与末尾交换） |

#### 查找操作
| 接口 | 说明 |
|------|------|
| `find(T)` | 线性查找 |
| `contains(T)` | 包含检查 |
| `count(T)` | 计数 |

#### 算法支持
| 接口 | 说明 |
|------|------|
| `sort()` / `sort(comp)` | 快速排序 |
| `stable_sort()` / `stable_sort(comp)` | 稳定排序 |
| `unique()` | 去重（需先排序） |
| `reverse()` | 反转 |
| `transform_inplace(op)` | 原地变换 |
| `filter(pred)` | 过滤元素 |
| `all_of(pred)` / `any_of(pred)` / `none_of(pred)` | 谓词检查 |
| `fold(init, op)` | 折叠/归约 |
| `sum()` | 求和 |
| `min_element()` / `max_element()` | 最值查找 |
| `binary_search(T)` | 二分查找（需已排序） |
| `lower_bound(T)` | 下界查找 |

---

### 2. small_vector（小缓冲优化向量）

**技术要点**:
- 小缓冲优化（SSO）：小数据使用栈空间，大数据自动转堆
- 模板参数 `<T, SmallSize = 16>` 确定小缓冲区大小
- 仅支持移动语义（简化实现）

**核心接口**:
| 接口 | 说明 |
|------|------|
| `push_back(T)` | 添加元素（自动扩容） |
| `emplace_back(Args...)` | 就地构造 |
| `pop_back()` | 弹出末尾 |
| `clear()` | 清空 |
| `size()/capacity()` | 大小/容量查询 |
| `operator[]` | 索引访问 |

---

### 3. 使用示例

#### 基础用法
```cpp
#include "constexpr/constexpr_vector.h"

// 定义容量为 16 的编译期 vector
shine::constexpr_::constexpr_vector<int, 16> vec;
vec.push_back(42);
vec.push_back(100);
vec.push_back(7);

// 编译期确定容量
constexpr size_t cap = decltype(vec)::capacity_v;  // = 16

// 算法操作
vec.sort();  // 排序
bool has_42 = vec.contains(42);  // 查找
int total = vec.sum();  // 求和
```

#### 编译期字段收集（反射场景）
```cpp
struct FieldInfo {
    const char* name;
    uint32_t type_id;
};

// 编译期收集字段
template <typename T>
consteval auto CollectFields() {
    shine::constexpr_::constexpr_vector<FieldInfo, 16> fields;
    
    // 假设通过宏展开添加字段
    fields.push_back({"id", GetTypeId<int>()});
    fields.push_back({"name", GetTypeId<std::string>()});
    fields.push_back({"active", GetTypeId<bool>()});
    
    return fields;
}

constexpr auto fields = CollectFields<MyClass>();
constexpr size_t field_count = fields.size();  // 编译期确定
```

#### 编译期排序与二分查找
```cpp
// 编译期排序的字段列表
constexpr shine::constexpr_::constexpr_vector<const char*, 8> field_names = 
    [](auto init) {
        init.push_back("z_index");
        init.push_back("alpha");
        init.push_back("position");
        init.sort();  // 编译期排序
        return init;
    }(shine::constexpr_::constexpr_vector<const char*, 8>{});

// 编译期二分查找
static_assert(field_names.binary_search("alpha"));  // true
static_assert(!field_names.binary_search("rotation"));  // false
```

#### 过滤与变换
```cpp
shine::constexpr_::constexpr_vector<int, 16> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

// 过滤偶数
nums.filter([](int n) { return n % 2 == 0; });
// nums = {2, 4, 6, 8, 10}

// 原地变换（平方）
nums.transform_inplace([](int n) { return n * n; });
// nums = {4, 16, 36, 64, 100}
```

#### small_vector 用法
```cpp
// 小缓冲优化向量
shine::constexpr_::small_vector<std::string, 8> vec;

// 前 8 个元素使用栈空间
for (int i = 0; i < 8; ++i) {
    vec.push_back(std::to_string(i));
}

// 第 9 个元素开始自动分配堆内存
vec.push_back("heap allocated");
```

---

### 4. 在反射系统中的应用

#### 编译期字段注册
```cpp
template <typename T, size_t N>
struct CompileTimeTypeInfo {
    shine::constexpr_::constexpr_vector<FieldInfo, N> fields;
    
    consteval auto add_field(const char* name, uint32_t type_id) {
        fields.push_back({name, type_id});
        return *this;
    }
    
    consteval auto sort_fields() {
        fields.sort([](const auto& a, const auto& b) {
            return std::string_view(a.name) < std::string_view(b.name);
        });
        return *this;
    }
    
    constexpr const FieldInfo* find_field(const char* name) const {
        for (size_t i = 0; i < fields.size(); ++i) {
            if (std::strcmp(fields[i].name, name) == 0) {
                return &fields[i];
            }
        }
        return nullptr;
    }
};

// 使用
constexpr auto type_info = CompileTimeTypeInfo<MyClass, 16>{}
    .add_field("id", GetTypeId<int>())
    .add_field("name", GetTypeId<std::string>())
    .add_field("active", GetTypeId<bool>())
    .sort_fields();
```

#### 编译期元数据存储
```cpp
struct MetadataEntry {
    uint64_t key_hash;
    shine::constexpr_::constexpr_str<32> value;
};

template <typename T>
struct MetadataStorage {
    shine::constexpr_::constexpr_vector<MetadataEntry, 8> entries;
    
    consteval auto add(const char* key, const char* value) {
        entries.push_back({
            shine::constexpr_::detail::fnv1a_hash<char>::compute(key, strlen(key)),
            shine::constexpr_::constexpr_str<32>(value)
        });
        return *this;
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
- push_back: O(1) 均摊
- 查找: O(n) 线性
- 排序: O(n log n)
- 二分查找: O(log n)
- 内存占用: sizeof(T) * N + size_t（无动态分配）
