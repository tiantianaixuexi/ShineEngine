# SString 优化任务表

## 目标

当前 SString 的主要优化方向不是继续深挖字符串类本体，而是先减少上层模块对 SString 的临时构造、重复路径转换和每帧字符串拼接。

本任务表按收益优先级排序，优先处理最可能产生真实分配和拷贝浪费的调用点。

## 总优先级

1. 资产注册表与依赖图的临时键构造
2. 基于 STextView 的透明查找
3. 路径字符串转换收敛
4. 资产浏览器每帧标签拼接
5. 脚本桥接里的键值字符串归一化
6. 字符串热点基准
7. SString 本体微调

## 任务拆解

| ID | 优先级 | 任务 | 主要文件 | 主要函数/区域 | 修改动作 | 验证方式 |
|---|---|---|---|---|---|---|
| T1 | P0 | 梳理资产表临时键 | src/editor/ShineAsset/registry/EditorAssetRegistry.cpp | Register / OnFileMoved / OnFileDeleted / TryDelete / Find / FindByPath / IsKnown / IsDangling / UpdatePathIndex | 统计所有 `SString(uuid)`、`SString(path.string())`、`SString(oldPath)` 这类查找前临时 owning key 的位置，形成替换清单 | 代码审查确认点位完整；后续替换后重新构建 MainEngine |
| T2 | P0 | 梳理依赖图临时键 | src/editor/ShineAsset/registry/AssetDependencyGraph.cpp | SetDependencies / RemoveAsset / GetDependencies / GetDependents / HasDependents / IsReachable | 标记所有 `m_forward.find(SString(...))`、`m_reverse.find(SString(...))`、`stack.push(SString(...))`、`dep == SString(target)` 场景，区分查找键与持久存储键 | 代码审查确认查找类临时对象与持久对象已分开 |
| T3 | P0 | 梳理运行时资产表临时键 | src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp | RegisterFactory / UnregisterFactory / Unregister / Find / Contains / RequestLoad | 标记所有 `find/erase/contains` 前的 `SString(typeId)`、`SString(uuid)` 临时构造点 | 替换前后行为一致；MainEngine 构建通过 |
| T4 | P1 | 支持透明字符串查找 | src/string/shine_string.h 及使用 `unordered_map<SString,...>` / `unordered_set<SString,...>` 的模块 | `std::hash<SString>` 附近；资产注册表、依赖图、运行时资产表 | 为 SString 增加支持 `STextView` 的透明哈希与透明比较器，允许容器使用 `find(STextView)`、`contains(STextView)`、`erase(STextView)` 风格访问，避免临时 owning key | 新增或补充测试；MainEngine 构建；相关调用点去掉临时 SString 后编译通过 |
| T5 | P1 | 替换资产表查找调用 | src/editor/ShineAsset/registry/EditorAssetRegistry.cpp | Find / TryDelete / IsKnown / IsDangling / FindByPath / UpdatePathIndex | 在透明查找就绪后，把所有纯查找/擦除路径改为直接使用 `STextView` 或稳定 view，不再构造临时 SString | 资产注册、移动、删除、查询流程手测通过 |
| T6 | P1 | 替换依赖图查找调用 | src/editor/ShineAsset/registry/AssetDependencyGraph.cpp | GetDependencies / GetDependents / HasDependents / IsReachable | 把纯查询路径切到 view 查找；只在真正需要持久保存到容器时构造 SString | 依赖图增删查与循环检测行为不变 |
| T7 | P1 | 替换运行时资产表查找调用 | src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp | Find / Contains / Unregister / RequestLoad | 用透明查找替代临时 owning key 构造；保留真正插入时的 SString 持久化 | 运行时资产注册和查询逻辑不变；构建通过 |
| T8 | P1 | 收敛路径字符串转换 | src/editor/ShineAsset/registry/EditorAssetRegistry.cpp | Register / OnFileMoved / OnFileDeleted / FindByPath | 明确 path 到内部 key 的单一路径，避免同一逻辑里反复 `filesystem::path -> std::string -> SString` | 代码审查确认同一函数内无重复路径字符串化 |
| T9 | P1 | 收敛脚本路径转换 | src/script/ScriptSystem.cpp | ComputeSourcePathFromJs / ComputeJsPathFromSource | 减少 `std::string` 中转和重复 substring 拼装，尽量统一成单次规范化后再转换为 SString | 脚本加载、热重载路径映射行为不变 |
| T10 | P2 | 缓存资产浏览器标签 | src/editor/browers/AssetsBorwer.cpp | 绘制 `.sasset` 树节点和子资产节点的区域 | 把 `stem + ## + fullpath`、`subName + ## + uuid` 这类每帧拼接改成缓存字段或预构建 label/id | 打开资产浏览器时行为一致；避免每帧重复分配 |
| T11 | P2 | 收敛属性面板字符串桥接 | src/editor/util/PropertyDrawer.cpp | `UI::None` 分支中的 SString 编辑、`UI::TextInput` 分支、FunctionSelector | 评估 `shine::SString(inputBuffer.data())` 是否可复用缓冲或减少中间对象；优先保证接口不变 | 属性编辑与 OnChange 行为不变 |
| T12 | P2 | 收敛脚本键值转换 | src/script/ScriptSystem.cpp | QuickJSScriptBridge::ToScriptField 中 map key 归一化逻辑 | 统一 key 转字符串路径，减少 `SString`/`std::string`/`STextView` 多次互转；必要时抽 helper | 脚本 map 转换结果一致；构建通过 |
| T13 | P3 | 建字符串热点基准 | 建议新增到 dev/test 或现有测试程序 | 资产查询、依赖图查询、脚本键值转换、资产浏览器标签构造 | 用可重复输入构建基准，比较优化前后分配次数和耗时，不凭感觉继续改 | 输出基准数字；用于决定是否继续动 SString 本体 |
| T14 | P4 | 微调 SString 本体 | src/string/shine_string.h | `_append_raw` / `_assign_from` / `replace_inplace` / reserve/grow 路径 | 只有在基准确认 SString 本体仍是热点时，再调整增长策略、短追加路径和重复扫描路径 | 基准回归对比；避免无收益重构 |

