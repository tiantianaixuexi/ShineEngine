---
name: ShineEngine-Project-Skills
description: ShineEngine 项目的核心技术架构、模块划分及开发验证标准清单
---

# ShineEngine 项目技能清单

## 描述
本文档基于对 ShineEngine 源代码及构建脚本的深度审计生成，旨在为开发者和 AI 提供关于引擎核心能力的详细技术参考。它定义了项目在架构、渲染、框架、构建及合规性方面的技术标准和实现路径。

## 使用场景
- 当需要了解 ShineEngine 的核心架构设计与模块划分时
- 当需要查询渲染管线、资源管理或跨平台适配的具体实现路径时
- 当需要验证代码质量或性能指标（如 FPS、内存碎片率）时
- 在进行构建配置排查或执行 CI/CD 流程时

## 详情

### 1. 核心架构 (Core Architecture)

#### 1.1 模块化系统 (Modular System)
- **技术要点**: JSON 驱动的依赖管理，支持动态/静态库切换。
- **实现路径**: 定义于 `Module/*.json`，通过 CMake `parse_and_cache_modules` 解析。
- **验证标准**: 循环依赖检测耗时 < 1s，模块加载成功率 100%。

#### 1.2 反射系统 (Reflection System)
- **技术要点**: 运行时类型信息 (RTTI)，宏自动注册。
- **实现路径**: `EngineCore/reflection`，支持字段、方法、枚举元数据提取。
- **验证标准**: 反射调用开销 < 原生调用 2x，覆盖率 ≥ 95%。

#### 1.3 Tick 调度 (Tick Scheduling)
- **技术要点**: 基于 Kahn 算法的依赖图拓扑排序。
- **实现路径**: `TickManager` 对 `TickFunction` 进行排序，支持分层（Pre/Physics/Post）。
- **验证标准**: 调度开销 < 0.05ms，无死锁，拓扑排序正确性 100%。

#### 1.4 内存管理 (Memory Management)
- **技术要点**: 高性能分配器集成与标签监控。
- **实现路径**: 集成 `mimalloc`，通过 `ReflectionMemory` 实现带标签监控。
- **验证标准**: 内存分配速度提升 ≥ 20%，内存碎片率 < 5%。

#### 1.5 对象模型 (Object Model)
- **技术要点**: 实体组件系统 (Actor-Component)。
- **实现路径**: `SObject` 提供 GUID，`Actor` 聚合 `UComponent`，由 `World` 管理。
- **验证标准**: 对象创建/销毁吞吐量 ≥ 10k/s，无内存泄漏。

#### 1.6 事件系统 (Event System)
- **技术要点**: 泛型事件分发与解耦。
- **实现路径**: 集成 `eventpp`，使用 `EventDispatcher` 和 `CallbackList`。
- **验证标准**: 事件分发延迟 < 10μs，监听器上限 ≥ 1000。

#### 1.7 全局上下文 (Global Context)
- **技术要点**: 单例生命周期管理。
- **实现路径**: `EngineContext` 统一管理 `Subsystem` 实例（Input, Asset, Render）。
- **验证标准**: 子系统初始化顺序正确率 100%，全局访问无锁竞争。

#### 1.8 数学库优化 (Math Optimization)
- **技术要点**: SIMD/WASM 指令集加速。
- **实现路径**: `math/` 模块，WASM 平台使用 `__builtin_elementwise` 优化。
- **验证标准**: 矩阵乘法性能 ≥ 10M ops/s，单元测试通过率 100%。

### 2. 渲染管线 (Render Pipeline)

#### 2.1 可脚本化管线 (Scriptable Pipeline)
- **技术要点**: 渲染流程解耦，支持自定义 Pass。
- **实现路径**: `ScriptableRenderContext` 收集指令，`RenderPipeline` 定义流程。
- **验证标准**: 管线切换耗时 < 1ms，支持自定义 Pass 数量 ≥ 10。

#### 2.2 渲染后端抽象 (RHI Abstraction)
- **技术要点**: 跨平台图形 API 隔离 (OpenGL/WebGL)。
- **实现路径**: `IRenderBackend` 接口层，隔离 GL 3.3 与 WebGL 2.0。
- **验证标准**: 跨平台渲染一致性 ≥ 99% (Pixel Diff)，API 调用零错误。

#### 2.3 命令缓冲区 (Command Buffer)
- **技术要点**: 延迟执行模型与多线程录制支持。
- **实现路径**: `CommandBuffer` 序列化渲染指令。
- **验证标准**: 指令录制吞吐量 ≥ 1M cmds/s，回放开销 < 0.1ms。

#### 2.4 材质系统 (Material System)
- **技术要点**: 数据驱动着色器与 Uniform 自动绑定。
- **实现路径**: `Material` 类管理 Shader 参数。
- **验证标准**: 材质切换开销 < 10μs，Shader 编译错误率 0%。

#### 2.5 纹理管理 (Texture Management)
- **技术要点**: 多格式异步加载与引用计数。
- **实现路径**: `TextureManager` 集成 `stb_image`，支持 PNG/JPG/WebP。
- **验证标准**: 纹理加载速度 ≥ 100MB/s，显存泄漏 = 0。

#### 2.6 UI 渲染 (UI Rendering)
- **技术要点**: ImGui 深度集成与 Docking 布局。
- **实现路径**: `imgui_impl_opengl3` 后端绑定。
- **验证标准**: UI 渲染帧率 ≥ 60 FPS，DrawCall < 50 (Batching)。

#### 2.7 后处理特效 (Post Processing)
- **技术要点**: Framebuffer Ping-Pong 处理。
- **实现路径**: `PostProcessPass` 实现屏幕空间特效。
- **验证标准**: 全屏特效耗时 < 2ms (1080p)，显存占用优化。

