# Tracy Profiler API 使用速查

本文档提供 Tracy Profiler 最常用的 API 宏和函数，方便在代码中快速查阅。所有宏在未定义 `TRACY_ENABLE` 时均定义为空操作，不影响性能。

## 基本设置

| 宏 / 函数 | 说明 |
|-----------|------|
| `TRACY_ENABLE` | 全局定义此宏以启用分析（必须为整个项目定义）。 |
| `tracy::SetThreadName(name)` | 为当前线程设置名称，显示在时间线上。 |
| `tracy::SetThreadNameWithHint(name, groupHint)` | 设置线程名并指定分组提示（`int32_t`）。 |

## 帧标记

| 宏 | 说明 |
|----|------|
| `FrameMark` | 标记一个连续帧的结束（如 SwapBuffers 后）。 |
| `FrameMarkNamed(name)` | 标记一个命名的连续帧集。 |
| `FrameMarkStart(name)` | 标记一个非连续帧的开始。 |
| `FrameMarkEnd(name)` | 标记一个非连续帧的结束。 |
| `FrameImage(image, width, height, offset, flip)` | 发送当前帧的图像数据（RGBA）。 |

## 区域标记（Zone）

### 基本区域

| 宏 | 说明 |
|----|------|
| `ZoneScoped` | 标记一个区域，自动记录函数名、文件、行号。 |
| `ZoneScopedC(color)` | 带颜色的区域（`color` 为 0xRRGGBB）。 |
| `ZoneScopedN(name)` | 带自定义名称的区域。 |
| `ZoneScopedNC(name, color)` | 同时指定名称和颜色。 |
| `ZoneText(text, size)` | 为当前区域附加文本。 |
| `ZoneColor(color)` | 动态修改当前区域颜色。 |
| `ZoneValue(value)` | 附加一个数值（`uint64_t`）。 |
| `ZoneName(text, size)` | 动态修改区域名称（不影响统计分组）。 |
| `ZoneIsActive` | 返回当前区域是否活跃。 |

### 命名区域（用于一个作用域内多个区域）

| 宏 | 说明 |
|----|------|
| `ZoneNamed(var, active)` | 创建一个命名区域变量，`active` 为 `bool` 控制是否启用。 |
| `ZoneNamedN(var, name, active)` | 带名称的命名区域。 |
| `ZoneNamedC(var, color, active)` | 带颜色的命名区域。 |
| `ZoneNamedNC(var, name, color, active)` | 带名称和颜色的命名区域。 |
| `ZoneTextV(var, text, size)` | 为命名区域附加文本。 |
| `ZoneColorV(var, color)` | 为命名区域动态改色。 |
| `ZoneValueV(var, value)` | 为命名区域附加数值。 |
| `ZoneNameV(var, text, size)` | 为命名区域动态改名。 |

### 临时区域（适用于动态代码场景）

| 宏 | 说明 |
|----|------|
| `ZoneTransient(var, active)` | 临时区域，复制源位置到堆，避免字符串生命周期问题。 |
| `ZoneTransientN(var, name, active)` | 带名称的临时区域。 |

## 锁标记

| 宏 | 说明 |
|----|------|
| `TracyLockable(type, var)` | 定义一个可追踪的锁（替换 `type var`）。 |
| `TracyLockableN(type, var, description)` | 带描述的可追踪锁。 |
| `LockableBase(type)` | 用于模板参数（如 `std::lock_guard`）。 |
| `LockMark(var)` | 标记锁持有位置（可选）。 |
| `TracySharedLockable(type, var)` | 定义可追踪的读写锁。 |
| `SharedLockableBase(type)` | 读写锁的模板参数基。 |

## 消息日志

| 宏 | 说明 |
|----|------|
| `TracyMessage(text, size)` | 发送文本消息。 |
| `TracyMessageL(text)` | 发送字符串字面量消息。 |
| `TracyMessageC(text, size, color)` | 带颜色的消息。 |
| `TracyMessageLC(text, color)` | 带颜色的字符串字面量消息。 |

## 绘图数据

