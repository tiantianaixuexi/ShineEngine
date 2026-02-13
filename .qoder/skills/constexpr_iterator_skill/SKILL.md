---
name: constexpr-iterator-skill
description: ShineEngine 编译期类型元编程工具集，提供类型列表、值列表、类型特性检测、编译期循环、函数特性萃取等元编程基础设施
---

# constexpr_iterator 技能清单

## 描述
`iterator.h` 是 ShineEngine 的编译期类型元编程核心工具集，提供类型列表操作、值列表计算、类型特性检测、编译期循环、函数特性萃取等功能。它是实现编译期反射系统的基础设施，支持在编译期进行复杂的类型计算和元数据处理。

## 使用场景
- 当需要在编译期操作类型列表（如字段类型列表）时
- 当需要编译期类型特性检测（如检查是否有某个成员）时
- 当需要编译期循环展开时
- 当需要萃取函数签名信息时
- 当实现编译期类型 ID 系统时

## 详情

### 1. 编译期容量特性 (ct_capacity)

**文件位置**: `src/constexpr/iterator.h`

**技术要点**:
- 萃取类型的编译期固定容量
- 支持 `std::array`、原始数组等
- 概念约束 `has_ct_capacity`

**核心接口**:
| 接口 | 说明 |
|------|------|
| `ct_capacity_v<T>` | 获取类型 T 的编译期容量 |
| `ct_capacity<T>` | 容量特性萃取类型 |
| `has_ct_capacity<T>` | 检查类型是否有编译期容量 |

**使用示例**:
```cpp
#include "constexpr/iterator.h"

// 获取 std::array 容量
constexpr size_t cap1 = shine::ct_capacity_v<std::array<int, 16>>;  // 16

// 获取原始数组容量
constexpr size_t cap2 = shine::ct_capacity_v<int[32]>;  // 32

// 概念约束
static_assert(shine::has_ct_capacity<std::array<int, 8>>);
```

---

### 2. 类型列表 (type_list)

**技术要点**:
- 编译期类型容器
- 支持类型级算法：push、pop、at、find、filter 等
- 转换为 `std::tuple`

**核心接口**:
| 接口 | 说明 |
|------|------|
| `type_list<Ts...>` | 类型列表主模板 |
| `size` | 类型数量 |
| `empty` | 是否为空 |
| `push_front<T>` / `push_back<T>` | 添加类型 |
| `pop_front` | 移除第一个类型 |
| `front` / `back` | 首尾类型 |
| `at<I>` | 索引访问 |
| `contains<T>` | 包含检查 |
| `count<T>` | 类型出现次数 |
| `find<T>` | 查找类型索引 |
| `reverse` | 反转列表 |
| `concat<List>` | 连接列表 |
| `remove<T>` | 移除类型 |
| `filter<Pred>` | 按谓词过滤 |
| `as_tuple` | 转为 std::tuple |
| `apply<Template>` | 应用到模板 |

**辅助工具**:
| 接口 | 说明 |
|------|------|
| `type_list_size_v<List>` | 列表大小 |
| `type_list_empty_v<List>` | 是否为空 |
| `type_list_contains_v<List, T>` | 包含检查 |
| `type_list_at_t<List, I>` | 索引访问 |
| `type_list_concat_t<L1, L2>` | 连接两个列表 |

**使用示例**:
```cpp
// 定义类型列表
using MyTypes = shine::type_list<int, float, double, char>;

// 类型级操作
using WithBool = MyTypes::push_back<bool>;  // int, float, double, char, bool
using WithoutFirst = MyTypes::pop_front;    // float, double, char
using Reversed = MyTypes::reverse;          // char, double, float, int

// 检查包含
static_assert(MyTypes::contains<float>);     // true
static_assert(!MyTypes::contains<std::string>); // false

// 查找索引
constexpr size_t idx = MyTypes::find<double>;  // 2

// 过滤（保留算术类型）
using ArithmeticTypes = MyTypes::filter<std::is_arithmetic>;

// 转为 tuple
using AsTuple = MyTypes::as_tuple;  // std::tuple<int, float, double, char>

// 应用到模板
using VariantType = MyTypes::apply<std::variant>;  // std::variant<int, float, double, char>
```

---

### 3. 值列表 (value_list)

**技术要点**:
- 编译期值容器
- 支持数学运算：sum、product、min、max
- 转为 `std::array`

**核心接口**:
| 接口 | 说明 |
|------|------|
| `value_list<Vs...>` | 值列表模板 |
| `size` / `empty` | 大小/空检查 |
| `at<I>` | 索引访问 |
| `sum` | 求和 |
| `product` | 求积 |
| `min` / `max` | 最值 |
| `to_array()` | 转为 std::array |
| `concat<Us...>` | 连接值列表 |

