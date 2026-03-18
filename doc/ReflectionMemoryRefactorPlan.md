# 反射系统与底层内存系统改进任务书

## 背景

基于 FFieldVariant 的思路，这次改进的核心不是单纯模仿 UE5 接口，而是把三件事真正落地到当前工程中：

1. 用标记指针压缩反射对象之间的归属关系。
2. 用热冷分离压缩反射描述对象尺寸，减少无效元数据开销。
3. 用专用内存域、池化和连续布局，让反射数据真正获得缓存友好和低碎片收益。

当前工程已有基础：

- 反射注册、字段/方法描述与查找已经可用。
- ScriptView 与 InspectorView 已经基于 TypeInfo 消费反射数据。
- 底层内存已经具备 UE Binned2 风格的小块池、线程缓存和大块分配路径。

现阶段的主要问题不是功能缺失，而是对象表达、内存布局和生命周期管理还不够轻量，导致反射系统没有完全吃到内存系统的优势。

## 总体目标

### 目标一：压缩反射对象模型

将 TypeInfo、FieldInfo、MethodInfo 从“灵活但偏重”的运行时描述对象，重构为“热路径紧凑、冷数据外置”的布局。

### 目标二：引入反射归属标记句柄

参考 FFieldVariant 的 tagged pointer 思路，为字段、方法、容器子节点等反射节点提供统一归属句柄，避免为了表达 owner 关系而引入额外对象层级和额外指针字段。

### 目标三：建立反射专用内存域

将反射注册期对象、元数据块、字符串、脚本桥接临时对象从通用堆分配路径中分离出来，改为专用 arena 或 slab，减少碎片、降低 header 成本，并提升局部性。

### 目标四：让底层内存系统为布局服务

不只关注“能分配”和“统计字节数”，而是围绕对齐、缓存行、对象尺寸、分配路径和生命周期做约束与验证。

## 当前执行状态（截至 2026-03-18）

下面这一节用于给新窗口或后续继续开发时快速接手，记录的是当前代码已经落地的状态，而不是计划目标。

### 已完成

- Issue 01 已完成：反射对象尺寸、对齐、成员偏移、缓存行跨度基线已经接入 ReflectionTest，可直接输出 TypeInfo、FieldInfo、MethodInfo、FieldColdData、MethodColdData、MethodCallCache、MetadataValue、EnumEntry、ScriptValue、ScratchBuffer 的布局信息。
- Issue 02 已完成：已新增 ReflectionOwnerHandle，并用于 FieldInfo 和 MethodInfo 的 owner 表达。TypeRegistry::Register 会在稳定地址生成后重新回填 owner handle，避免指向注册前临时 TypeInfo。
- Issue 03 已完成第一阶段并稳定：Category、DisplayName、Min、Max、EditCondition 不再走高频 metadata 线性扫描，已提升为内建元数据访问路径。InspectorView、InspectorBuilder、PropertyDrawer 已优先消费这些固定访问器。
- Issue 04 已完成第一阶段：FieldInfo 和 MethodInfo 已完成热冷分离。当前热区保留哈希、偏移、flags、访问器、owner handle、调用缓存，名字、UI schema、扩展 metadata 已收敛到 FieldColdData 和 MethodColdData。
- Issue 08 已部分完成：InspectorView、PropertyDrawer、StaticInspector 的主要消费路径已适配热区/冷区访问器，不再依赖旧的直接成员访问。
- Issue 09 已完成：MemoryTag 已细化出 ReflectionMeta、ReflectionCold、ReflectionTemp、ReflectionString、ScriptBridgeTemp、EditorInspectorTemp，ReflectionTest 已能输出相关 tag 的分配增量。
- Issue 10 已完成：底层分配器已显式提供 tagged pointer 对齐前提和可用低位 bit 查询，UE Binned2 统计已增加 bin 维度、线程缓存命中和 tagged pointer 对齐信息。
- Issue 12 已部分完成：StaticInspector 的 FunctionSelector 分支已移除运行时补注册，UI 绘制路径不再在查询时偷偷修改 TypeRegistry。

### 当前关键实现结果

- 当前 ReflectionTest 在本地 MSVC Debug 运行下给出的布局基线为：TypeInfo 176 B，TypeColdData 48 B，FieldInfo 128 B，MethodInfo 192 B，FieldColdData 144 B，MethodColdData 48 B，MethodCallCache 104 B，MetadataValue 40 B，EnumEntry 24 B，ScriptValue 40 B，ScratchBuffer 144 B。
- 当前热区跨度基线为：TypeInfo hot_bytes=96，FieldInfo hot_bytes=120，MethodInfo hot_bytes=184。也就是说，TypeInfo 已因为 enumEntries 与 type name 冷化从 216 B/144 hot bytes 继续压到 176 B/96 hot bytes，FieldInfo 已压到 2 个缓存线内，但 MethodInfo 热区仍然偏大，后续仍有继续瘦身空间。
- 当前 tagged pointer 前提已被运行时基线直接打印：min_align=16，usable_low_bits=4，当前底层分配器满足 2 bit 和 4 bit tag 的使用前提。
- 当前 ReflectionTest 已能直接验证：owner handle 正确、内建 metadata 可读、EditCondition 可见性正确、内存 tag 路由正确、tagged pointer 前提正确输出。
- 当前冷区已经从按类型分池推进到 ReflectionCold 冷数据页。FieldColdData 和 MethodColdData 现在按类型落到页内连续槽位，优先顺序填充当前页、回收后复用旧槽位；注册宏会先跑一次计数 builder，再让真实 TypeBuilder 按字段数/方法数进入批量页保留，并按计划预铺 fields、methods、enumEntries 的热区槽位，然后用写指针原地发射描述对象；同时 TypeInfo 的 type name 与 enumEntries 都已经移入 TypeColdData，字段/方法注册时也会先分配冷块、直接写入 name 与后续 fluent builder 的 UI schema、DisplayName、Range、扩展 metadata，避免在注册阶段反复走 EnsureColdData 和冷区回填；TypeRegistry 现在已经进一步切到 ReflectionMeta arena 持有，types_ 只保留稳定槽位视图，idRegistry_ / nameRegistry_ 继续把 TypeId 和类型名映射到槽位索引，按名查找的回退路径不再需要额外对象所有权。同一批次注册的 TypeInfo 已能稳定聚集到 arena 页内，但最终的 arena/graph builder 目标还没完成。

