# Shine Engine 反射系统文档

> **C++23 / MSVC** | 单头文件入口: `#include "EngineCore/reflection/Reflection.h"`

---

## 目录

1. [架构总览](#1-架构总览)
2. [文件结构](#2-文件结构)
3. [核心类型系统 (ReflectionCore.h)](#3-核心类型系统)
4. [运行时注册与查询 (Reflection.h)](#4-运行时注册与查询)
5. [DSL 注册语法](#5-dsl-注册语法)
6. [TypeBuilder 构建器](#6-typebuilder-构建器)
7. [宏系统](#7-宏系统)
8. [视图层 (Views)](#8-视图层)
9. [错误处理 (ReflectionError.h)](#9-错误处理)
10. [内存管理 (ReflectionMemory)](#10-内存管理)
11. [编辑器集成](#11-编辑器集成)
12. [使用示例](#12-使用示例)
13. [consteval 优化分析](#13-consteval-优化分析)

---

## 1. 架构总览

```
┌─────────────────────────────────────────────────────────────┐
│                      用户代码层                              │
│   REFLECTION_STRUCT / REFLECT_FIELD / REFLECT_METHOD 宏      │
├──────────────┬──────────────────┬───────────────────────────┤
│  DSL 层      │  TypeBuilder     │  Views (Inspector/Script) │
│  FieldDSLNode│  FieldBuilder    │  InspectorView            │
│  MethodDSLNode  MethodBuilder   │  ScriptView / ECSView     │
├──────────────┴──────────────────┴───────────────────────────┤
│                    TypeRegistry (单例)                        │
│              unordered_map<TypeId, TypeInfo>                  │
├─────────────────────────────────────────────────────────────┤
│                    核心数据结构                               │
│         TypeInfo / FieldInfo / MethodInfo                     │
├─────────────────────────────────────────────────────────────┤
│                  编译期基础设施                               │
│     consteval Hash / GetTypeName / GetTypeId                 │
│     EnumFlags / PropertyFlags / FunctionFlags                │
├─────────────────────────────────────────────────────────────┤
│                  内存管理子系统                               │
│   ReflectionMemoryManager / ArenaAllocator / StringPool      │
└─────────────────────────────────────────────────────────────┘
```

**设计哲学**:
- **编译期尽可能多做**: TypeId、TypeName、Hash 均为 `consteval`
- **运行时注册**: 通过 static init lambda 自动注册到全局 `TypeRegistry`
- **类型擦除**: 通过函数指针 (`GetterFn`, `SetterFn`, `InvokeFn`) 实现零开销多态
- **双轨制**: 同时支持运行时反射 (`TypeBuilder`) 和编译期静态检视 (`StaticInspectorBuilder`)

---

## 2. 文件结构

```
src/EngineCore/reflection/
├── Reflection.h             # 公共入口 (TypeRegistry, Views, DSL, TypeBuilder, 宏)
├── ReflectionCore.h         # 核心类型 (Hash, TypeId, Flags, FieldInfo, MethodInfo, TypeInfo)
├── ReflectionError.h        # 错误系统 (std::expected<T, ReflectionError>)
└── Memory/
    ├── ReflectionMemory.h   # 内存管理 (ArenaAllocator, MemoryGuard, StringPool)
    └── ReflectionMemory.cpp # 内存管理实现
```

---

## 3. 核心类型系统

### 3.1 类型标识

```cpp
using TypeId = uint32_t;  // FNV-1a 哈希值作为类型的唯一标识
```

#### Hash 函数 (双重重载)

| 函数签名 | 求值时机 | 用途 |
|----------|---------|------|
| `constexpr Hash(std::string_view)` | 编译期/运行时 | 接受运行时字符串 |
| `consteval Hash(const char (&)[N])` | **仅编译期** | 字符串字面量专用，零运行时开销 |

```cpp
// 运行时 hash (constexpr, 可在编译期求值但不强制)
constexpr TypeId Hash(std::string_view str) noexcept;

// 编译期 hash (consteval, 强制编译期求值)
template <std::size_t N>
consteval TypeId Hash(const char (&str)[N]) noexcept;
```

#### 类型名与类型ID

```cpp
// 从 __FUNCSIG__ 提取干净的类型名 (去除 struct/class/enum 前缀)
template <typename T>
consteval std::string_view GetTypeName() noexcept;

// 编译期生成唯一 TypeId = Hash(GetTypeName<T>())
template <typename T>
consteval TypeId GetTypeId() noexcept;
```

### 3.2 标志位系统

使用自定义 `EnumFlags` 概念约束，支持 `|`, `&`, `^`, `~`, `|=`, `&=`, `^=` 以及 `HasFlag` / `HasAnyFlag` 操作。

#### PropertyFlags (字段属性)

| 标志 | 值 | 说明 |
|------|-----|------|
| `None` | 0 | 无标志 |
| `EditAnywhere` | 1<<0 | 编辑器中可编辑 |
| `ReadOnly` | 1<<1 | 只读 |
| `Transient` | 1<<2 | 不序列化 |
| `ScriptRead` | 1<<3 | 脚本可读 |
| `ScriptWrite` | 1<<4 | 脚本可写 |
| `ScriptReadWrite` | (1<<3)\|(1<<4) | 脚本可读写 |
| `SaveGame` | 1<<5 | 存档时保存 |

#### FunctionFlags (方法属性)

| 标志 | 值 | 说明 |
|------|-----|------|
| `None` | 0 | 无标志 |
| `ScriptCallable` | 1<<0 | 可被脚本调用 |
| `EditorCallable` | 1<<1 | 可在编辑器中调用 |
| `Const` | 1<<2 | const 方法 |
| `Static` | 1<<3 | 静态方法 |

### 3.3 UI Schema (编辑器 UI 描述)

**自动推导机制**: 绝大多数情况下 **无需手动指定** UI 控件。系统根据 C++ 类型自动选择合适的控件：

| C++ 类型 | 自动推导的控件 | 条件 |
|----------|---------------|------|
| `bool` | Checkbox | — |
| `float` | SliderFloat | 有 `.Range()` 元数据 |
| `float` | DragFloat | 无 `.Range()` |
| `int` | SliderInt | 有 `.Range()` 元数据 |
| `int` | DragInt | 无 `.Range()` |
| `double` | DragScalar | — |
| `std::string` | TextInput | — |
| `enum` | Dropdown (Combo) | 已注册枚举值 |
| 已注册结构体 | 递归 Inspector | — |
| 容器类型 | 可展开列表 | — |

只需 `.EditAnywhere()` 即可在编辑器中显示，类似 UE5 的 `UPROPERTY(EditAnywhere)` 用法。

`UI::Schema` 仍可用于 **显式覆盖** 默认控件（如 `FunctionSelector`），但常规类型无需指定：

```cpp
using Schema = std::variant<
    None,              // 默认：按类型自动推导控件
    TextInput,         // 文本输入框 (覆盖)
    NumberInput,       // 数值输入 (覆盖)
    Slider,            // 滑块 (覆盖)
    Dropdown,          // 下拉框 (覆盖)
    Checkbox,          // 复选框 (覆盖)
    ColorPicker,       // 颜色选取器
    FilePicker,        // 文件选取器
    FunctionSelector,  // 函数选择器 (从反射方法中选择)
    VectorEditor,      // 向量编辑器
    MatrixEditor       // 矩阵编辑器
>;
```

### 3.4 Metadata (元数据)

```cpp
using MetadataKey       = TypeId;   // 键 = Hash("KeyName")
using MetadataValue     = std::variant<std::monostate, bool, int, float, double, std::string_view>;
using MetadataContainer = std::vector<std::pair<MetadataKey, MetadataValue>>;
```

常用元数据键:
- `"Category"` — 编辑器中的分类分组
- `"DisplayName"` — 显示名称 (覆盖字段名)
- `"Min"` / `"Max"` — 数值范围
- `"EditCondition"` — 条件显示 (关联 bool 字段名)
- `"BlueprintFunction"` — 标记蓝图函数

### 3.5 FieldInfo (字段描述符)

```cpp
struct FieldInfo {
    TypeId          typeId;         // 字段类型的 Hash ID
    ContainerType   containerType;  // None / Sequence / Associative
    std::size_t     offset;         // 字段在对象中的偏移量
    std::size_t     size;           // sizeof(FieldType)
    std::size_t     alignment;      // alignof(FieldType)
    bool            isPod;          // std::is_trivially_copyable_v

    // 类型擦除的函数指针 (由 TypeBuilder 自动生成)
    GetterFn   getterFn;    // void(*)(const void* inst, void* out, offset, size)
    SetterFn   setterFn;    // void(*)(void* inst, const void* in, offset, size)
    OnChangeFn onChangeFn;  // void(*)(void* inst, const void* oldValue)
    EqualsFn   equalsFn;    // bool(*)(const void* a, const void* b, size)
    CopyFn     copyFn;      // void(*)(void* dst, const void* src, size)

    PropertyFlags     flags;
    UI::Schema        uiSchema;
    std::string_view  name;
    MetadataContainer metadata;

    // 便捷方法
    void Get(const void* inst, void* out) const;
    void Set(void* inst, const void* in) const;
    void OnChange(void* inst, const void* old) const;
    bool Equals(const void* a, const void* b) const;
    void Copy(void* dst, const void* src) const;
    const MetadataValue* GetMeta(MetadataKey key) const;  // 线性查找
    bool HasMeta(MetadataKey key) const;
};
```

### 3.6 MethodInfo (方法描述符)

```cpp
struct MethodInfo {
    std::string_view    name;
    InvokeFn            invokeFn;       // void(*)(void* inst, void** args, void* ret)
    TypeId              returnType;
    std::vector<TypeId> paramTypes;
    FunctionFlags       flags;
    MetadataContainer   metadata;
    const TypeInfo*     owner;          // 所属类型 (反向引用)

    void Invoke(void* inst, void** args, void* ret) const;
    const MetadataValue* GetMeta(MetadataKey key) const;
};
```

### 3.7 TypeInfo (类型描述符)

```cpp
struct TypeInfo {
    TypeId              id;
    std::string_view    name;
    std::size_t         size;
    std::size_t         alignment;
    bool                isEnum;
    bool                isPod;

    std::vector<FieldInfo>  fields;
    std::vector<MethodInfo> methods;

    struct EnumEntry { int64_t value; std::string_view name; };
    std::vector<EnumEntry> enumEntries;

    const FieldInfo*  FindField(std::string_view name) const;   // 线性查找
    const MethodInfo* FindMethod(std::string_view name) const;  // 线性查找
};
```

### 3.8 容器特征 (Container Traits)

```cpp
// 序列容器 (vector, list 等)
struct SequenceTrait {
    TypeId       elementType;
    std::size_t  (*GetSize)(const void*);
    void*        (*GetElement)(void*, std::size_t);
    const void*  (*GetElementConst)(const void*, std::size_t);
    void         (*Resize)(void*, std::size_t);
};

// 关联容器 (map 等)
struct MapTrait {
    TypeId       keyType;
    TypeId       valueType;
    std::size_t  (*GetSize)(const void*);
    void         (*Clear)(void*);
};

// 自动为容器类型生成 SequenceTrait
template <typename List>
struct ListThunks { static const SequenceTrait& GetTrait(); };
```

---

## 4. 运行时注册与查询

### TypeRegistry (全局单例)

```cpp
class TypeRegistry {
public:
    static TypeRegistry& Get();                   // Meyer's singleton
    void Register(TypeInfo info);                  // 注册类型
    const TypeInfo* Find(TypeId id) const;         // 按 ID 查找
    template <typename T>
    const TypeInfo* Find() const;                  // 按模板类型查找
    std::size_t GetRegisteredTypeCount() const;
};
```

**注册流程** (由 `REFLECTION_REGISTER` 宏触发):

```
程序启动
  └─ 静态初始化
       └─ _ReflectInit_##Type lambda 执行
            ├─ 创建 TypeInfo (id, name, size, alignment, isPod, isEnum)
            ├─ 创建 TypeBuilder<T>
            ├─ 调用 _ReflectRegFn_##Type(builder)
            │    ├─ RegisterFieldFromDSL (生成 getterFn, setterFn, equalsFn, copyFn)
            │    └─ RegisterMethodFromDSL (生成 invokeFn, 记录参数类型)
            ├─ 修补 method.owner 指针
            └─ TypeRegistry::Get().Register(std::move(info))
```

---

## 5. DSL 注册语法

### FieldDSLNode

由 `REFLECT_FIELD(Member)` 宏创建，提供链式调用:

```cpp
REFLECT_FIELD(fieldName)
    .EditAnywhere()              // 可在编辑器中编辑 (类似 UE5 UPROPERTY)
    .ReadOnly()                  // 只读
    .ScriptReadWrite()           // 脚本可读写
    .Range(0.0f, 100.0f)         // 设置 Min/Max → 数值类型自动变为 Slider
    .DisplayName("显示名称")      // 设置 DisplayName 元数据
    .Meta("Category", "Audio")   // 自定义元数据
    .FunctionSelect()            // 函数选择器 UI (显式覆盖)
    .UI(UI::ColorPicker{})       // 显式覆盖 UI 控件 (仅特殊场景需要)
    .OnChange<&MyClass::OnChanged>()  // 值变更回调
```

> **注意**: 常规类型无需 `.UI()` — 系统根据 C++ 类型自动推导控件。
> 只有特殊控件（如 `ColorPicker`, `FunctionSelector`）才需要显式指定。

### MethodDSLNode

由 `REFLECT_METHOD(Member)` 宏创建:

```cpp
REFLECT_METHOD(methodName)
    .ScriptCallable()                    // 脚本可调用
    .EditorCallable()                    // 编辑器可调用
    .Meta("BlueprintFunction", true)     // 自定义元数据
```

---

## 6. TypeBuilder 构建器

`TypeBuilder<T>` 是运行时注册的核心，负责:

1. **字段注册** (`RegisterFieldFromDSL`):
   - 自动计算 `offset` (通过 `ComputeOffset`)
   - 自动生成类型安全的 `getterFn` / `setterFn` / `equalsFn` / `copyFn`
   - POD 类型使用 `memcpy`，非 POD 使用赋值运算符

2. **方法注册** (`RegisterMethodFromDSL`):
   - 使用 `MethodTraits` 自动推导返回类型、参数类型
   - 生成类型安全的 `invokeFn` (从 `void**` 参数数组解包)

3. **枚举注册** (`Enums`):
   - 注册枚举值和显示名称的映射

### FieldBuilder 流式 API

```cpp
builder.RegisterFieldFromDSL(node)
    .Range(0.f, 100.f)           // float/int 自动使用 Slider
    .EditAnywhere()              // 编辑器可见
    .ScriptReadWrite()           // 脚本可读写
    .DisplayName("显示名")       // 显示名称
    .Meta("key", value)          // 自定义元数据
    .OnChange<&T::Callback>()    // 值变更回调
    // .UI(UI::ColorPicker{})    // 仅特殊控件需要显式指定
```

---

## 7. 宏系统

| 宏 | 用途 |
|----|------|
| `REFLECTION_STRUCT(Type)` | 声明并定义类型的反射注册函数体 |
| `REFLECTION_REGISTER(Type)` | 生成 static init lambda，在程序启动时自动注册到 TypeRegistry |
| `REFLECT_FIELD(Member)` | 在注册函数体内注册一个字段，返回可链式调用的 DSL 节点 |
| `REFLECT_METHOD(Member)` | 在注册函数体内注册一个方法，返回可链式调用的 DSL 节点 |
| `REFLECT_ENUM(Type)` | 枚举类型反射注册 (声明 + 自动注册 + 函数体) |
| `REFLECTION_REGISTER_LIMITS(Type, LimitsType)` | 带限制类型的注册 (目前等价于 `REFLECTION_REGISTER`) |

### 宏展开示例

```cpp
// 用户代码
REFLECTION_STRUCT(MyStruct) {
    REFLECT_FIELD(myField).EditAnywhere();
    REFLECT_METHOD(myMethod).ScriptCallable();
}
REFLECTION_REGISTER(MyStruct)

// 展开为:
template<typename _RB>
inline void _ReflectRegFn_MyStruct(_RB& builder) {
    builder.RegisterFieldFromDSL(
        shine::reflection::DSL::FieldDSLNode<&MyStruct::myField>("myField")
    ).EditAnywhere();
    builder.RegisterMethodFromDSL(
        shine::reflection::DSL::MakeMethodDSL<&MyStruct::myMethod>("myMethod")
    ).ScriptCallable();
}

// 自动注册
inline auto _ReflectInit_MyStruct = []() {
    TypeInfo _info{};
    _info.id   = GetTypeId<MyStruct>();
    _info.name = "MyStruct";
    // ... size, alignment, isPod, isEnum ...
    TypeBuilder<MyStruct> _builder(_info);
    _ReflectRegFn_MyStruct(_builder);
    for (auto& _m : _info.methods) _m.owner = &_info;
    TypeRegistry::Get().Register(std::move(_info));
    return true;
}();
```

**注意**: `_ReflectRegFn_` 是模板函数，同时被 `TypeBuilder`（运行时注册）和 `StaticInspectorBuilder`（编译期绘制代码生成）调用，实现了**一次定义，双轨使用**。

---

## 8. 视图层

### InspectorView — 编辑器属性面板

```cpp
struct InspectorView : TypeView {
    // 遍历所有字段
    FieldIterator begin() const;
    FieldIterator end() const;

    // 判断字段是否可编辑 (EditAnywhere && !ReadOnly)
    bool IsEditable(const FieldInfo& f) const;

    // 获取 UI Schema
    const UI::Schema& GetUISchema(const FieldInfo& f) const;

    // 条件可见性 (EditCondition 元数据关联 bool 字段)
    bool IsVisible(const FieldInfo& f, const void* instance) const;

    // 获取分类名
    std::string_view GetCategory(const FieldInfo& f) const;

    // 安全写入 (仅在可编辑时)
    void SetValue(void* instance, const FieldInfo& f, const void* value) const;
};
```

### ScriptView — 脚本绑定

```cpp
struct ScriptView : TypeView {
    // 字段读写 (通过 ScriptBridge 做值转换)
    ScriptValue GetField(void* inst, const FieldInfo* f, const ScriptBridge& br) const;
    void SetField(void* inst, const FieldInfo* f, const ScriptValue& v, const ScriptBridge& br) const;

    // 方法调用 (类型安全，通过 void** 参数数组)
    ScriptValue CallMethod(void* inst, const MethodInfo* m,
                           const std::vector<ScriptValue>& args,
                           const ScriptBridge& br) const;
};
```

ScriptView 内部使用 `ScratchBuffer` 优化:
- 小于 64 字节的临时值使用栈分配 (零堆分配)
- 超过 64 字节才使用 `std::make_unique<char[]>`

### ECSView — ECS 组件布局

```cpp
struct ECSView {
    struct ComponentLayout {
        std::size_t size;
        std::size_t alignment;
        const TypeInfo* layoutSource;
    };
    ComponentLayout layout;
};
```

---

## 9. 错误处理

使用 C++23 的 `std::expected<T, ReflectionError>`:

```cpp
enum class ErrorCode {
    Success, TypeNotFound, TypeAlreadyRegistered, InvalidTypeCast,
    TypeMismatch, FieldNotFound, FieldNotAccessible, FieldReadOnly,
    MethodNotFound, MethodNotCallable, ParameterCountMismatch,
    OutOfMemory, BufferTooSmall, SerializationFailed, DeserializationFailed,
    ContainerOperationFailed, InternalError, NotImplemented,
};

template <typename T>
using Result = std::expected<T, ReflectionError>;

// 便捷工厂函数
auto err = MakeError(ErrorCode::FieldNotFound, "message");
auto typeErr = TypeError("MyType");
auto fieldErr = FieldError("myField");
```

`ReflectionError` 自动捕获 `std::source_location` 提供精确的错误定位。

---

## 10. 内存管理

### ReflectionMemoryManager

- **全局单例**，所有反射分配通过此管理器
- 所有分配标记为 `shine::co::MemoryTag::Reflection` (mimalloc 后端)
- **原子统计** (lock-free): 总分配、峰值、分配/释放计数、分类字节数
- **Arena 分配器**: 临时数据使用 bump allocator (O(1) 分配，O(1) 重置)

### MemoryTag (细粒度标签)

| 标签 | 用途 |
|------|------|
| `TypeInfo` | TypeInfo 结构分配 |
| `FieldInfo` | FieldInfo 结构分配 |
| `MethodInfo` | MethodInfo 结构分配 |
| `StringStorage` | 字符串存储 |
| `HashTables` | 哈希表 |
| `TemporaryBuffers` | 临时缓冲 (走 Arena) |
| `CacheData` | 缓存数据 |

### ArenaAllocator

- 64KB 页大小 (可配置)
- `Allocate(size, alignment)` — O(1) bump 分配
- `Reset()` — O(pageCount) 重置 (不释放内存，下次复用)
- `Clear()` — 释放所有页面

### StringMemoryManager

- **字符串驻留池** (interning): 通过 `unordered_set<string_view>` 去重
- **线性 Arena 存储**: 8KB 块大小，字符串拷贝后以 null 结尾
- 避免反射系统中常见的重复字符串分配 (如 "float", "int", 字段名等)

### MemoryGuard<T>

RAII 包装器，自动通过 `ReflectionMemoryManager` 管理生命周期。

---

## 11. 编辑器集成

### 双轨渲染架构

```
REFLECTION_STRUCT(Type) { ... }        ← 一份定义
           │
    ┌──────┴──────┐
    ▼              ▼
TypeBuilder       StaticInspectorBuilder
(运行时注册)      (编译期 ImGui 代码生成)
    │              │
    ▼              ▼
TypeRegistry      直接生成 ImGui 绘制代码
    │              (通过 FieldProxy 析构)
    ▼
InspectorBuilder  ← 运行时 Inspector 绘制
PropertyDrawer    ← 运行时字段控件绘制
```

### StaticInspectorBuilder (编译期路径)

- 利用 `if constexpr` 在编译期分派字段类型
- `FieldProxy` 的**析构函数**中执行 ImGui 绘制 (RAII trick)
- 直接通过成员指针访问字段，零类型擦除开销
- 支持嵌套结构、向量容器、枚举下拉框的递归绘制

### PropertyDrawer (运行时路径)

- 使用 `std::visit` 分派 `UI::Schema` variant
- 通过 `GetterFn` / `SetterFn` 类型擦除访问字段
- **`UI::None` (默认)**: 根据 `field.typeId` 自动推导控件 (bool→Checkbox, float→DragFloat/Slider, int→DragInt/Slider, string→TextInput, enum→Combo, struct→递归)
- 显式 Schema 可覆盖默认行为: ColorPicker / FunctionSelector / NumberInput 等

---

## 12. 使用示例

### 基本结构体反射

```cpp
#include "EngineCore/reflection/Reflection.h"

struct MyComponent {
    float speed = 10.0f;
    int   health = 100;
    bool  isActive = true;
    std::string name = "Player";

    void Reset() { speed = 10.0f; health = 100; }
    void TakeDamage(int amount) { health -= amount; }
};

// 无需手动指定 .UI() — 系统根据 C++ 类型自动推导控件
// float + Range → Slider | bool → Checkbox | string → TextInput
REFLECTION_STRUCT(MyComponent) {
    REFLECT_FIELD(speed)
        .Range(0.0f, 100.0f)       // float + Range → SliderFloat
        .EditAnywhere()
        .DisplayName("速度")
        .Meta("Category", std::string_view{"Movement"});

    REFLECT_FIELD(health)
        .Range(0.0f, 1000.0f)      // int + Range → SliderInt
        .EditAnywhere()
        .DisplayName("生命值")
        .Meta("Category", std::string_view{"Stats"});

    REFLECT_FIELD(isActive)         // bool → Checkbox (自动)
        .EditAnywhere();

    REFLECT_FIELD(name)             // string → TextInput (自动)
        .EditAnywhere();

    REFLECT_METHOD(Reset).EditorCallable();
    REFLECT_METHOD(TakeDamage).ScriptCallable();
}
REFLECTION_REGISTER(MyComponent)
```

### 枚举反射

```cpp
enum class GameMode { Survival, Creative, Adventure };

REFLECT_ENUM(GameMode) {
    builder.Enums({
        {GameMode::Survival,  "生存"},
        {GameMode::Creative,  "创造"},
        {GameMode::Adventure, "冒险"},
    });
}
```

### 运行时查询

```cpp
using namespace shine::reflection;

// 查找类型
const TypeInfo* info = TypeRegistry::Get().Find<MyComponent>();

// 遍历字段
for (const auto& field : info->fields) {
    std::cout << field.name << " (size=" << field.size << ")\n";
}

// 读写字段
MyComponent comp;
float speed;
info->FindField("speed")->Get(&comp, &speed);
speed = 42.0f;
info->FindField("speed")->Set(&comp, &speed);

// 调用方法
info->FindMethod("Reset")->Invoke(&comp, nullptr, nullptr);
```

### 编辑器绘制

```cpp
// 编译期路径 (推荐，零开销)
StaticInspectorBuilder<MyComponent>::Draw(&myComp);

// 运行时路径
InspectorBuilder::DrawInspector(&myComp, TypeRegistry::Get().Find<MyComponent>());
```

---

## 13. consteval 优化分析

### 当前已 consteval 的部分

| 函数 | 说明 |
|------|------|
| `Hash(const char (&str)[N])` | 字符串字面量的编译期哈希 |
| `GetTypeName<T>()` | 编译期类型名提取 |
| `GetTypeId<T>()` | 编译期类型ID生成 |

### 当前 constexpr (非 consteval) 的部分

| 函数 | 能否改为 consteval | 原因 |
|------|-------------------|------|
| `Hash(std::string_view)` | **不能** | 需要在运行时接受动态字符串 |
| `ErrorCodeToString(ErrorCode)` | **不能** | 在 `ReflectionError::ToString()` 中以运行时 ErrorCode 调用 |
| `ComputeOffset(M C::* ptr)` | **不能** | 使用了 `reinterpret_cast` (不允许出现在常量表达式中) |
| `BuildTypeInfo<T>(name)` | **可以改为 constexpr** | 所有操作均为编译期安全 (C++23 constexpr vector) |
| `EnumFlags` 运算符 | 已经是 constexpr | 已最优 |

### 可优化点

#### 优化 1: 预计算常用元数据键 (MetaKeys)

当前 `Meta(std::string_view key, ...)` 方法中 `Hash(key)` 使用 `constexpr` 重载 (因为 key 是 `string_view` 类型)，不保证编译期求值。通过预定义常量可确保为编译期常量：

```cpp
namespace MetaKeys {
    inline constexpr TypeId Category          = Hash("Category");
    inline constexpr TypeId DisplayName       = Hash("DisplayName");
    inline constexpr TypeId Min               = Hash("Min");
    inline constexpr TypeId Max               = Hash("Max");
    inline constexpr TypeId EditCondition     = Hash("EditCondition");
    inline constexpr TypeId BlueprintFunction = Hash("BlueprintFunction");
}
```

#### 优化 2: BuildTypeInfo 标记为 constexpr

```cpp
template <typename T>
constexpr TypeInfo BuildTypeInfo(std::string_view name) { ... }
```

#### 优化 3: 添加 consteval 用户定义字面量

```cpp
consteval TypeId operator""_hash(const char* str, std::size_t len) noexcept {
    // 在编译期计算 FNV-1a 哈希
}
// 使用: field.GetMeta("Category"_hash)
```

#### 优化 4: DSL 的 Meta 方法添加 MetadataKey 重载

```cpp
template <typename V>
auto Meta(MetadataKey key, V&& val) const;  // 直接接受预计算的 hash 值
```

---

## 附录: 数据流图

```
编译期                                          运行时
──────                                          ──────
GetTypeName<T>() ─consteval──► string_view
                                    │
GetTypeId<T>() ─consteval──────► TypeId ────► TypeInfo.id
                                                   │
Hash("Category") ─consteval──► MetadataKey ──► FieldInfo.metadata
                                                   │
REFLECTION_STRUCT ─────────────────────────► TypeBuilder
  REFLECT_FIELD ─ 生成 FieldDSLNode ──────► RegisterFieldFromDSL()
  REFLECT_METHOD ─ 生成 MethodDSLNode ────► RegisterMethodFromDSL()
                                                   │
                                              TypeRegistry.Register()
                                                   │
                                              TypeRegistry.Find()
                                                   │
                            ┌──────────────────────┤
                            ▼                      ▼
                     InspectorView           ScriptView
                     PropertyDrawer          ScriptBridge
                            │                      │
                            ▼                      ▼
                     ImGui 编辑器面板          脚本引擎交互
```