## 资产表临时键点位清单

这一节是对 T1、T2、T3 的进一步展开，目标是把“资产表临时键”拆到函数级和语句模式级，明确哪些点优先改，哪些点暂时保留。

### 标记规则

- Q：查询类临时键。理论上最适合改成透明查找。
- P：持久化键。用于插入、存储、长期保存在容器中，不应简单删除。
- C：路径转换临时值。重点关注 `filesystem::path -> std::string -> SString` 的重复中转。
- R：可延后。不是当前第一批必须动的点。

### A. EditorAssetRegistry 细点位

文件：[src/editor/ShineAsset/registry/EditorAssetRegistry.cpp](src/editor/ShineAsset/registry/EditorAssetRegistry.cpp)

| 函数 | 语句模式 | 类型 | 说明 | 建议动作 |
|---|---|---|---|---|
| Register | `SString uuid(record.uuid);` | P | `m_entries` 的主键，属于长期持久化值 | 保留 |
| Register | `SString pathStr(diskPath.string());` | C/P | 既有路径字符串化，又作为 `m_pathIndex` 和 entry 的持久值 | 先保留持久化语义，后续收敛成单一路径 key 构造 |
| Register | `m_entries.find(uuid)` | R | 这里不是临时键，已经复用本地持久值 | 不作为第一批优化重点 |
| OnFileMoved | `SString oldStr(oldPath.string());` | C/Q | 纯查找用旧路径 key | 透明查找后优先改 |
| OnFileMoved | `m_pathIndex.find(oldStr)` | Q | 纯查询路径 | 改成基于 view 的查找 |
| OnFileMoved | `SString uuid = pit->second;` | R | 这里是从 map 取出的持久 key 拷贝 | 不是第一批重点 |
| OnFileMoved | `SString newStr(newPath.string());` | C/P | 后续要写回 entry 和 path index | 保留持久化，但收敛转换路径 |
| OnFileDeleted | `SString pathStr(path.string());` | C/Q | 纯查询路径 | 透明查找后优先改 |
| OnFileDeleted | `m_pathIndex.find(pathStr)` | Q | 纯查询路径 | 改成基于 view 的查找 |
| OnFileDeleted | `SString uuid = pit->second;` | R | 从索引结果复制 UUID | 可延后 |
| TryDelete | `m_entries.find(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| Find | `m_entries.find(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| FindByPath | `m_pathIndex.find(SString(diskPath.string()))` | C/Q | 路径字符串化加临时 key 查找 | 第一批优先替换 |
| IsKnown | `m_entries.find(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| IsDangling | `m_entries.find(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| UpdatePathIndex | `m_pathIndex.erase(SString(oldPath))` | Q | 纯擦除 old key | 透明查找后优先改 |
| UpdatePathIndex | `m_pathIndex[SString(newPath)] = SString(uuid);` | P | 新 key 和 value 都会写入容器 | 保留持久化语义，只优化构造次数 |

### A-1. EditorAssetRegistry 第一批建议先改的点

1. `TryDelete`
2. `Find`
3. `FindByPath`
4. `IsKnown`
5. `IsDangling`
6. `UpdatePathIndex` 中的 `erase(oldPath)`
7. `OnFileMoved` 与 `OnFileDeleted` 的 `m_pathIndex.find(...)`

这些点的共同特征是：纯查找或纯擦除，当前都是为了访问容器而临时构造 SString。

### A-2. EditorAssetRegistry 暂时不先动的点

1. `Register` 里的 `uuid` 和 `pathStr`
2. `OnFileMoved` 里的 `newStr`
3. `UpdatePathIndex` 里写回容器的新 key/value

这些点本质上是持久化数据，不是为了查找而白白构造出来的对象。

### B. AssetDependencyGraph 细点位

文件：[src/editor/ShineAsset/registry/AssetDependencyGraph.cpp](src/editor/ShineAsset/registry/AssetDependencyGraph.cpp)

| 函数 | 语句模式 | 类型 | 说明 | 建议动作 |
|---|---|---|---|---|
| SetDependencies | `SString owner(ownerUuid);` | P | 后续作为 forward key 和 reverse value 使用 | 保留 |
| SetDependencies | `m_forward.find(owner)` | R | 使用的是已构造持久 key | 不作为第一批重点 |
| SetDependencies | `SString depKey(dep);` | P | 新依赖要写入 `m_forward` 和 `m_reverse` | 保留 |
| RemoveAsset | `SString owner(ownerUuid);` | P/Q | 既参与查找，也参与从 reverse set 擦除 | 当前可保留，等透明查找后再细分 |
| RemoveAsset | `m_forward.find(owner)` | R | 复用本地 key | 次优先级 |
| GetDependencies | `m_forward.find(SString(ownerUuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| GetDependents | `m_reverse.find(SString(targetUuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| HasDependents | `m_reverse.find(SString(targetUuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| IsReachable | `stack.push(SString(from))` | P | DFS 栈里需要拥有化副本 | 暂不动 |
| IsReachable | `if (dep == SString(target))` | Q | 循环内反复构造临时 SString | 第一批优先替换 |

### B-1. AssetDependencyGraph 第一批建议先改的点

1. `GetDependencies`
2. `GetDependents`
3. `HasDependents`
4. `IsReachable` 里的 `dep == SString(target)`

其中 `IsReachable` 的比较点尤其值得先改，因为它在 DFS 循环内部，重复成本比单次查询更差。

### B-2. AssetDependencyGraph 暂时不先动的点

1. `SetDependencies` 的 `owner` 和 `depKey`
2. `IsReachable` 的 `stack.push(SString(from))`

这些对象不是单纯为了查找而存在，至少当前仍然需要拥有化存储。

### C. RuntimeAssetRegistry 细点位

文件：[src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp](src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp)

| 函数 | 语句模式 | 类型 | 说明 | 建议动作 |
|---|---|---|---|---|
| RegisterFactory | `m_factories[SString(typeId)] = ...` | P | 工厂表插入 key | 保留持久化语义 |
| UnregisterFactory | `m_factories.erase(SString(typeId))` | Q | 擦除前临时 key | 第一批优先替换 |
| Register | `SString key(asset->GetUUID());` | P | 资产表插入 key | 保留 |
| Unregister | `m_assets.erase(SString(uuid))` | Q | 擦除前临时 key | 第一批优先替换 |
| Find | `m_assets.find(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| Contains | `m_assets.contains(SString(uuid))` | Q | 典型临时 owning key 查找 | 第一批优先替换 |
| RequestLoad | `SString key(uuid);` | P/Q | 这里先查找后可能插入同一个 key | 需要分两段看，先不要机械替换 |
| RequestLoad | `m_factories.find(SString(typeId))` | Q | 工厂表查找临时 key | 第一批优先替换 |

### C-1. RuntimeAssetRegistry 第一批建议先改的点

1. `UnregisterFactory`
2. `Unregister`
3. `Find`
4. `Contains`
5. `RequestLoad` 里的 `m_factories.find(SString(typeId))`

### C-2. RuntimeAssetRegistry 需要单独处理的点

`RequestLoad` 里的 `SString key(uuid)` 不能直接删。因为这里既要查找现有资产，又可能把同一个 key 写入 `m_assets.emplace(key, placeholder)`。更合理的改法是：

1. 先用 view 做查找。
2. 只有在确认要插入时，再构造一次持久 SString key。

## 第一批实施清单

如果只做第一轮最有收益、最不容易出错的改动，建议按下面顺序执行：

1. [src/editor/ShineAsset/registry/EditorAssetRegistry.cpp](src/editor/ShineAsset/registry/EditorAssetRegistry.cpp)
	先改 `Find`、`TryDelete`、`FindByPath`、`IsKnown`、`IsDangling`、`UpdatePathIndex::erase`。
2. [src/editor/ShineAsset/registry/AssetDependencyGraph.cpp](src/editor/ShineAsset/registry/AssetDependencyGraph.cpp)
	再改 `GetDependencies`、`GetDependents`、`HasDependents`、`IsReachable` 的目标比较。
3. [src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp](src/editor/ShineAsset/core/RuntimeAssetRegistry.cpp)
	最后改 `Find`、`Contains`、`Unregister`、`UnregisterFactory`、`RequestLoad` 的工厂查找。

## 第一批改完后的验证

1. 重新构建 MainEngine。
2. 手测资产查询、删除、移动、依赖查询是否仍然正常。
3. 确认没有因为透明查找破坏现有 `unordered_map<SString,...>` 的插入与持久化语义。
4. 在进入路径字符串收敛前，再回头确认是否还有新的临时 key 漏网点。

## 推荐执行顺序

### 阶段 1：先查清热点调用点

1. 完成 T1、T2、T3。
2. 输出一份“临时键替换清单”，确认哪些点只是查询，哪些点需要真正持久化 key。

### 阶段 2：先优化容器访问方式

1. 完成 T4。
2. 接着完成 T5、T6、T7。

这是最可能立刻减少 SString 临时构造的一组改动。

### 阶段 3：再处理高频字符串拼接

1. 完成 T8、T9。
2. 完成 T10、T11、T12。

这一步主要针对编辑器每帧路径和脚本桥接的字符串中转。

### 阶段 4：最后再决定是否动 SString 本体

1. 完成 T13。
2. 只有基准证明仍然是字符串类本体热点时，再做 T14。

## 进入实现前的检查项

1. 不要把持久 key 与查询 key 混在一起改。
2. 不要为了透明查找去破坏现有 `unordered_map<SString,...>` 存储语义。
3. 先改查找路径，再谈 intern 或额外池化。
4. 路径相关修改要保持 Windows 路径行为一致。
5. 所有改动后至少重建 MainEngine；如涉及资产系统行为，补一次编辑器侧手测。

## 预期结果

完成 T1 到 T12 后，预期收益应主要体现在：

1. 资产系统的哈希表访问不再为查询频繁构造 SString。
2. 编辑器资产浏览器减少每帧标签拼接分配。
3. 脚本系统减少路径和键值桥接中的重复字符串中转。
4. 是否继续优化 SString 本体可以由基准决定，而不是凭经验猜测。