#### 2.8 模型加载 (Model Loading)
- **技术要点**: GLTF 2.0 完整支持。
- **实现路径**: `gltfLoader` 解析 Mesh/Material/Node 树。
- **验证标准**: 解析速度 ≥ 50MB/s，顶点属性完整性 100%。

### 3. 游戏框架 (Gameplay Framework)

#### 3.1 脚本绑定 (Script Binding)
- **技术要点**: JS/TS 互操作与 C++ 接口暴露。
- **实现路径**: 集成 QuickJS，通过 `ScriptBridge` 暴露接口。
- **验证标准**: JS 执行开销 < 5ms/frame，绑定接口覆盖率 ≥ 80%。

#### 3.2 输入系统 (Input System)
- **技术要点**: 跨平台输入映射 (Win32/Web)。
- **实现路径**: `InputManager` 统一处理消息与事件。
- **验证标准**: 输入延迟 < 16ms，按键响应丢失率 0%。

#### 3.3 场景管理 (Scene Management)
- **技术要点**: 层级化节点树与可视化。
- **实现路径**: `Scene` 管理 `Actor` 树，配合 `SceneHierarchyView`。
- **验证标准**: 场景节点数支持 ≥ 10k，遍历更新耗时 < 2ms。

#### 3.4 相机系统 (Camera System)
- **技术要点**: 视图矩阵计算与视锥体剔除。
- **实现路径**: `CameraManager` 管理投影与剔除逻辑。
- **验证标准**: 剔除准确率 100%，矩阵计算精度误差 < 1e-6。

#### 3.5 插件机制 (Plugin Mechanism)
- **技术要点**: 动态库热加载。
- **实现路径**: `PluginsManager` 使用 `LoadLibraryA` 加载 DLL。
- **验证标准**: 插件加载耗时 < 500ms，崩溃隔离成功率 100%。

#### 3.6 文件监视 (File Watching)
- **技术要点**: 热重载支持。
- **实现路径**: `FileWatchManager` 监控资源目录变更。
- **验证标准**: 变更检测延迟 < 1s，回调触发准确率 100%。

#### 3.7 性能分析 (Profiling)
- **技术要点**: 实时 CPU/Memory 监控。
- **实现路径**: 集成 `Tracy` 及内置 `FpsControlView`。
- **验证标准**: Profiler 自身开销 < 5%，采样精度 ≥ 1μs。

#### 3.8 编辑器框架 (Editor Framework)
- **技术要点**: 视图组件化与面板系统。
- **实现路径**: `EditorView` 基类派生各功能面板。
- **验证标准**: 编辑器启动时间 < 3s，面板响应延迟 < 50ms。

### 4. 构建与平台 (Build & Platform)

#### 4.1 现代构建系统 (Modern Build System)
- **技术要点**: CMake 4.1+ 配置与 C++20 Modules。
- **实现路径**: 自定义 `parse_json_type` 宏，多配置管理。
- **验证标准**: CMake 配置耗时 < 10s，构建成功率 100%。

#### 4.2 WASM 工具链 (WASM Toolchain)
- **技术要点**: Emscripten 适配与子工程构建。
- **实现路径**: `subcmake` 机制独立编译 `smallwasm`。
- **验证标准**: WASM 包体积 < 10MB (Gzip)，浏览器兼容性 Chrome/Edge/FF。

#### 4.3 批处理自动化 (Batch Automation)
- **技术要点**: 环境自检与参数注入。
- **实现路径**: `Build.bat` 检测 VS 版本与构建模式。
- **验证标准**: 脚本执行错误码准确率 100%，环境变量污染 = 0。

#### 4.4 注册表集成 (Registry Integration)
- **技术要点**: 文件关联扩展。
- **实现路径**: `Register.bat` 操作 `HKEY_CLASSES_ROOT`。
- **验证标准**: 注册表写入成功率 100%，卸载清理残留 = 0。

#### 4.5 跨平台抽象 (Platform Abstraction)
- **技术要点**: 平台宏隔离与 OS 封装。
- **实现路径**: `src/platform` 区分 Win32/WASM 实现。
- **验证标准**: 跨平台编译通过率 100%，特定平台代码隔离度 100%。

#### 4.6 持续集成 (CI Integration)
- **技术要点**: 命令行构建接口。
- **实现路径**: `Build.bat` 支持 `--no-pause` 及错误码返回。
- **验证标准**: CI 流水线单次构建耗时 < 5min，日志可读性 Pass。

### 5. 自动化合规 (Automation Compliance)

#### 5.1 脚本静态扫描 (Script Linting)
- **策略**: ShellCheck / CMake-Lint。
- **实施**: CI 阶段扫描 `Build.bat` 和 `CMakeLists.txt`。
- **指标**: Critical/High 级别警告 = 0。

#### 5.2 权限最小化 (Least Privilege)
- **策略**: UAC 控制。
- **实施**: `Register.bat` 仅必要时提权，`Build.bat` 普通权限。
- **指标**: 非必要 Admin 请求 = 0。

#### 5.3 数字签名 (Code Signing)
- **策略**: SignTool 签名。
- **实施**: 发布前对 `MainEngine.exe` 及核心 DLL 签名。
- **指标**: 签名验证通过，证书有效。

#### 5.4 依赖 SBOM (Dependency SBOM)
- **策略**: CycloneDX / SPDX。
- **实施**: 解析 `Module/*.json` 生成软件物料清单。
- **指标**: SBOM 覆盖率 100%，高危漏洞组件 = 0。