**使用示例**:
```cpp
// 定义值列表
using Sizes = shine::value_list<4, 8, 16, 32, 64>;

// 数学运算
constexpr auto total = Sizes::sum;      // 124
constexpr auto product = Sizes::product; // 4*8*16*32*64
constexpr auto min = Sizes::min;        // 4
constexpr auto max = Sizes::max;        // 64

// 索引访问
constexpr int size_at_2 = Sizes::at<2>;  // 16

// 转为数组
constexpr auto arr = Sizes::to_array();  // std::array<int, 5>{4, 8, 16, 32, 64}

// 连接
using MoreSizes = Sizes::concat<128, 256>;  // 4, 8, 16, 32, 64, 128, 256
```

---

### 4. 类型哈希与 ID

**核心接口**:
| 接口 | 说明 |
|------|------|
| `type_hash_v<T>` | 类型哈希值（FNV-1a） |
| `type_hash<T>` | 哈希特性类型 |
| `type_id_v<T>` | 类型 ID（当前等于哈希） |
| `same_type_v<T, U>` | 类型相同检查 |

**使用示例**:
```cpp
// 类型哈希（编译期）
constexpr uint64_t int_hash = shine::type_hash_v<int>;
constexpr uint64_t float_hash = shine::type_hash_v<float>;

// 类型 ID
constexpr uint64_t my_type_id = shine::type_id_v<MyClass>;

// 类型比较
static_assert(shine::same_type_v<int, int>);      // true
static_assert(!shine::same_type_v<int, float>);   // false
```

---

### 5. 概念约束 (Concepts)

**核心概念**:
| 概念 | 说明 |
|------|------|
| `serializable<T>` | 可序列化类型 |
| `reflectable<T>` | 可反射类型（类或枚举） |
| `default_constructible<T>` | 默认可构造 |
| `copyable<T>` | 可复制 |
| `movable<T>` | 可移动 |

**类型检测**:
| 接口 | 说明 |
|------|------|
| `is_pointer_v<T>` | 指针类型 |
| `is_reference_v<T>` | 引用类型 |
| `is_unique_ptr_v<T>` | unique_ptr |
| `is_shared_ptr_v<T>` | shared_ptr |
| `is_smart_ptr_v<T>` | 智能指针 |
| `is_function_ptr_v<T>` | 函数指针 |
| `is_member_function_ptr_v<T>` | 成员函数指针 |
| `is_template_v<T>` | 模板实例 |

**使用示例**:
```cpp
// 概念约束
template <shine::reflectable T>
struct ReflectionInfo {
    // 只接受类或枚举类型
};

// 类型检测
static_assert(shine::is_pointer_v<int*>);
static_assert(shine::is_smart_ptr_v<std::shared_ptr<int>>);
static_assert(shine::is_template_v<std::vector<int>>);
```

---

### 6. 成员检测 (SHINE_HAS_MEMBER)

**技术要点**:
- 宏生成成员检测模板
- 检测类型是否有指定成员

**生成的检测器**:
| 检测器 | 检测成员 |
|--------|----------|
| `has_size_v<T>` | size() |
| `has_data_v<T>` | data() |
| `has_begin_v<T>` | begin() |
| `has_end_v<T>` | end() |
| `has_push_back_v<T>` | push_back() |
| `has_emplace_back_v<T>` | emplace_back() |
| `has_clear_v<T>` | clear() |
| `has_resize_v<T>` | resize() |
| `has_reserve_v<T>` | reserve() |
| `has_insert_v<T>` | insert() |
| `has_erase_v<T>` | erase() |
| `has_find_v<T>` | find() |
| `has_contains_v<T>` | contains() |

**使用示例**:
```cpp
// 检测容器接口
static_assert(shine::has_push_back_v<std::vector<int>>);   // true
static_assert(shine::has_find_v<std::map<int, int>>);      // true
static_assert(!shine::has_contains_v<std::vector<int>>);   // false (C++20 前)

// 用于模板约束
template <typename T>
    requires shine::has_push_back_v<T>
void add_element(T& container, auto&& value) {
    container.push_back(std::forward<decltype(value)>(value));
}
```

---

### 7. 函数特性萃取

**核心接口**:
| 接口 | 说明 |
|------|------|
| `function_result_t<F>` | 函数返回类型 |
| `function_args_t<F>` | 函数参数类型列表 |
| `function_arity_v<F>` | 函数参数数量 |

**使用示例**:
```cpp
// 函数签名
int foo(double, char);

// 萃取返回类型
using Result = shine::function_result_t<decltype(foo)>;  // int

// 萃取参数列表
using Args = shine::function_args_t<decltype(foo)>;  // type_list<double, char>

// 参数数量
constexpr size_t arity = shine::function_arity_v<decltype(foo)>;  // 2

// 获取第 N 个参数类型
using FirstArg = Args::at<0>;  // double
```

---

### 8. 模板特性萃取

