# ShineEngine 内存分配器迁移方案（mimalloc -> UE5 MallocBinned2）

## 1. 目标与边界

- 目标：将 `src/memory` 的底层分配后端从 mimalloc 迁移到 UE5 的 `FMallocBinned2`。
- 保留：`MemoryTag`、`MemoryScope`、`TaggedAllocator`、全局 `new/delete`、现有统计接口。
- 分阶段切换：默认仍走 mimalloc，新增开关后可逐步切换到 UE Binned2，降低一次性替换风险。

## 2. 当前现状（已确认）

- 现有分配路径在 `memory.cpp` 中直接调用：
  - `mi_malloc_aligned`
  - `mi_free`
  - `mi_realloc_aligned`
- `Memory::Alloc/Free/Realloc` 依赖 `AllocationHeader` 保存 `size/tag/offset`，并维护 tag 统计。
- 全局 `operator new/delete` 已统一路由到 `shine::co::Memory`。

## 3. UE5 MallocBinned2 核心机制（与迁移强相关）

- 小内存分配：
  - 按 bin size 映射到 `FPoolTable`。
  - 每个池页用 `FFreeBlock` 管理连续空闲 bin。
- 大内存分配：
  - 走 OS 分配路径，并通过池映射结构记录分配信息。
- 并发模型：
  - 每个 bin table 自带互斥。
  - 配合线程缓存降低热路径锁竞争。
- 安全与校验：
  - canary 检测块头有效性。
  - fork 场景有额外 canary 状态分支。

## 4. 迁移总体设计

### 4.1 后端抽象层

在 `src/memory` 增加后端抽象，不改上层 API：

- `Alloc(size, align)`
- `Free(ptr)`
- `Realloc(ptr, size, align)`
- 可选：`GetAllocationSize(ptr)`（用于优化统计精度）

后端实现：

- MimallocBackend（默认）
- UEBinned2Backend（新加）

### 4.2 切换策略

- 编译期开关：`SHINE_MEMORY_BACKEND_UE_B2`
  - `0`：mimalloc（默认）
  - `1`：UE Binned2
- 这样可以做到：
  - 先合入代码与结构
  - 再逐步补齐 UE 依赖
  - 最后切换开关进行真实替换

### 4.3 与现有 Header 协议兼容

当前 `AllocationHeader` 继续保留，原因：

- 保持 `MemoryTag` 与统计逻辑不变。
- 避免一次性改动上层调用代码。
- 后续如需进一步贴近 UE 统计，可再做二阶段重构。

## 5. 实施步骤（逐步执行）

### Step 1：新增迁移文档与后端开关（当前阶段）

- 落地本文档。
- 在 `memory.ixx/.cpp` 增加后端选择宏，默认不改变行为。

### Step 2：重构 `memory.cpp` 到后端接口

- 把直接 `mi_*` 调用替换成统一后端函数调用。
- 保证默认配置下行为与当前一致。

### Step 3：新增 UE Binned2 适配层骨架

- 新建 `ue_binned2_port.h/.cpp`。
- 直接移植 Binned2 关键策略（64KB 页、SmallBinSizes、按池回收、大块路径）。
- 不再直接包含 `UE/public/HAL/MallocBinned2.h`，避免 UE 基础设施缺失导致无法推进。

### Step 4：验证与灰度切换

- 默认后端（mimalloc）编译 + 基础运行验证。
- 开启 UE Binned2 后端进行编译验证，逐项补齐依赖缺口。
- 最终在测试目标中替换并验证稳定性。

## 6. 你需要补齐/确认的 UE 依赖（第一批）

以下为迁移 `FMallocBinned2` 时最关键的依赖面，建议先确认这些头/实现是否完整可用：

- `UE/public/HAL/MallocBinned2.h`
- `UE/private/HAL/MallocBinned2.cpp`
- `UE/public/HAL/MallocBinnedCommon.h`
- `UE/private/HAL/MallocBinnedCommon.cpp`
- `UE/public/HAL/MallocBinnedCommonUtils.h`
- `UE/public/HAL/Allocators/CachedOSPageAllocator.h`
- `UE/private/HAL/Allocators/CachedOSPageAllocator.cpp`

平台与基础设施接口（至少要有可编译实现）：

- `FPlatformMemory::{GetConstants, BinnedAllocFromOS, BinnedFreeToOS, OnOutOfMemory}`
- `FPlatformTLS`
- 平台锁与线程原语

可按需裁剪（先关再开）：

- LLM/Stats/CSV 统计宏相关模块
- VeryLargePageAllocator 路径
- Fork 支持路径

## 7. 风险与规避

- 风险：UE 依赖链较深，直接全量迁移可能出现大量编译缺失。
  - 规避：先完成后端抽象 + 默认后端稳定，再分批接入 UE 依赖。
- 风险：对齐/页粒度假设与当前平台不一致。
  - 规避：先在 Windows 单平台固定配置验证，再扩展。
- 风险：一次性切换影响全局 `new/delete`。
  - 规避：保留编译期开关，可快速回退。

## 8. 验收标准

- 默认配置下，`memory` 模块行为与当前一致。
- 开启 `SHINE_MEMORY_BACKEND_UE_B2` 时：
  - 能在依赖齐全条件下编译通过。
  - `Memory::Alloc/Free/Realloc` 基本路径可运行。
  - `MemoryTag` 统计功能保持可用。
