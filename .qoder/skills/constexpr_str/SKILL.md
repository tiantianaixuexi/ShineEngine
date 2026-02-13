---
name: constexpr-skill
description: ShineEngine 编译期容器与字符串操作工具集，提供 constexpr_vector、constexpr_str 等编译期数据结构
---

# constexpr 工具集技能清单

## 描述
本文档描述 ShineEngine 中用于编译期计算的容器和字符串工具，包括 constexpr_vector（编译期动态数组）和 constexpr_str（编译期字符串）。这些工具支持在编译期完成类型信息构建、字符串处理和元数据操作，是实现零开销反射的基础。

## 使用场景
- 当需要在编译期构建类型信息（TypeInfo）时
- 当需要在编译期处理字符串（类型名、字段名）时
- 当需要编译期容器存储字段列表、方法列表时
- 当实现编译期反射注册系统时

## 详情

### 1. constexpr_vector（编译期动态数组）

**文件位置**: `src/constexpr/constexpr_vector.h`

**技术要点**: 
- 固定容量模板参数 `<T, N>`，编译期确定最大容量
- 支持 C++23 特性：deducing this、三向比较运算符
- MSVC 优化：`[[msvc::forceinline]]` 和 `__assume` 提示

**核心接口**:
| 接口 | 说明 |
|------|------|
| `push_back(T)` | 添加元素（编译期/运行期均可） |
| `emplace_back(Args...)` | 就地构造，编译期使用 `std::construct_at` |
| `operator[]` | 索引访问，带 `__assume` 边界优化 |
| `size()/capacity()` | 返回当前大小/固定容量 |
| `find(T)` | 线性查找 |
| `contains(T)` | 包含检查 |

**使用示例**:
```cpp
#include "constexpr/constexpr_vector.h"

// 定义容量为 16 的编译期 vector
shine::constexpr_::constexpr_vector<int, 16> vec;
vec.push_back(42);
vec.push_back(100);

// 编译期确定大小
constexpr size_t cap = decltype(vec)::capacity_v;  // = 16
```

**验证标准**: 
- 编译期操作成功率 100%
- 运行时性能与原生数组相当

---

### 2. constexpr_str（编译期字符串）

**文件位置**: `src/constexpr/constexpr_str.h`

**技术要点**:
- 模板参数为字符串长度 `<N>`（含 null 终止符）
- 支持编译期哈希（FNV-1a 算法）
- 支持字符串拼接、子串、修剪等操作
- 与 `std::string_view` 无缝转换

**核心接口**:
| 接口 | 说明 |
|------|------|
| `hash()` | 编译期计算 FNV-1a 哈希值 |
| `runtime_hash()` | 运行期计算哈希 |
| `find(char/str)` | 查找字符/子串 |
| `starts_with/ends_with` | 前缀/后缀检查 |
| `substr<Pos, Count>()` | 编译期子串提取 |
| `trim/trim_left/trim_right` | 空白字符修剪 |
| `to_lower/to_upper` | 大小写转换 |

**字面量操作符**:
| 操作符 | 说明 |
|--------|------|
| `"text"_cts` | 创建 constexpr_str |
| `"text"_ctst` | 创建 cts_t 类型包装器 |
| `"text"_hash` | 直接获取编译期哈希值 |

**使用示例**:
```cpp
#include "constexpr/constexpr_str.h"
using namespace shine::constexpr_::literals;

// 字面量创建
constexpr auto str = "Hello"_cts;
constexpr auto hash = "Category"_hash;  // 直接得哈希值

// 字符串操作
constexpr auto upper = "hello"_cts.to_upper();     // "HELLO"
constexpr auto sub = "HelloWorld"_cts.substr<0, 5>(); // "Hello"
constexpr auto trimmed = "  text  "_cts.trim();    // "text"

// 类型名提取（编译期）
constexpr auto type_name = shine::constexpr_::type_name<int>();

// 哈希比较（用于编译期 switch）
constexpr uint64_t h1 = "EditAnywhere"_hash;
constexpr uint64_t h2 = "ReadOnly"_hash;
```

**验证标准**:
- 编译期字符串操作成功率 100%
- 哈希冲突率 < 0.001%

---

### 3. cts_t（编译期字符串类型包装器）

**技术要点**:
- 将 constexpr_str 作为类型参数包装
- 支持类型级别的字符串操作
- 用于模板元编程和编译期多态

**使用示例**:
```cpp
using namespace shine::constexpr_::literals;

// 类型级别的字符串
template <constexpr_str S>
struct NamedField {
    constexpr static auto name = S;
    constexpr static uint64_t name_hash = S.hash();
};

// 使用
using NameField = NamedField<"name">;
using AgeField = NamedField<"age">;
```

---

### 4. 工具函数

**整数转字符串**:
```cpp
constexpr auto num_str = shine::constexpr_::format_int<42>();  // "42"
```

**类型名提取**:
```cpp
constexpr auto name = shine::constexpr_::type_name<MyClass>();
// MSVC: "MyClass"
```

**字符串分割**:
```cpp
constexpr auto str = "hello,world"_cts;
constexpr auto [first, second] = shine::constexpr_::split<str, ','>();
// first = "hello", second = "world"
```

---

### 5. 在反射系统中的应用

**编译期字段名哈希**:
```cpp
struct FieldInfo {
    constexpr static uint64_t name_hash = 0;
};

template <constexpr_str Name>
struct ConstexprField : FieldInfo {
    constexpr static uint64_t name_hash = Name.hash();
    constexpr static auto name = Name;
};

// 使用
using IDField = ConstexprField<"id">;
static_assert(IDField::name_hash == "id"_hash);
```

**编译期类型注册**:
```cpp
template <typename T, constexpr_str Name>
struct CompileTimeTypeInfo {
    constexpr static auto type_name = Name;
    constexpr static uint32_t type_id = static_cast<uint32_t>(Name.hash());
    shine::constexpr_::constexpr_vector<FieldInfo, 16> fields;
    
    constexpr auto add_field(const FieldInfo& field) {
        fields.push_back(field);
        return *this;
    }
};
```

---

## 构建与验证

**编译要求**:
- C++23 标准
- MSVC 19.34+ 或 GCC 13+ / Clang 16+


**性能指标**:
- 编译期字符串哈希：O(n)，n 为字符串长度
- constexpr_vector 操作：O(1) push/pop，O(n) 查找
- 内存占用：编译期确定，无运行时分配
