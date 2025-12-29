# Tick System Analysis & Roadmap

本文档分析 `shine::gameplay::tick` 模块的现状，并针对“引擎级 Tick 系统”与“WASM 支持”两个目标提出改进建议。

## 1. 现状分析 (Current Status)

目前的实现包含以下核心功能：
*   **分层执行 (Tick Groups)**: 支持 `PrePhysics`, `Physics`, `PostPhysics`, `Late` 四个阶段。
*   **依赖管理 (Dependency Graph)**: 使用 Kahn 算法进行拓扑排序，确保 Tick 执行顺序满足依赖关系。
*   **基础控制**: 支持 `interval` (Tick 间隔) 和 `enable` (开关) 控制。
*   **轻量级封装**: 使用 `TickFunction` 结构体和函数指针，解耦了具体对象。

**优点**: 逻辑清晰，依赖解决算法正确，对简单游戏逻辑足够使用。

---

## 2. 迈向引擎级 Tick 系统的差距 (Gaps to Engine-Level)

要达到商业引擎（如 Unreal, Unity）的级别，尚缺以下关键特性：

### 2.1 并行化与任务图 (Parallelism & Task Graph)
*   **问题**: 目前 `ExecutePhase` 是单线程串行执行的 (`for` 循环)。如果场景中有 1000 个互不依赖的组件，它们依然会排队执行，无法利用多核 CPU。
*   **改进**:
    *   引入 **Job System (任务系统)**。
    *   在构建依赖图时，识别出“入度为 0”的一组任务，这些任务可以**并行分发**到 Worker Threads。
    *   使用 `ParallelFor` 替代简单的 `for` 循环。

### 2.2 固定时间步长 (Fixed Timestep)
*   **问题**: `ExecuteAll(float dt)` 接受的是变长帧时间 (Variable Delta Time)。这对于 `PrePhysics` 和 `Physics` 组是危险的，因为物理模拟需要固定的时间步长（如 0.02s）以保证确定性和稳定性。
*   **改进**:
    *   将 Tick 系统拆分为 **Fixed Update** (物理相关) 和 **Variable Update** (渲染/逻辑相关)。
    *   实现“累积时间器” (Accumulator) 模式，在一次渲染帧中可能执行多次物理 Tick。

### 2.3 性能分析与监控 (Profiling & Stats)
*   **问题**: 无法得知哪个 Tick 函数耗时最长，导致性能瓶颈难以定位。
*   **改进**:
    *   集成 **Profiler** 宏 (如 `SHINE_PROFILE_SCOPE`)。
    *   统计每个 Group 的总耗时。
    *   检测“长耗时 Tick”并发出警告。

### 2.4 数据局部性 (Data Locality / Cache Friendliness)
*   **问题**: `TickFunction` 使用 `void* userdata` 并回调。这通常意味着指针跳转 (Pointer Chasing)，导致 CPU 缓存未命中 (Cache Miss)，特别是当组件散落在内存各处时。
*   **改进**:
    *   对于高性能需求的组件（如粒子、简单移动逻辑），应采用 **ECS (Entity Component System)** 或 **Data-Oriented** 的数组批量处理，而不是逐个对象回调。

---

## 3. WASM (WebAssembly) 支持分析

WebAssembly 环境对性能和内存布局有特殊限制，为了更好地支持 WASM，需要注意以下几点：

### 3.1 减少间接调用 (Indirect Calls Overhead)
*   **挑战**: 在 WASM 中，通过函数指针 (`TickFn`) 调用函数（间接调用）比直接调用慢，因为需要查找 Table 表并进行类型检查。目前的 `fn(userdata, dt)` 每一帧对每个对象都会触发一次间接调用。
*   **优化**:
    *   **Batching (批处理)**: 尽量减少 Tick 函数的数量。例如，不要让 1000 个子弹对象各自注册 Tick，而是注册一个全局的 `BulletManager` Tick，在内部用紧凑循环更新所有子弹。
    *   **Devirtualization**: 如果使用 C++ 虚函数 (Virtual Function)，在 WASM 中同样有开销。尽量使用模板或具体的 Manager 类。

### 3.2 内存对齐与布局 (Memory Layout)
*   **挑战**: WASM 是 32 位（通常）或 64 位线性内存。
*   **优化**: 确保 `TickFunction` 结构体的大小是对齐友好的。目前的结构体包含 `std::vector`，在频繁创建/销毁 Tick 时可能会导致内存碎片。建议使用 **Object Pool (对象池)** 来分配 `TickFunction`，减少 `new/delete` 开销。

### 3.3 线程模型 (Threading)
*   **挑战**: WASM 的多线程（SharedArrayBuffer + Web Workers）与原生 OS 线程不同。主线程通常不能阻塞。
*   **优化**: 如果实现并行 Tick，必须适配 Web Workers 的消息传递或共享内存机制。对于单线程 WASM 构建，Tick Manager 必须能退化为串行执行。

### 3.4 异常处理 (Exception Handling)
*   **挑战**: WASM 开启 C++ 异常捕获会显著增加包体大小并降低性能。
*   **现状**: 目前代码使用了 `assert`，未见 `try-catch`，这是好的。
*   **建议**: 保持无异常设计 (No-Exception)，错误处理通过返回值或 Error Code 进行。

---

## 4. 改进路线图 (Roadmap)

1.  **Phase 1 (架构)**: 实现 **FixedUpdate Loop**，将物理相关的 Group 剥离出来单独驱动。
2.  **Phase 2 (性能)**: 引入 **Tick Context**，传递更多信息而不仅仅是 `dt`。为 `TickFunction` 增加性能统计计时。
3.  **Phase 3 (并行)**: 集成 Job System，利用拓扑排序的层级信息进行并行分发。
4.  **Phase 4 (WASM)**: 编写 `BulletManager` 或类似 Manager 类的范例，展示如何批量更新以减少 WASM 间接调用开销。

## 5. 总结

当前的 `TickManager` 是一个标准的面向对象实现，逻辑正确但性能上限较低。为了达到引擎级别，核心在于**从“逐个对象调度”转向“批量/并行调度”**，并针对 WASM 的特性减少函数指针调用频率。