### 当前验证结果

- ReflectionTest 已构建并运行通过。
- MainEngine 已构建通过。
- ReflectionTest 的内存标签基线样例已经可直接看到：ReflectionMeta 分配增量 256 B、ReflectionCold 在当前预热后对 96 个字段/96 个方法的批量冷数据构造保持 alloc_delta=0、current_delta=0，同时页级统计显示 FieldColdData 为 1 页、113 槽每页、burst 后 tail_used=108，MethodColdData 为 1 页、341 槽每页、burst 后 tail_used=101；新增的冷区批量页保留测试还能直接看到 batch 会因为当前页剩余槽位不足而切到新页；注册计划测试还能直接验证 Transform 计划为 9 字段/5 方法且 reserve 容量完全命中，ETestEnum 计划为 4 个枚举项且 enumEntries reserve 命中；新增的 TypeBuilder 原地发射测试还能直接验证 Transform 字段、方法以及 ETestEnum 的 enumEntries 都是在预铺槽位上原地写入且计数精确命中；新增的已注册类型冷页局部性测试还能直接验证 Transform 的字段冷块和方法冷块各自都落在单一冷页；新增的 TypeRegistry arena 持有测试还能直接验证顺序注册的 probe 类型会进入有效 arena 页，并共享同一 ReflectionMeta arena 页；同时基础功能测试里 ETestEnum 的注册结果现在已经正确显示 4 个枚举项，TypeInfo 的名称查找路径也已切到冷区访问器，FindByNameFast("Transform") 与按 TypeId 查找已验证返回同一稳定地址，新增的 RegistryFallbackProbe 也已验证名字查找的 fallback index 路径会回到和按 id 查找相同的稳定 TypeInfo 指针；ReflectionString 分配增量 1024 B，ScriptBridgeTemp 分配增量 512 B。
- 当前已知构建输出中的 wmic 缺失、Draco PDB 缺失、LNK4098 等均为环境或既有警告，不是本轮改动引入的功能性回归。

### 当前量化基线说明

- 上面的尺寸数字以当前本地 MSVC Debug 构建下的 ReflectionTest 输出为准，不应手工推断或跨编译器硬套。
- 如果后续继续调整成员顺序、容器类型或所有权模型，应先重新运行 ReflectionTest，再更新本节，避免文档中的尺寸预算与真实 ABI 漂移。
- 当前真正已打通并被测试验证的 tag 路由是 ReflectionMeta、ReflectionCold、ReflectionString、ScriptBridgeTemp；ReflectionTemp、EditorInspectorTemp 目前还是“标签已定义但尚未形成稳定流量”的状态。

### 当前代码约定

- 对字段名、方法名、UI schema、扩展 metadata 的读取，不应再直接访问旧成员，而应统一通过 GetNameView、GetUISchema、GetMetadata、GetMeta、GetDisplayNameView、GetCategoryView、GetEditConditionView、HasRange 等访问器。
- TypeInfo::BuildLookup 现在依赖 nameHash 作为热路径哈希数据，碰撞确认阶段仍比较 GetNameView。
- ScriptSystem 侧的字段读写路径已切到 ScriptView 同款 ScratchBuffer，超出栈缓冲时会进入 ScriptBridgeTemp；当前仍需继续复核的主要是其它脚本桥接临时对象是否还残留在局部堆分配路径里。
- 如果继续做消费侧适配，默认原则应保持为“消费路径只读查询，构建路径集中注册”，不要重新引入 UI 或脚本路径内的隐式 Register。

## 当前系统问题清单

## 反射系统

### 1. 归属关系表达不够轻量

当前字段、方法与所属类型之间的关系主要依赖外层 TypeInfo 容器和常规指针语义，没有像 FFieldVariant 那样提供统一、紧凑、可标记的句柄表达。这会限制后续做嵌套属性、容器子字段、参数节点和反射树结构时的扩展效率。

### 2. 元数据容器过于通用

metadata 目前使用 vector<pair<MetadataKey, MetadataValue>>。该方案灵活，但对高频反射描述对象来说空间效率偏低，且会引入额外动态分配和缓存不友好访问。

### 3. 运行时注册路径偏重

当前宏注册流程以静态初始化构建 TypeInfo、再注册进 TypeRegistry 为主。短期可用，但在模块边界、初始化顺序、重复注册检测、热重载和未来增量构建场景下扩展性有限。

### 4. 描述对象热区和冷区混杂

查找字段、方法调用、脚本桥接真正依赖的只是哈希、偏移、flags、typeId、访问器和少量缓存。但当前对象同时承载了名称、显示名、元数据、UI schema 等冷信息，导致热路径对象体积偏大。

### 5. 反射临时内存策略不统一

ScriptView 已经有 ScratchBuffer，但它还不是完整的反射临时内存策略。脚本桥接、反射注册期字符串与元数据、检查器运行时临时对象仍未统一收敛到专用内存域。

