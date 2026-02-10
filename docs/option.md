# LLVM WebAssembly 编译、性能与体积优化分析指南

本文档整理了一套**面向 LLVM / wasm 后端开发者**的完整方法论，覆盖：

* LLVM → wasm 的编译链路
* wasm 运行性能优化
* wasm 体积优化
* wasm 产物的系统化分析方法

目标是：**少走弯路，快速定位问题，改到真正有收益的地方。**

---

## 一、LLVM → wasm 编译链路总览

完整编译流程：

```
C/C++
  ↓
clang frontend
  ↓
LLVM IR
  ↓
LLVM WebAssembly backend
  ↓
.wasm object
  ↓
lld (wasm-ld)
  ↓
最终 wasm 模块
```

可以优化的层级：

| 层级           | 决定什么     |
| ------------ | -------- |
| clang / IR   | 性能和体积的上限 |
| wasm backend | 实际运行性能   |
| wasm-ld      | 体积、启动速度  |

**核心原则：**

> wasm 优化一定是多层配合，而不是只改一个点。

---

## 二、优化目标与优先级

### 你的目标

* wasm **运行性能**
* wasm **体积**

### 推荐优先级

1. **WebAssembly backend（性能核心）**
2. **lld/wasm（体积立竿见影）**
3. **clang 默认参数（放大器）**

---

## 三、wasm 运行性能优化（重点）

### 1. WebAssembly Backend 关键目录

```
llvm/lib/Target/WebAssembly/
```

### 2. 性能最关键的文件

#### 2.1 WebAssemblyISelLowering.cpp

职责：

* IR → wasm 指令 lowering

重点函数：

* `LowerCall`
* `LowerReturn`
* `LowerFormalArguments`
* `LowerOperation`

常见性能问题：

* 参数在 stack / local 间来回搬
* 多余的 i32/i64 extend/trunc
* 频繁的 `local.get / local.set`

> wasm 是 stack machine，local 数量和搬运次数直接影响性能。

---

#### 2.2 WebAssemblyFrameLowering.cpp

职责：

* 函数 prologue / epilogue
* stack pointer (`__stack_pointer`) 管理

可优化点：

* 减少 `global.get/set __stack_pointer`
* 小函数避免建立 stack frame
* 更激进的 frame elision

> wasm 中 SP 操作是明确的性能杀手。

---

#### 2.3 WebAssemblyRegisterInfo.cpp

说明：

* wasm 的 register 本质是 **locals**

可优化方向：

* 合并 locals
* 缩短 live range
* 减少 spill（wasm spill 成本很高）

---

#### 2.4 WebAssemblyTargetMachine.cpp

职责：

* wasm 默认 pass pipeline

优化重点：

* 禁用对 wasm 无收益甚至负优化的 pass
* 提前执行 `mem2reg`
* 添加 wasm-specific peephole pass

原则：

> wasm 优化：减少指令数 > ILP

---

### 3. SIMD / Tail Call / Fast Math

* SIMD：`WebAssemblyInstrSIMD.td`
* Tail Call：减少 stack frame，适合递归和状态机

这是 **数量级提升性能** 的杠杆，但复杂度较高。

---

## 四、wasm 体积优化（高性价比）

### 1. lld/wasm（体积核心）

目录：

```
lld/wasm/
```

#### 1.1 Dead Code Elimination

* 改进符号可达性分析
* 避免 JS glue 强制保留未用符号

效果非常明显。

---

#### 1.2 Section Layout

* 合并 section
* 压缩 custom section
* 调整 import/export 顺序

---

#### 1.3 Symbol / Relocation 压缩

* aggressive strip
* 减少名字表

---

### 2. Backend 里的体积优化

* 减少 locals
* 精简 prologue / epilogue
* 消除 helper intrinsics

> 性能优化 ≈ 体积优化（80% 重叠）

---

### 3. clang 默认参数（不要忽略）

目录：

```
clang/lib/Driver/ToolChains/WebAssembly.cpp
```

建议：

* wasm 默认使用 `-Oz`
* 自动开启 `-ffunction-sections`
* 自动开启 `-fdata-sections`

这是 lld DCE 生效的前提。

---

## 五、wasm 产物分析方法（核心能力）

### 总体原则

> **一定要多层对照分析**

```
LLVM IR → wasm text → wasm 指令 → section → profiler
```

---

## 六、第一层：wasm → 可读形式

### 1. wasm → WAT

```bash
wasm2wat a.wasm -o a.wat
```

重点观察：

* `local.get / local.set`
* `i32.load / store`
* `global.get __stack_pointer`

经验法则：

> local 操作多 = backend 有问题

---

### 2. wasm 指令统计

```bash
wasm-objdump -d a.wasm
grep -c "local.get" a.wat
grep -c "global.get" a.wat
```

`__stack_pointer` 多 → FrameLowering 问题。

---

## 七、第二层：LLVM IR vs wasm

### 1. 查看 LLVM IR

```bash
clang --target=wasm32 -O2 test.c -S -emit-llvm
```

检查：

* `alloca` 是否已被 mem2reg
* 是否还有多余 load/store

判断：

* IR 干净 → backend 问题
* IR 本身很差 → clang / pipeline 问题

---

### 2. llc 单独跑 backend（必会）

```bash
llc -march=wasm32 test.ll -o test.s
llc -march=wasm32 -print-after-all test.ll
```

用途：

* 定位哪个 pass 引入了多余 local / SP

这是 LLVM wasm 优化的核心工具。

---

## 八、第三层：性能 Profiling

### 1. 浏览器 Profiling

* Chrome DevTools → Performance → WebAssembly
* Firefox wasm profiler

关注：

* 热函数
* 小函数是否 stack-heavy
* 调用深度异常

---

### 2. Runtime / perf

```bash
perf record wasmtime a.wasm
perf report
```

如果 runtime helper 占比高，说明 wasm 本身过于啰嗦。

---

## 九、第四层：体积分析

### 1. Section 分析

```bash
wasm-objdump -h a.wasm
```

重点：

* code section
* name section
* custom section

---

### 2. 函数级体积分析

```bash
wasm-split a.wasm
```

可精确定位哪个函数是体积大头。

---

### 3. 符号 / 导出分析

```bash
wasm-objdump -x a.wasm
```

用于验证 lld DCE 是否真正生效。

---

## 十、常见 wasm 反模式（看到就该警觉）

### 1. 频繁 stack pointer 调整

```
global.get __stack_pointer
i32.const 16
i32.sub
global.set __stack_pointer
```

### 2. local 来回搬

```
local.get 2
local.set 3
local.get 3
```

### 3. 小函数却有完整 frame

这是 backend 或 frame lowering 的典型问题。

---

## 十一、推荐的实战第一步

**第一刀建议：**

1. 写 wasm micro-benchmark
2. 使用 `llc -print-after-all`
3. 定位多余 `local.get/set` 或 SP 操作
4. 实现简单 peephole pass 消除它们

这是：

* 最容易出结果
* 最容易 upstream 的路径

---

## 十二、总结

* wasm 优化不是“玄学”，而是**结构性工程问题**
* 性能与体积高度相关
* 真正高价值的优化集中在 backend 和 linker
* 数据和分析永远比感觉重要

> 你已经站在 LLVM wasm 开发者的正确路径上。
