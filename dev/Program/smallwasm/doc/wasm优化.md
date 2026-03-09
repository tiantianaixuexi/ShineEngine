# 通过直接内存访问消除 WebAssembly 中的函数调用开销

本文档描述了 smallwasm 项目如何通过使用直接内存访问和内联数学序列（使用 WebAssembly `f32.store` / `f32.load` 及相关模式）来替代结构化函数调用，从而减少运行时开销。

---

## 目录

1. [问题：WASM 中的函数调用开销](#1-问题wasm-中的函数调用开销)
2. [技术 1：强制内联热路径函数](#2-技术-1强制内联热路径函数)
3. [技术 2：直接内存写入替代循环+索引](#3-技术-2直接内存写入替代循环索引)
4. [技术 3：内联 SVector 操作](#4-技术-3内联-svector-操作)
5. [技术 4：内联变换数学运算 (worldXYZ)](#5-技术-4内联变换数学运算-worldxyz)
6. [技术 5：内联顶点写入辅助函数](#6-技术-5内联顶点写入辅助函数)
7. [WASM 指令映射总结](#7-wasm-指令映射总结)
8. [相关源文件](#8-相关源文件)

---

## 1. 问题：WASM 中的函数调用开销

在 WebAssembly 中，每个 `call` 指令都有可测量的成本：

- 将返回地址和参数压入栈
- 跳转到被调用函数
- 执行被调用函数体
- 弹出栈并返回

当一个函数在**每帧中被调用数千次**时（例如 `CommandBuffer::Pass::push`、`SVector::push_back`、顶点写入），这种开销就会占据主导地位。内联消除了 `call` 指令，将逻辑转换为调用点处的一系列直接的加载、存储和算术运算。

### WASM 调用成本示例

```wat
;; 每次调用都会增加开销：参数传递、栈设置、返回
(call $some_push_function
  (local.get $op)
  (local.get $a)
  (local.get $b)
  ...
)
```

---

## 2. 技术 1：强制内联热路径函数

### 之前（调用繁重，优化大小）

```cpp
// wasm_command_buffer.h - NOINLINE 版本（减少二进制大小但增加调用开销）
__attribute__((noinline)) void push(int op, int a, int b, int c, int d, int e, int f, int g) {
    int* ptr = &m_data[m_count];
    ptr[0] = op; ptr[1] = a; ptr[2] = b; ptr[3] = c;
    ptr[4] = d; ptr[5] = e; ptr[6] = f; ptr[7] = g;
    m_count += 8;
}
```

每次 `pass.push(...)` 在 WASM 二进制文件中都会变成一个真实的 `call`。

### 之后（内联，优化速度）

```cpp
// wasm_command_buffer.h - 实际实现
#define SHINE_FORCE_INLINE inline __attribute__((always_inline))

SHINE_FORCE_INLINE void push(int op, int a, int b, int c, int d, int e, int f, int g) {
    if (write + 8 > end) {
        if (owner) owner->note_overflow();
        return;
    }
    owner->update_stats(op, c, d);
    int* p = write;
    p[0] = op;
    p[1] = a;
    p[2] = b;
    p[3] = c;
    p[4] = d;
    p[5] = e;
    p[6] = f;
    p[7] = g;
    write = p + 8;
    ++count;
}
```

使用 `SHINE_FORCE_INLINE` (`__attribute__((always_inline))`)，编译器会在每个调用点直接内联发出存储序列。生成的 WASM 类似于：

```wat
;; 内联的 push - 没有调用指令
(local.get $base_ptr)
(i32.const 0)
(i32.add)
(local.get $op)
(i32.store offset=0)
(local.get $base_ptr)
(i32.const 4)
(i32.add)
(local.get $a)
(i32.store offset=0)
;; ... 更多 b, c, d, e, f, g 的 i32.store
```

没有 `call`/`return`；只有 `i32.store` 和地址运算。

### 便捷变体 (push2, push3, push4, push5)

```cpp
SHINE_FORCE_INLINE void push2(int op, int a) { push(op, a, 0, 0, 0, 0, 0, 0); }
SHINE_FORCE_INLINE void push3(int op, int a, int b) { push(op, a, b, 0, 0, 0, 0, 0); }
SHINE_FORCE_INLINE void push4(int op, int a, int b, int c) { push(op, a, b, c, 0, 0, 0, 0); }
SHINE_FORCE_INLINE void push5(int op, int a, int b, int c, int d) { push(op, a, b, c, d, 0, 0, 0); }
```

当需要较少参数时，这些变体减少了参数传递的开销。

---

## 3. 技术 2：直接内存写入替代循环+索引

### 之前（循环 + 除法/取模）

```cpp
for (int i = 0; i < vertex_count; ++i) {
    int col = i % grid;   // 取模在 WASM 中开销大
    int row = i / grid;   // 除法在 WASM 中开销大
    px[i] = x0 + col * step_x;
    py[i] = y0 + row * step_y;
}
```

这会生成：

- 循环计数器更新
- `i % grid` 和 `i / grid`（在 WASM 中开销大）
- 带索引的加载/存储

### 之后（嵌套循环 + 指针递增，无除法）

```cpp
float* outp = buf.data();
int remaining = count;

for (int gy = 0; gy < grid && remaining > 0; ++gy) {
    float gy_f = (float)gy;
    float cy_base_row = base_pos_off + gy_f * cell;
    const float cx_sin_offset = sin(t_base + gy_f * 0.1f) * 0.05f;
    g = gy_f * inv_grid;

    int row_limit = remaining;
    if (row_limit > grid) row_limit = grid;

    float cx_linear = cx_linear_base;
    float cos_arg = cos_arg_base;
    float r_val = r_val_base;

    for (int gx = 0; gx < row_limit; ++gx) {
        cx = cx_linear + cx_sin_offset;
        cy = cy_base_row + cos(cos_arg) * 0.05f;
        r = r_val;

        float px0 = cx * sx, px1 = (cx - size) * sx, px2 = (cx + size) * sx;
        float py0 = cy + size, py1 = cy - size, py2 = cy - size;

        *outp++ = px0; *outp++ = py0; *outp++ = r; *outp++ = g; *outp++ = b;
        *outp++ = px1; *outp++ = py1; *outp++ = r; *outp++ = g; *outp++ = b;
        *outp++ = px2; *outp++ = py2; *outp++ = r; *outp++ = g; *outp++ = b;

        cx_linear += cell;
        r_val += inv_grid;
        cos_arg += 0.1f;
    }

    remaining -= row_limit;
}
```

这映射为连续的 `f32.store` 配合递增的指针，没有除法/取模，每个顶点的指令数更少。

### 实际的 WASM 输出（摘自 output.gl.wat）

```wat
;; 来自 update_vertices 的直接 f32.store 序列
(f32.store
  (local.get $9)
  (local.get $1))
(f32.store offset=4
  (local.get $9)
  (local.get $2))
(f32.store offset=8
  (local.get $9)
  (local.get $5))
(f32.store offset=12
  (local.get $9)
  (local.get $6))
(f32.store offset=16
  (local.get $9)
  (local.get $7))
;; ... 每个三角形 15 个浮点数，以此类推
```

---

## 4. 技术 3：内联 SVector 操作

### 之前（外部链接，类型擦除）

```cpp
// wasm_runtime.cpp - 节省大小但增加调用开销
void svector_push_void(void* vec, const void* value, size_t elem_size) {
    SVectorBase* v = (SVectorBase*)vec;
    if (v->m_size >= v->m_cap) grow(...);
    memcpy((char*)v->m_data + v->m_size * elem_size, value, elem_size);
    v->m_size++;
}
```

每次 `vec.push_back(x)` 都会变成对 `svector_push_void` 的 `call`。

### 之后（头文件中的内联模板）

```cpp
// SVector.h - 头文件中内联
inline void push_back(const T& v) noexcept {
    const unsigned int n = length + 1u;
    if (unlikely(n > cap)) _grow_to(n);
    static_cast<T*>(pointer)[length] = v;  // 单次存储，无调用
    length = n;
}
```

内联后，这在调用点会变成一个边界检查加上单个 `f32.store` 或 `i32.store`，没有函数调用。

### 增长路径的委派

增长路径（`_grow_to` → `svector_grow_impl`）保持外部链接，以保持热路径代码小巧，但常见情况（容量足够）是完全内联的。

---

## 5. 技术 4：内联变换数学运算 (worldXYZ)

### 之前（noinline）

```cpp
__attribute__((noinline)) void worldXYZ(float& outX, float& outY, float& outZ) const {
    // 递归父节点遍历，矩阵乘法
}
```

每次 `worldXYZ` 调用在 WASM 中都是一个真实的 `call`。

### 之后（内联）

```cpp
// transform.h - 头文件中内联
inline void worldXYZ(float& outX, float& outY, float& outZ) const noexcept {
    Node* p = node ? node->parent : nullptr;
    float parentX = 0.0f, parentY = 0.0f, parentZ = 0.0f;
    if (p) {
        Transform* pt = p->getComponent<Transform>();
        if (pt) pt->worldXYZ(parentX, parentY, parentZ);
    }
    // ... 脏检查，计算 m_worldX/Y/Z ...
    outX = m_worldX;
    outY = m_worldY;
    outZ = m_worldZ;
}
```

编译器可以将变换数学运算直接融合到 `SpriteRenderer::onRender` 中，生成一系列 `f32.load`、`f32.mul`、`f32.add`、`f32.store`，没有函数调用。

---

## 6. 技术 5：内联顶点写入辅助函数

### 实现

```cpp
// renderer_2d.cpp
static inline void writeQuadVtxCol(float* d, float cx, float cy, float w, float h, float r, float g, float b) {
    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;
    d[0] = x1; d[1] = y1; d[2] = r; d[3] = g; d[4] = b;
    d[5] = x2; d[6] = y1; d[7] = r; d[8] = g; d[9] = b;
    d[10]= x1; d[11]= y2; d[12]= r; d[13]= g; d[14]= b;
    d[15]= x1; d[16]= y2; d[17]= r; d[18]= g; d[19]= b;
    d[20]= x2; d[21]= y1; d[22]= r; d[23]= g; d[24]= b;
    d[25]= x2; d[26]= y2; d[27]= r; d[28]= g; d[29]= b;
}

static inline void writeQuadVtxUV(float* d, float cx, float cy, float w, float h) {
    float x1 = cx - w * 0.5f;
    float y1 = cy - h * 0.5f;
    float x2 = cx + w * 0.5f;
    float y2 = cy + h * 0.5f;
    d[0] = x1; d[1] = y1; d[2] = 0.0f; d[3] = 0.0f; d[4] = 0.0f;
    d[5] = x2; d[6] = y1; d[7] = 1.0f; d[8] = 0.0f; d[9] = 0.0f;
    d[10]= x1; d[11]= y2; d[12]= 0.0f; d[13]= 1.0f; d[14]= 0.0f;
    d[15]= x1; d[16]= y2; d[17]= 0.0f; d[18]= 1.0f; d[19]= 0.0f;
    d[20]= x2; d[21]= y1; d[22]= 1.0f; d[23]= 0.0f; d[24]= 0.0f;
    d[25]= x2; d[26]= y2; d[27]= 1.0f; d[28]= 1.0f; d[29]= 0.0f;
}
```

这些辅助函数是 `static inline` 的，因此它们会被内联到 `drawRectColor`、`drawRectUV` 等函数中。编译器会发出一长串带偏移寻址的 `f32.store` 指令，如 WASM 输出所示：

```wat
;; writeQuadVtxCol / writeQuadVtxUV 内联 - 每个四边形 30 个 f32.store
(f32.store
  (local.get $9)
  (local.get $1))
(f32.store offset=4
  (local.get $9)
  (local.get $2))
(f32.store offset=8
  (local.get $9)
  (local.get $5))
;; ... 一直到偏移 116，对应 30 个浮点数（6 个顶点 × 5 个浮点数）
```

---

## 7. WASM 指令映射总结

| C++ 模式                 | WASM 效果（之前）           | WASM 效果（之后）                     |
|--------------------------|---------------------------|---------------------------------------|
| `pass.push(a,b,c,...)`   | `call` + 多个 `i32.store` | 仅在调用点有 `i32.store`               |
| `vec.push_back(x)`       | `call` 到运行时           | 内联 `i32.store` / `f32.store`        |
| `tr->worldXYZ(x,y,z)`    | `call` + 加载/存储        | 内联 `f32.load`/`f32.store`           |
| 包含 `i % n`、`i/n` 的循环 | 每次迭代都有除法/取模      | 嵌套循环，指针递增                     |
| `writeQuadVtxCol(d,...)` | `call` + 存储             | 内联 `f32.store` 序列                  |

目标是用内联的 `f32.load`、`f32.store`、`i32.load`、`i32.store` 和算术运算序列替换 `call`，使热路径主要是直接内存访问和数学运算。

---

## 8. 相关源文件

| 文件 | 描述 |
|------|------|
| `src/graphics/wasm_command_buffer.h` | `push`、`push2`–`push5`、`SHINE_FORCE_INLINE` |
| `src/Container/SVector.h` | `push_back`、`resize_uninitialized`、内联方法 |
| `src/graphics/renderer_2d.cpp` | `writeQuadVtxCol`、`writeQuadVtxUV`、`flush`、顶点写入 |
| `src/demo/demo_game.cpp` | `update_vertices`、`update_instances`（指针递增，嵌套循环） |
| `src/game/transform.h` | `worldXYZ`、`worldXY`（内联） |
| `src/game/sprite_renderer.h` | `onRender`（调用 `worldXYZ`、绘制回调） |
| `src/util/wasm_compat.h` | `likely`、`unlikely`、`ptr_i32`、`f2i` |

---

## 构建与分析

- **构建**：`.\build.bat wasm smallwasm build release`
- **分析**：`dev/Program/smallwasm/tools/run_wasm_analysis.ps1`（调试构建）
- **WAT 输出**：`wasm2wat web/dist/output.gl.wasm -o output.gl.wat`
- **DWARF**：`llvm-dwarfdump web/dist/output.gl.wasm > output.gl.dwarf.txt`

---

## 权衡

| 关注点 | 方法 | 效果 |
|-------|------|------|
| **大小** | 热辅助函数使用 `noinline` | 二进制更小，更多 `call` 开销 |
| **速度** | 热辅助函数使用 `always_inline` | 二进制更大，更少 `call`，更多直接内存操作 |

当前配置偏向于**速度**（内联，直接内存访问）。对于大小关键的部署，可以在调用频率较低的路径上选择性地重新引入 `noinline`。