## 内存系统

### 1. 通用入口与专用场景没有分层

当前 Memory + memory_backend 模型适合作为统一入口，但没有针对“反射元对象”“脚本临时对象”“长生命周期小对象”建立专门分配路径。

### 2. 已有对齐保证，但没有上升为 tagged pointer 能力

底层分配器默认最小对齐足以支持低位标记，但系统层没有提供统一的标记句柄抽象、对齐断言和调试校验。

### 3. ObjectPool 更偏通用对象池，而非元数据池

当前对象池带 liveObjects_ 跟踪和互斥保护，适合通用场景，不适合“注册期批量构建、整体回收、极少单独释放”的反射元数据对象。

### 4. 缺少面向布局的可观测性

当前统计主要关注 tag 维度的字节、次数和 frame spikes，但缺少对象尺寸预算、缓存行跨越、bin 命中率、反射启动期分配次数等布局级观测能力。

## 改进任务

## 可执行拆解（带代码落点）

### Issue 01：建立反射对象尺寸与布局基线

改动入口：

- [src/EngineCore/reflection/ReflectionCore.h#L311](src/EngineCore/reflection/ReflectionCore.h#L311) 当前 FieldInfo 定义。
- [src/EngineCore/reflection/ReflectionCore.h#L407](src/EngineCore/reflection/ReflectionCore.h#L407) 当前 MethodInfo 定义。
- [src/EngineCore/reflection/ReflectionCore.h#L445](src/EngineCore/reflection/ReflectionCore.h#L445) 当前 TypeInfo 定义。
- [src/EngineCore/reflection/ReflectionCore.h#L191](src/EngineCore/reflection/ReflectionCore.h#L191) 当前 MetadataValue 定义。
- [src/EngineCore/reflection/Views/ScriptView.h#L35](src/EngineCore/reflection/Views/ScriptView.h#L35) 当前 ScratchBuffer。

执行内容：

- 为 TypeInfo、FieldInfo、MethodInfo、MetadataValue、ScratchBuffer 建立 sizeof、alignof、成员偏移输出。
- 增加缓存行审计辅助，先确认 FieldInfo 和 MethodInfo 是否已经跨单缓存线。
- 输出当前对象尺寸报告，作为后续重构前基线。

完成标准：

- 能在调试输出或测试中直接看到上述结构的尺寸与布局。
- 基线结果能区分热路径结构和冷路径结构。

### Issue 02：给反射系统引入统一的 OwnerHandle

改动入口：

- [src/EngineCore/reflection/ReflectionCore.h#L311](src/EngineCore/reflection/ReflectionCore.h#L311) FieldInfo 需要新增 owner handle。
- [src/EngineCore/reflection/ReflectionCore.h#L407](src/EngineCore/reflection/ReflectionCore.h#L407) MethodInfo 需要新增 owner handle。
- [src/EngineCore/reflection/ReflectionCore.h#L445](src/EngineCore/reflection/ReflectionCore.h#L445) TypeInfo 作为 owner 目标之一。

建议新增文件：

- src/EngineCore/reflection/Core/ReflectionOwnerHandle.h

执行内容：

- 实现基于 uintptr_t 的 tagged handle。
- 最低位或低两位编码 owner 类型。
- 提供 IsNull、IsType、IsField、AsType、AsField 等接口。
- 为句柄解码增加对齐和非法 tag 断言。

完成标准：

- 字段和方法都能用统一句柄表达 owner。
- 不引入额外对象头和额外虚表。

### Issue 03：把元数据高频项从通用 vector 里剥离出来

改动入口：

- [src/EngineCore/reflection/ReflectionCore.h#L191](src/EngineCore/reflection/ReflectionCore.h#L191) MetadataValue。
- [src/EngineCore/reflection/ReflectionCore.h#L192](src/EngineCore/reflection/ReflectionCore.h#L192) MetadataContainer。
- [src/EngineCore/reflection/ReflectionCore.h#L311](src/EngineCore/reflection/ReflectionCore.h#L311) FieldInfo 当前 metadata 持有方式。
- [src/EngineCore/reflection/ReflectionCore.h#L407](src/EngineCore/reflection/ReflectionCore.h#L407) MethodInfo 当前 metadata 持有方式。
- [src/EngineCore/reflection/Views/InspectorView.h#L41](src/EngineCore/reflection/Views/InspectorView.h#L41) EditCondition 依赖 metadata。
- [src/EngineCore/reflection/Views/InspectorView.h#L57](src/EngineCore/reflection/Views/InspectorView.h#L57) Category 依赖 metadata。

执行内容：

- 将 Category、DisplayName、Min、Max、EditCondition 从 metadata vector 中提升为固定字段或紧凑槽位。
- metadata vector 仅保留低频扩展项。
- 为 InspectorView 增加优先读取内建字段的路径。

完成标准：

- InspectorView 不再依赖 metadata 线性扫描获取高频字段。
- FieldInfo 热路径体积明显下降。

### Issue 04：拆分 FieldInfo 与 MethodInfo 的热区和冷区

改动入口：

- [src/EngineCore/reflection/ReflectionCore.h#L311](src/EngineCore/reflection/ReflectionCore.h#L311) FieldInfo。
- [src/EngineCore/reflection/ReflectionCore.h#L407](src/EngineCore/reflection/ReflectionCore.h#L407) MethodInfo。
- [src/EngineCore/reflection/Views/ScriptView.h#L126](src/EngineCore/reflection/Views/ScriptView.h#L126) 字段读路径。
- [src/EngineCore/reflection/Views/ScriptView.h#L158](src/EngineCore/reflection/Views/ScriptView.h#L158) 方法调用路径。
- [src/EngineCore/reflection/Views/InspectorView.h#L22](src/EngineCore/reflection/Views/InspectorView.h#L22) 检查器字段遍历路径。

执行内容：

- 把名字显示、UI schema、低频 metadata、编辑器展示信息移到冷区结构。
- 热区只保留哈希、偏移、flags、访问器、owner handle、调用缓存。
- 对 ScriptView 和 InspectorView 增加冷区访问适配。

完成标准：

- 顺序遍历字段时只接触热区数据。
- 脚本字段读写和方法调用路径不再被冷字段污染缓存。

### Issue 05：把反射注册从“零散堆对象”改成“图谱构建”

改动入口：

- [src/EngineCore/reflection/Core/ReflectionMacros.h#L29](src/EngineCore/reflection/Core/ReflectionMacros.h#L29) REFLECTION_STRUCT。
- [src/EngineCore/reflection/Core/ReflectionMacros.h#L64](src/EngineCore/reflection/Core/ReflectionMacros.h#L64) REFLECT_FIELD。
- [src/EngineCore/reflection/Core/ReflectionMacros.h#L78](src/EngineCore/reflection/Core/ReflectionMacros.h#L78) REFLECT_METHOD。
- [src/EngineCore/reflection/Core/ReflectionMacros.h#L88](src/EngineCore/reflection/Core/ReflectionMacros.h#L88) REFLECT_ENUM。
- [src/EngineCore/reflection/DSL/TypeBuilder.h#L69](src/EngineCore/reflection/DSL/TypeBuilder.h#L69) TypeBuilder 主体。
- [src/EngineCore/reflection/DSL/TypeBuilder.h#L132](src/EngineCore/reflection/DSL/TypeBuilder.h#L132) RegisterFieldFromDSL。
- [src/EngineCore/reflection/DSL/TypeBuilder.h#L211](src/EngineCore/reflection/DSL/TypeBuilder.h#L211) RegisterMethodFromDSL。
- [src/EngineCore/reflection/TypeRegistry.h#L34](src/EngineCore/reflection/TypeRegistry.h#L34) Register 入口。

执行内容：

- 新增 ReflectionArena 或 ReflectionGraphBuilder。
- TypeBuilder 不再主要依赖 vector 零散扩容，而是向连续内存块写入。
- 宏层保持原语义，上层调用尽量不改。

完成标准：

- 注册后的描述对象主要位于反射专用连续内存域。
- TypeRegistry 持有稳定视图，而不是零散 shared_ptr 图。

### Issue 06：重构 TypeRegistry 的持有模型

改动入口：

- [src/EngineCore/reflection/TypeRegistry.h#L25](src/EngineCore/reflection/TypeRegistry.h#L25) TypeRegistry 定义。
- [src/EngineCore/reflection/TypeRegistry.h#L34](src/EngineCore/reflection/TypeRegistry.h#L34) Register。
- [src/EngineCore/reflection/TypeRegistry.h#L66](src/EngineCore/reflection/TypeRegistry.h#L66) FindFast。
- [src/EngineCore/reflection/TypeRegistry.h#L72](src/EngineCore/reflection/TypeRegistry.h#L72) FindByNameFast。
- [src/EngineCore/reflection/ReflectionCore.h#L489](src/EngineCore/reflection/ReflectionCore.h#L489) BuildLookup。

执行内容：

- 将 shared_ptr 持有改造为 arena 稳定地址或句柄索引持有。
- 保留按 id 和按 name 的快速查找接口。
- 清理重复构建 lookup 和静态初始化耦合点。

完成标准：

- TypeRegistry 仍保持现有查询语义。
- 注册后对象地址稳定，不依赖 shared_ptr 扩散。

### Issue 07：为 ScriptView 建立专用临时内存路径

改动入口：

- [src/EngineCore/reflection/Views/ScriptView.h#L35](src/EngineCore/reflection/Views/ScriptView.h#L35) ScratchBuffer。
- [src/EngineCore/reflection/Views/ScriptView.h#L62](src/EngineCore/reflection/Views/ScriptView.h#L62) BuildMethodCallCache。
- [src/EngineCore/reflection/Views/ScriptView.h#L126](src/EngineCore/reflection/Views/ScriptView.h#L126) GetField。
- [src/EngineCore/reflection/Views/ScriptView.h#L137](src/EngineCore/reflection/Views/ScriptView.h#L137) SetField。
- [src/EngineCore/reflection/Views/ScriptView.h#L158](src/EngineCore/reflection/Views/ScriptView.h#L158) CallMethod。
- [src/script/ScriptSystem.cpp#L1470](src/script/ScriptSystem.cpp#L1470) JsReflectGetField。
- [src/script/ScriptSystem.cpp#L1513](src/script/ScriptSystem.cpp#L1513) JsReflectSetField。
- [src/script/ScriptSystem.cpp#L1556](src/script/ScriptSystem.cpp#L1556) JsReflectCallMethod。

执行内容：

- 将脚本调用期临时对象统一归入反射或脚本专用临时分配域。
- 减少 CallMethod 内部潜在堆分配。
- 为 ScriptSystem 的反射调用加上临时分配统计。

完成标准：

- 反射脚本调用路径的临时内存有独立 tag。
- 常见调用路径尽量不落到通用堆。

### Issue 08：为 InspectorView 和 PropertyDrawer 建立热路径适配

改动入口：

- [src/EngineCore/reflection/Views/InspectorView.h#L22](src/EngineCore/reflection/Views/InspectorView.h#L22) InspectorView。
- [src/EngineCore/reflection/Views/InspectorView.h#L41](src/EngineCore/reflection/Views/InspectorView.h#L41) IsVisible。
- [src/editor/util/InspectorBuilder.cpp#L8](src/editor/util/InspectorBuilder.cpp#L8) DrawInspector。
- [src/editor/util/PropertyDrawer.cpp#L356](src/editor/util/PropertyDrawer.cpp#L356) DrawField。
- [src/editor/util/PropertyDrawer.cpp#L122](src/editor/util/PropertyDrawer.cpp#L122) field.typeId 查询。
- [src/editor/util/StaticInspector.h#L308](src/editor/util/StaticInspector.h#L308) FunctionSelector 分支。
- [src/editor/util/StaticInspector.h#L324](src/editor/util/StaticInspector.h#L324) 临时 FindFast。
- [src/editor/util/StaticInspector.h#L328](src/editor/util/StaticInspector.h#L328) 临时 Register。

执行内容：

- 让检查器优先消费热区字段。
- 清理 FunctionSelector 路径里的临时注册行为，避免 UI 路径偷偷改 TypeRegistry。
- 将高频 inspector 查询改为稳定图谱只读访问。

完成标准：

- 绘制 inspector 时不再依赖运行中临时补注册。
- UI 路径不再承担反射图谱构建职责。

### Issue 09：把 MemoryTag 细化到反射热数据、冷数据和临时数据

改动入口：

- [src/memory/memory.ixx#L50](src/memory/memory.ixx#L50) MemoryTag 定义。
- [src/memory/memory.cpp#L170](src/memory/memory.cpp#L170) Memory::Alloc。
- [src/memory/memory.cpp#L212](src/memory/memory.cpp#L212) Memory::Free。
- [src/memory/memory.cpp#L243](src/memory/memory.cpp#L243) Memory::Realloc。
- [src/memory/memory.cpp#L393](src/memory/memory.cpp#L393) DumpAllocatorStats。
- [src/memory/memory.cpp#L415](src/memory/memory.cpp#L415) FlushAllThreadStats。

执行内容：

- 在 Reflection 和 Script 之下继续细分子标签。
- 让反射 arena、反射冷数据、脚本桥接临时数据分开统计。
- 更新统计输出，能独立看到这些子路径的 current、peak、alloc_count。

完成标准：

- 不同反射内存流量可以独立观察。
- 后续优化有明确量化依据。

### Issue 10：为底层分配器正式提供 tagged pointer 约束支持

改动入口：

- [src/memory/ue_binned2_port.cpp#L32](src/memory/ue_binned2_port.cpp#L32) kMinAlign。
- [src/memory/ue_binned2_port.cpp#L59](src/memory/ue_binned2_port.cpp#L59) AlignHeader。
- [src/memory/ue_binned2_port.cpp#L66](src/memory/ue_binned2_port.cpp#L66) LargeAllocHeader。
- [src/memory/ue_binned2_port.cpp#L84](src/memory/ue_binned2_port.cpp#L84) Alloc。
- [src/memory/ue_binned2_port.cpp#L111](src/memory/ue_binned2_port.cpp#L111) Free。
- [src/memory/ue_binned2_port.cpp#L171](src/memory/ue_binned2_port.cpp#L171) ValidateHeap。
- [src/memory/ue_binned2_port.cpp#L201](src/memory/ue_binned2_port.cpp#L201) DumpAllocatorStats。

执行内容：

- 把“最小 16 字节对齐可支持低位借位”从隐含事实变成显式约束。
- 增加 tagged pointer 使用前提断言和调试说明。
- 输出每个 bin 的使用情况和线程缓存命中情况，为后续反射小对象池提供依据。

完成标准：

- tagged pointer 的使用前提在底层有文档和断言支撑。
- 分配器统计可指导反射专用池尺寸设计。

### Issue 11：把当前 ObjectPool 分成“通用池”和“元数据池”两条线

改动入口：

- [src/memory/object_pool.h#L18](src/memory/object_pool.h#L18) ObjectPool。
- [src/memory/object_pool.h#L29](src/memory/object_pool.h#L29) create。
- [src/memory/object_pool.h#L59](src/memory/object_pool.h#L59) destroy。
- [src/memory/object_pool.h#L117](src/memory/object_pool.h#L117) allocateBlock。
- [src/memory/object_pool.h#L132](src/memory/object_pool.h#L132) clear。
- [src/memory/object_pool.h#L152](src/memory/object_pool.h#L152) liveObjects_。

执行内容：

- 保留当前 ObjectPool 用于需要单独销毁和调试跟踪的对象。
- 新增 MetadataPool 或 ArenaPool，去掉 liveObjects_ 和逐对象销毁成本。
- 让反射冷数据、枚举条目、扩展元数据优先走元数据池。

完成标准：

- 反射元数据对象不再依赖带哈希跟踪的通用池。
- 池化模型与对象生命周期匹配。

### Issue 12：清理 UI 和脚本路径中的“运行时补注册”行为

改动入口：

- [src/editor/util/StaticInspector.h#L324](src/editor/util/StaticInspector.h#L324) FindFast。
- [src/editor/util/StaticInspector.h#L328](src/editor/util/StaticInspector.h#L328) Register。
- [src/script/ScriptSystem.cpp#L1487](src/script/ScriptSystem.cpp#L1487) FindByNameFast。
- [src/script/ScriptSystem.cpp#L1530](src/script/ScriptSystem.cpp#L1530) FindByNameFast。
- [src/script/ScriptSystem.cpp#L1583](src/script/ScriptSystem.cpp#L1583) FindByNameFast。

执行内容：

- 禁止 UI 绘制路径内隐式注册类型。
- 脚本路径保持只读反射访问，不承担构建职责。
- 将反射图谱缺失视为启动阶段或注册阶段错误，而不是消费阶段动态补洞。

完成标准：

- 反射构建与反射消费边界清晰。
- UI 和脚本路径变成稳定的只读查询链。

## 推荐执行顺序

1. 先做 Issue 01、Issue 09、Issue 10，拿到基线和底层约束。
2. 再做 Issue 02、Issue 03、Issue 04，压缩反射对象模型。
3. 然后做 Issue 05、Issue 06，重构注册与持有方式。
4. 最后做 Issue 07、Issue 08、Issue 11、Issue 12，清理消费链和专用池。

## 第一部分：建立对象尺寸、对齐和分配基线

### 任务 1：建立反射对象尺寸报告

输出以下对象的 sizeof、alignof、字段排列和缓存行跨越情况：

- TypeInfo
- FieldInfo
- MethodInfo
- MetadataValue
- EnumEntry
- ScriptValue
- ScriptView 调用缓存结构

### 任务 2：建立反射启动期分配基线

统计反射系统初始化时：

- TypeInfo 注册总数
- FieldInfo/MethodInfo/metadata 分配次数
- 反射字符串分配次数
- 通用 Memory 路径命中次数
- UE Binned2 小块池命中分布

### 任务 3：建立临时内存基线

统计以下路径上的临时分配：

- ScriptView 字段读写
- ScriptView 方法调用
- InspectorView 字段遍历与绘制辅助
- 反射错误信息构造

### 任务 4：给关键对象设立尺寸预算

为后续重构建立明确约束：

- TypeInfo 保持紧凑，避免无限扩张。
- FieldInfo 控制在单缓存线附近。
- MethodInfo 仅保留热路径必要字段。
- metadata 常用项不允许继续无限制堆积在通用 vector 中。

## 第二部分：设计并落地反射归属标记句柄

### 任务 5：引入 ReflectionOwnerHandle

新增一个统一句柄类型，底层使用 uintptr_t 存储，低位作为类型标记。

建议至少支持：

- Null
- TypeInfo*
- FieldInfo*

如果后续会加入参数节点、容器子节点或属性链节点，可以预留额外标记位。

### 任务 6：为 tagged pointer 建立安全边界

新增静态断言和运行时断言：

- 被句柄持有的对象必须满足最小对齐要求。
- Debug 模式下验证标记位合法性。
- 解码后地址必须满足预期对齐。
- 空指针、错误 tag、错误解码都要有明确诊断。

### 任务 7：先在反射 owner 关系中试点替换

先替换字段和方法归属表达，不要一开始把 tagged pointer 扩散到所有系统对象。优先收敛反射内部 owner 链路，降低调试风险。

## 第三部分：重构 FieldInfo 与 MethodInfo 为热冷分离布局

### 任务 8：拆分 FieldInfo 热区与冷区

FieldInfo 热区建议保留：

- nameHash
- typeId
- offset
- size
- alignment
- flags
- getterFn
- setterFn
- equalsFn
- copyFn
- ownerHandle
- containerType
- containerTrait 引用

FieldInfo 冷区建议外置：

- 原始显示名称
- Category
- EditCondition
- UI schema
- 低频元数据
- 调试字符串

### 任务 9：拆分 MethodInfo 热区与冷区

MethodInfo 热区建议保留：

- nameHash
- returnType
- paramTypes
- invokeFn
- flags
- ownerHandle
- 调用缓存

MethodInfo 冷区建议外置：

- 展示名称
- 编辑器可见信息
- 低频元数据
- 调试和诊断信息

### 任务 10：保留兼容层

在 InspectorView、ScriptView、TypeRegistry 的调用层面提供过渡适配，避免一次性重写所有消费代码。

## 第四部分：重构元数据系统

### 任务 11：将高频元数据内建化

以下元数据不应继续统一走通用 metadata vector：

- Category
- DisplayName
- Min
- Max
- EditCondition

这些信息应提升为专门字段或紧凑固定槽位。

### 任务 12：扩展元数据改为紧凑存储

低频元数据继续允许扩展，但应改为：

- 小型内联数组
- slab/arena 中的紧凑条目块
- 只读注册后冻结的数据块

避免每个字段各自持有独立 vector 的额外容量和分配成本。

### 任务 13：规范 metadata value 类型边界

高频元数据类型应尽量固定，减少 variant 参与热路径判断。字符串类元数据优先改为 STextView 指向稳定存储，避免重复堆分配。

## 第五部分：将反射注册改造成图谱构建流程

### 任务 14：引入 ReflectionArena

增加反射专用 arena 或 slab allocator，负责承载：

- TypeInfo 图谱
- 字段数组
- 方法数组
- 枚举条目
- 冷数据块
- 反射名称存储
- 扩展元数据块

### 任务 15：Builder 输出改为写入 arena

当前 TypeBuilder 构建 TypeInfo 的过程需要从“堆上零散对象拼装”转成“向反射图谱连续写入”。

### 任务 16：TypeRegistry 持有稳定图谱视图

TypeRegistry 不再默认依赖共享堆对象，而是持有 arena 中稳定对象的索引或指针视图，为后续热重载、批量销毁和重复注册处理留出空间。

### 任务 17：保留现有宏语义

REFLECTION_STRUCT、REFLECT_FIELD、REFLECT_METHOD 的上层用法尽量保持不变，将改动收敛在 builder、registry 和底层图谱构建层。

## 第六部分：为反射场景扩展底层内存系统

### 任务 18：细化 MemoryTag

在现有 tag 基础上新增更细粒度分类，例如：

- ReflectionMeta
- ReflectionTemp
- ReflectionString
- ScriptBridgeTemp
- EditorInspectorTemp

让反射与脚本相关内存从当前笼统的 Reflection、Script 标签中继续分离。

### 任务 19：新增反射专用分配器

针对反射图谱构建和只读描述对象，新增以下至少一种分配模式：

- 线性 arena
- 页块链表 arena
- 小对象 slab

不建议直接复用当前带 live tracking 的通用 ObjectPool。

### 任务 20：调整 ObjectPool 定位

保留现有 ObjectPool 作为通用池，但新增更轻量的无跟踪版本或批量回收版本，用于：

- 反射冷数据
- 编译期生成的描述对象批量装配
- 编辑器临时对象

### 任务 21：重新审视 AllocationHeader 成本

当前通用分配路径会为每个块额外携带头部。对反射元对象这种数量大、尺寸小、生命周期稳定的对象来说，这部分成本很高。需要为 arena 或 slab 路径设计绕过通用 header 的方案。

### 任务 22：把对齐保证正式化

为底层分配器补充：

- 最小对齐约束说明
- tagged pointer 使用前提说明
- 调试期对齐断言
- 低位借用安全边界说明

## 第七部分：围绕缓存行和局部性做布局优化

### 任务 23：连续存放字段与方法描述

字段数组、方法数组、枚举条目应优先连续存储，避免单个描述对象零散分散在不同页和不同 bin 上。

### 任务 24：优化结构成员顺序

针对以下结构做成员顺序重排，减少 padding 并提升热点字段聚集：

- TypeInfo
- FieldInfo 热区
- MethodInfo 热区
- 元数据条目结构
- Script 调用缓存结构

### 任务 25：建立缓存行审计工具

为热点结构新增布局审计输出，至少能回答以下问题：

- 一个 FieldInfo 是否跨多个缓存行。
- 顺序遍历字段时热点字段是否集中。
- metadata 冷数据是否污染热路径缓存。

### 任务 26：引入名称稳定存储策略

对于类型名、字段名、方法名、显示名，考虑引入集中存储策略，减少重复字符串对象与重复分配。

## 第八部分：统一脚本桥接和检查器的临时内存模型

### 任务 27：ScriptView 统一走反射临时内存域

ScratchBuffer 只是局部优化。需要把：

- 参数转换
- 返回值临时区
- 非 POD 构造/析构缓存

统一收敛到反射或脚本专用临时内存策略。

### 任务 28：InspectorView/PropertyDrawer 临时对象降堆分配

检查器遍历和 UI 绘制中涉及的字符串适配、临时容器和元数据查询结果，应尽量使用稳定视图、栈内缓存或帧级临时 arena，减少小块堆分配。

### 任务 29：统一临时对象标签与统计

反射、脚本、编辑器临时内存要能在统计上被明确区分出来，而不是全部混在通用 Reflection 或 Script tag 里。

## 第九部分：验证与验收

### 任务 30：正确性验证

确保以下行为不回退：

- 类型注册
- 字段查找
- 方法查找
- 字段读写
- 脚本调用
- Inspector 显示与编辑

### 任务 31：内存安全验证

确保以下条件成立：

- tagged handle 不产生错误解码
- arena 生命周期受控，无悬空视图
- ValidateHeap 稳定通过
- 反射冷数据、热数据、临时数据的回收边界清晰

### 任务 32：性能验证

至少评估以下指标：

- 反射初始化分配次数是否明显下降
- 反射描述对象总占用是否明显下降
- ScriptView 方法调用临时堆分配是否减少
- 字段顺序遍历是否更快
- 名字查找是否稳定或提升

### 任务 33：回归测试覆盖

补充覆盖以下场景：

- 普通结构体字段反射
- 枚举字段反射
- 容器字段反射
- 脚本字段读写
- 脚本方法调用
- 编辑器检查器显示
- 重复注册或非法 owner handle 诊断

## 实施优先级

建议按以下顺序执行：

1. 先建立尺寸、对齐和分配基线。
2. 先落地 ReflectionOwnerHandle，不先大改分配器。
3. 然后做 FieldInfo/MethodInfo 热冷分离。
4. 再改 metadata 存储模型。
5. 再把已经接到 TypeRegistry 持有层的 ReflectionArena 继续扩展到注册图谱化。
6. 最后补底层内存专用路径与缓存行优化。

原因是当前真正需要先解决的是“反射对象设计太重”，而不是“分配器算法不够复杂”。如果对象模型不先压缩，直接增强底层分配器收益会很有限。

## 未完成更新与下一窗口接手建议

### 仍未完成的主项

- Issue 05 已部分推进：ReflectionArena 已经落到 TypeRegistry 持有层，但还没有 ReflectionGraphBuilder，TypeBuilder 当前已经具备 dry-run 计数、热区预铺、原地发射和冷块直写，注册过程不再只是简单的 push_back 加常规对象回填。
- Issue 06 已完成第二阶段的持有切换：TypeRegistry 现在由 ReflectionMeta arena 统一持有 TypeInfo，types_ 只保留稳定槽位视图，idRegistry_ 和 nameRegistry_ 都只保存槽位索引；按名查找除了直接 Hash(typeName) 的快路径外，也已经验证了 fallback name index 会回到同一稳定 TypeInfo 指针，顺序注册的 probe 也会聚集到同一 arena 页。它仍然不是独立句柄表或冻结 registry 视图模型，但 shared_ptr / unique_ptr 过渡持有已经退出主路径。
- Issue 07 已部分推进：ScriptView::CallMethod 与 ScriptSystem 的 JsReflectGetField、JsReflectSetField 都已统一到 ScratchBuffer/ScriptBridgeTemp 路径，但仍未系统梳理所有脚本桥接热点，ReflectionTemp 也还没有形成独立稳定流量。
- Issue 08 未完成第二阶段：虽然 InspectorView、PropertyDrawer、StaticInspector 已适配冷数据访问器，但还没有系统性清理所有 editor/runtime 消费点的旧式 metadata 扫描和旧成员直接访问。
- Issue 09 仍有第二阶段：MemoryTag 枚举和统计输出已经齐备，ReflectionCold 已形成稳定、可复测且可复用的真实分配路径；ReflectionTemp、EditorInspectorTemp 仍停留在标签基础设施阶段。
- Issue 11 已部分推进：当前 ReflectionColdPool 已切到按类型冷数据页，FieldColdData/MethodColdData 不再逐对象直连底层分配，且同一 burst 会优先落入页内连续槽位；注册宏还会先做字段数/方法数/枚举项计数，再让 TypeBuilder 进入对应的批量页保留并预留热区 vector 容量。但还没有统一的 MetadataPool/ArenaPool，也还没有让 TypeBuilder 直接产出连续冷区块。
- Issue 12 未完全结束：StaticInspector 已不再补注册，但脚本路径虽然保持只读查询，仍需在整体注册图谱完成后再复核一次“消费期不补洞”的边界。

### 建议的新窗口起手顺序

1. 先补 Issue 11 的下一步：把当前“计数 builder + 页保留 + 热区 reserve”的 ReflectionCold 再推进到 builder 直接写块或冷区批量装配，减少字段/方法交错注册带来的页内空洞，并让同一 TypeInfo 的冷区更稳定地聚集。
2. 再补 Issue 09 第二阶段：为 ReflectionTemp、EditorInspectorTemp 各建立至少一条真实、可复测的调用路径，避免标签只停留在声明层。
3. 然后继续做 Issue 05 的第二阶段：把当前 TypeBuilder 从“预铺热区 + 冷块直写”推进到“连续图谱 + builder 直接装配”，让字段、方法、枚举、扩展 metadata 和名字存储都能在同一注册期图谱里闭合。这仍然是后续所有内存优化真正放大的关键。
4. 最后回头系统复核 Issue 07 和 Issue 12 的消费链边界，确认脚本/UI 路径已经完全收敛为只读查询，不再夹带构建或隐式补洞逻辑。

### 下一阶段落地建议

- 如果目标是尽快把“热冷分离”从对象模型扩展到真实内存布局，当前最先该做的不是再压 FieldInfo 字节数，而是把已经页化的 coldData 接到 builder 批量写入。当前热区已经压进预算，真正还没完全兑现的是“同一类型冷区一起生成、一起贴近放置”的局部性。
- MethodInfo 当前 192 B、hot_bytes 184，说明调用缓存仍然占据大头。后续如果继续优化 MethodInfo，应优先考虑把 callCache 从“始终常驻热对象”调整为“延迟构建 + 外置缓存块”或“共享缓存页”，而不是继续在 metadata 上做小修小补。
- Script 路径现在最明显的缺口不是 API 设计，而是临时内存模型还没有完全统一。只要脚本桥接里还残留独立 malloc/free 或其它绕开 ReflectionTemp / ScriptBridgeTemp 的分配路径，统计就无法真实反映脚本桥接成本。
- TypeRegistry 已经跨过 shared_ptr 和 unique_ptr 过渡持有模型，当前由 ReflectionMeta arena 提供稳定地址，types_ 只保留槽位视图，双 unordered_map 仍负责查询索引。这已经兑现了“注册表自身不再逐类型独立堆分配”的目标，但它还没有进一步压成冻结表、独立句柄层或完整 graph builder 输出；Issue 05 仍然是决定这轮改造是否真正完成的主轴，不应长期后置。

### 新窗口继续时优先检查的文件

- src/EngineCore/reflection/ReflectionCore.h
- src/EngineCore/reflection/DSL/TypeBuilder.h
- src/EngineCore/reflection/TypeRegistry.h
- src/EngineCore/reflection/Views/ScriptView.h
- src/script/ScriptSystem.cpp
- src/editor/util/StaticInspector.h
- dev/test/ReflectionTest/ReflectionTest.cpp

## 最终验收要求

### 功能层面

- 现有宏注册方式继续可用。
- TypeRegistry、InspectorView、ScriptView 公开语义不被破坏。
- tagged handle 仅作为内部优化，不向上层泄漏不必要复杂度。

### 内存层面

- 反射描述对象体积明显下降。
- 反射初始化期小块分配次数明显下降。
- 反射元对象不再大量依赖通用 header 分配路径。
- 临时对象分配路径清晰可观测。

### 性能层面

- 顺序遍历字段和方法时缓存局部性改善。
- ScriptView 调用临时分配减少。
- 反射启动阶段分配更集中、碎片更少。

### 调试层面

- tagged pointer 的对齐与标记合法性有明确断言。
- 关键结构的 sizeof/alignof 与预算可自动输出。
- 内存统计中能区分反射热数据、冷数据和临时数据。

## 结论

这次改造的核心方向可以概括为三句话：

1. 用标记指针压缩反射归属关系。
2. 用热冷分离压缩反射描述对象。
3. 用专用 arena 和连续布局把底层内存优势真正兑现出来。

只要这三条主线落地，当前反射系统和内存系统就会从“功能可用”进入“结构合理、布局可控、成本可量化”的状态。