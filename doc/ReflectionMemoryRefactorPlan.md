# 反射内存重构状态

## 当前状态

本轮主线已经闭合，核心方向已全部落地：

1. owner 关系已改为 ReflectionOwnerHandle。
2. FieldInfo 与 MethodInfo 已完成热冷分离。
3. TypeInfo、字段、方法、查找索引和冷数据已接入专用内存路径与可观测性。

当前已完成的关键点：

- Category、DisplayName、Min、Max、EditCondition 已内建化。
- TypeInfo / FieldInfo / MethodInfo / EnumEntry 名称与内建 metadata 文本统一走 ReflectionString。
- MethodInfo 的参数类型已从热区移到 MethodColdData。
- TypeInfo 的 fields、methods、fieldLookup_、methodLookup_ 已从 std::vector 切到 ReflectionColdVector。
- TypeInfo 消费侧已统一经由 GetFields / GetMethods / GetFieldAt / GetMethodAt 做只读访问，FieldInfo / MethodInfo 的 coldData 已重新收回私有边界，仅保留测试与布局探针辅助入口。
- TypeRegistrationGraph / TypeRegistrationPlan / TypeBuilder 已形成“先测量、再冻结、后发射、最后 replay 绑定”的稳定路径。
- 冷区发射已补齐批量装配路径，runtime metadata 与方法参数类型在 emit 阶段按已知大小一次性写入 ReflectionColdVector，继续优先保证页内连续性与局部性。
- TypeRegistry 已使用 ReflectionMeta arena 持有稳定 TypeInfo 地址。
- ScriptSystem 临时缓冲、StaticInspector、InspectorBuilder、PropertyDrawer 主链都已切到当前 runtime 访问模型。

## 量化基线

以下数字以本地 MSVC Debug 下 ReflectionTest 为准：

- TypeInfo: 144 B
- TypeColdData: 40 B
- FieldInfo: 128 B
- MethodInfo: 64 B
- FieldColdData: 136 B
- MethodColdData: 64 B
- ReflectionMetadataStorage: 24 B
- MethodCallParamStorage: 56 B
- MethodCallCache: 96 B
- MetadataValue: 40 B
- EnumEntry: 24 B
- ScriptValue: 40 B
- ScratchBuffer: 144 B

热区跨度：

- TypeInfo hot_bytes=80
- FieldInfo hot_bytes=120
- MethodInfo hot_bytes=56

当前已验证的 tagged pointer 前提：

- min_align=16
- usable_low_bits=4

当前已验证的内存标签流量：

- ReflectionMeta
- ReflectionCold
- ReflectionString
- ReflectionTemp
- ScriptBridgeTemp
- EditorInspectorTemp

当前验证结论：

- ReflectionTest 已通过 .\build.bat test ReflectionTest --no-pause。
- MainEngine 已通过 .\build.bat exe MainEngine --no-pause。
- 已知 `wmic` 缺失、Draco PDB 缺失、LNK4098 为环境或既有警告，不属于本轮回归。

## 代码约定

- 字段名、方法名、UI schema、metadata、方法参数类型统一通过访问器读取，不再直接访问旧成员。
- 消费路径默认只读查询，构建路径集中注册，不在 UI 或脚本路径内重新引入 Register。
- DSL staging 使用 DSLMetadataStorage；runtime metadata 使用 ReflectionMetadataStorage。
- ScriptSystem 的反射字段读写继续走 ScratchBuffer；超栈进入 ScriptBridgeTemp。
- editor 字符串输入缓冲继续走 EditorInspectorTemp。

推荐继续使用的访问器：

- GetNameView
- GetFields
- GetMethods
- GetFieldAt
- GetMethodAt
- GetUISchema
- GetMetadata
- GetMeta
- GetDisplayNameView
- GetCategoryView
- GetEditConditionView
- HasRange
- GetParamTypes
- GetParamCount
- GetParamType

## 剩余事项

当前无未闭合的 P0。

仍建议继续关注的事项：

1. 守住当前只读访问边界，新增 runtime 消费代码继续通过 TypeInfo / FieldInfo / MethodInfo 访问器读取，不回退到内部存储名或测试辅助接口。
2. 如继续做 editor 体验补完，优先处理 PropertyDrawer 中仍未实现的 UI schema：ColorPicker、Dropdown、FilePicker、VectorEditor、MatrixEditor。
3. 如继续做内存优化，继续沿“批量装配、页级复用、局部性优先”的方向推进，优先关注跨类型批次与更高层级的页复用，不优先追求表面字节数继续压缩。

## 关键入口

- src/EngineCore/reflection/ReflectionCore.h
- src/EngineCore/reflection/DSL/TypeBuilder.h
- src/EngineCore/reflection/TypeRegistry.h
- src/EngineCore/reflection/Views/ScriptView.h
- src/script/ScriptSystem.cpp
- src/editor/util/StaticInspector.h
- src/editor/util/PropertyDrawer.cpp
- dev/test/ReflectionTest/ReflectionTest.cpp