**核心接口**:
| 接口 | 说明 |
|------|------|
| `is_template_v<T>` | 是否是模板实例 |
| `template_args_t<T>` | 模板参数列表 |
| `is_base_of_v<Base, Derived>` | 基类检查 |
| `is_derived_from_v<Derived, Base>` | 派生类检查 |

**使用示例**:
```cpp
// 模板检测
static_assert(shine::is_template_v<std::vector<int>>);

// 萃取模板参数
using VecArgs = shine::template_args_t<std::vector<int>>;  // type_list<int>
using MapArgs = shine::template_args_t<std::map<int, std::string>>;  // type_list<int, std::string>

// 基类检查
static_assert(shine::is_base_of_v<Base, Derived>);
```

---

### 9. 编译期条件与循环

**核心接口**:
| 接口 | 说明 |
|------|------|
| `conditional_t<Condition, T, F>` | 条件选择 |
| `ct_switch_t<Value, Default, Cases...>` | 编译期 switch |
| `ct_for<Start, End, Step>(func)` | 编译期 for 循环 |

**使用示例**:
```cpp
// 条件类型选择
using Selected = shine::conditional_t<sizeof(int) == 4, int64_t, int32_t>;

// 编译期 switch
using Type = shine::ct_switch_t<2, void, 
    0, int,      // case 0: int
    1, float,    // case 1: float
    2, double    // case 2: double
>;  // double

// 编译期循环（展开）
shine::ct_for<0, 5>([](auto I) {
    // I 是 std::integral_constant<size_t, N>
    // 这里会被展开 5 次，I 分别为 0, 1, 2, 3, 4
    constexpr size_t index = I::value;
});
```

---

### 10. 辅助工具

**类型包装器**:
| 接口 | 说明 |
|------|------|
| `type_wrapper<T>` | 类型包装器 |
| `type_w<T>` | 类型包装器实例 |
| `value_wrapper<V>` | 值包装器 |
| `clean_type<T>` | 移除 cvref |

**编译期断言**:
| 接口 | 说明 |
|------|------|
| `ct_assert<Condition, Message>` | 编译期断言 |

**类型标签**:
| 接口 | 说明 |
|------|------|
| `tagged_type<T, Tag>` | 带标签的类型 |
| `get_tag_v<T>` | 获取类型标签 |

**使用示例**:
```cpp
// 类型包装器
auto int_wrapper = shine::type_w<int>;
using WrappedType = decltype(int_wrapper)::type;  // int

// 值包装器
using Value42 = shine::value_wrapper<42>;
constexpr int val = Value42::value;  // 42

// 类型标签
using TaggedInt = shine::tagged_type<int, 100>;
constexpr auto tag = shine::get_tag_v<TaggedInt>;  // 100
```

---

### 11. 在反射系统中的应用

#### 编译期字段列表
```cpp
// 字段类型列表
template <typename T>
struct FieldList {
    using types = shine::type_list<>;  // 特化时填充
};

// 为特定类型特化
template <>
struct FieldList<MyClass> {
    using types = shine::type_list<int, std::string, bool>;
    using names = shine::value_list<
        "id"_hash,
        "name"_hash,
        "active"_hash
    >;
};

// 使用
using MyFields = FieldList<MyClass>::types;
constexpr size_t field_count = MyFields::size;  // 3

// 遍历字段类型
shine::ct_for<0, MyFields::size>([](auto I) {
    using FieldType = MyFields::at<I::value>;
    // 处理每个字段类型...
});
```

#### 编译期类型 ID 生成
```cpp
// 类型注册表
template <typename... Ts>
struct TypeRegistry {
    using types = shine::type_list<Ts...>;
    
    template <typename T>
    static constexpr size_t index_of() {
        return types::template find<T>;
    }
    
    template <typename T>
    static constexpr bool contains() {
        return types::template contains<T>;
    }
};

// 使用
using MyRegistry = TypeRegistry<int, float, double, std::string>;
constexpr size_t float_idx = MyRegistry::index_of<float>();  // 1
static_assert(MyRegistry::contains<std::string>());
```

#### 函数反射
```cpp
template <typename Func>
struct FunctionReflection {
    using return_type = shine::function_result_t<Func>;
    using arg_types = shine::function_args_t<Func>;
    static constexpr size_t arity = shine::function_arity_v<Func>;
    
    // 参数类型名称列表
    static constexpr auto arg_type_names = [] {
        std::array<const char*, arity> names{};
        shine::ct_for<0, arity>([&](auto I) {
            using ArgType = arg_types::at<I::value>;
            names[I::value] = typeid(ArgType).name();
        });
        return names;
    }();
};

// 使用
using FooInfo = FunctionReflection<decltype(&MyClass::foo)>;
static_assert(FooInfo::arity == 2);
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
- 所有操作均为编译期计算，零运行时开销
- 类型列表操作：O(n) 复杂度
- 类型查找：O(n) 线性查找