| 宏 | 说明 |
|----|------|
| `TracyPlot(name, value)` | 记录一个数值点（`value` 可为整数或浮点数）。 |
| `TracyPlotConfig(name, format, step, fill, color)` | 配置绘图显示方式（`format` 为 `tracy::PlotFormatType`）。 |

## 内存分析

| 宏 | 说明 |
|----|------|
| `TracyAlloc(ptr, size)` | 记录内存分配。 |
| `TracyFree(ptr)` | 记录内存释放。 |
| `TracySecureAlloc(ptr, size)` | 安全分配（用于 TLS 销毁等场景）。 |
| `TracySecureFree(ptr)` | 安全释放。 |
| `TracyAllocN(ptr, size, name)` | 记录命名内存池的分配。 |
| `TracyFreeN(ptr, name)` | 记录命名内存池的释放。 |

## GPU 分析

### OpenGL

| 宏 | 说明 |
|----|------|
| `TracyGpuContext` | 声明 OpenGL 上下文（每个线程一个）。 |
| `TracyGpuContextName(name, size)` | 设置上下文名称。 |
| `TracyGpuZone(name)` | 标记 GPU 区域。 |
| `TracyGpuZoneC(name, color)` | 带颜色的 GPU 区域。 |
| `TracyGpuCollect` | 收集 GPU 事件（每帧调用）。 |

### Vulkan

| 宏 | 说明 |
|----|------|
| `TracyVkContext(physdev, device, queue, cmdbuf)` | 创建 Vulkan 上下文，返回 `ctx`。 |
| `TracyVkContextCalibrated(...)` | 创建校准后的上下文。 |
| `TracyVkDestroy(ctx)` | 销毁上下文。 |
| `TracyVkContextName(ctx, name, size)` | 设置上下文名称。 |
| `TracyVkZone(ctx, cmdbuf, name)` | 标记 GPU 区域。 |
| `TracyVkZoneC(ctx, cmdbuf, name, color)` | 带颜色的 GPU 区域。 |
| `TracyVkCollect(ctx, cmdbuf)` | 收集事件。 |

### Direct3D 11/12、Metal、OpenCL、CUDA

类似 Vulkan，有对应的 `TracyD3D11Context`、`TracyD3D12Zone`、`TracyMetalContext`、`TracyCLContext`、`TracyCUDAContext` 等宏。详细请参考原手册。

## 纤程 / 协程

| 宏 | 说明 |
|----|------|
| `TracyFiberEnter(fiber)` | 进入纤程（`fiber` 为唯一字符串标识）。 |
| `TracyFiberLeave` | 离开纤程。 |

## 调用栈收集

使用带 `S` 后缀的宏，并指定深度：

| 宏 | 说明 |
|----|------|
| `ZoneScopedS(depth)` | 区域标记并捕获调用栈。 |
| `ZoneScopedNS(name, depth)` | 带名称的区域加调用栈。 |
| `ZoneScopedCS(color, depth)` | 带颜色的区域加调用栈。 |
| `ZoneScopedNCS(name, color, depth)` | 名称+颜色+调用栈。 |
| `TracyAllocS(ptr, size, depth)` | 内存分配加调用栈。 |
| `TracyFreeS(ptr, depth)` | 内存释放加调用栈。 |
| `TracyMessageS(text, size, depth)` | 消息加调用栈。 |

## 辅助功能

| 宏 / 函数 | 说明 |
|-----------|------|
| `TracyIsConnected` | 返回当前是否有服务器连接。 |
| `TracyAppInfo(text, size)` | 设置应用程序信息（版本、环境等）。 |
| `TracySourceCallbackRegister(callback, data)` | 注册源文件回调以提供源代码。 |
| `TracyParameterRegister(callback, data)` | 注册参数回调，以支持运行时参数调整。 |

---

**注意**：所有宏中的字符串参数（如名称、文本）通常要求具有静态生命周期（字符串字面量或永不释放的缓冲区）。若使用动态字符串，请使用带 `size` 参数的版本，Tracy 会内部复制。