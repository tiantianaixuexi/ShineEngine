# 反射系统接入编译期容器任务表

## 目标

当前仓库已经具备三块基础能力：

- `constexpr_vector` / `constexpr_str` 这类可用于编译期描述的基础容器。
- `ConstexprTypeInfo<T>` 这类旁路编译期反射描述入口。
- 正式运行时反射主链 `REFLECTION_STRUCT -> BuildTypeRegistrationGraph -> TypeBuilder -> TypeRegistry`。

真正缺的不是“有没有编译期容器”，而是“正式反射注册主链还没有把这些编译期容器当成主输入”。

本任务表的目标是把正式反射系统的注册信息尽量前移到编译期，减少注册期 staging、冻结和临时分配，同时保持现有 `TypeInfo` / `FieldInfo` / `MethodInfo` 的对外语义不变。

## 非目标

以下内容不属于本任务表的主目标，避免实施过程中再次跑偏：

- 不以新增 `try_*` 无异常访问接口为核心任务。
- 不把运行时字段查找微优化当成主收益来源。
- 不要求一次性全仓切换到新链路。
- 不要求立即改掉所有运行时 materialization 逻辑。

## 当前判断

## 当前进度

已落地的第一批原型：

1. 正式主链已经支持按类型注入静态 plan，`BuildTypeRegistrationGraph` 会优先尝试静态 provider，再回退到旧的 `Measure` 路径。
2. 已新增独立的 CTPlan 模型：`CTPlanView`、`CTFieldPlan`、`CTMethodPlan`、`CTEnumPlan`、`CTMetadataEntry`。
3. 已新增 `CTPlan -> TypeRegistrationPlan` 的桥接转换，built-in metadata 和 runtime extras 在转换阶段显式分层。
4. `Transform` 和 `ETestEnum` 测试类型已经切到静态 CT provider，验证了正式 `REFLECTION_STRUCT` 主链可以消费这类描述。
5. `ReflectionTest` 已增加断言，确认 built-in metadata 没有重新混进 `runtimeMetadataEntries`；`ReflectionPerfTest` 继续通过。

当前还没完成的部分：

1. 现在的正式运行时注册仍会 replay DSL 绑定 getter/setter/invoke thunk，`TypeBuilder` 还没有完全退化成“只消费 CTPlan”的纯 materialization。
2. 目前只有测试类型接入了 CT provider，尚未推广到更多正式类型。
3. `UI::Schema` 和 runtime-only metadata 的边界虽然已经能分层，但还没形成统一 authoring API。

### 已具备的条件

1. 编译期容器和编译期描述入口已经存在，但仍是旁路线。
2. 正式主链已经有完整的 plan、freeze、materialization 流程。
3. 运行时查找热路径已经较成熟，字段/方法名查找已有专用 lookup。

### 当前真正的缺口

1. `REFLECTION_STRUCT` 仍以运行时注册 lambda 为上游，先测量再构建 plan。
2. `TypeRegistrationGraph` 仍承担测量器角色，而不是静态描述持有器。
3. `TypeBuilder` 仍同时承担“计划构建回放”和“最终 materialization”两种职责。
4. 注册期收益没有独立基准，现有性能讨论容易被运行时 benchmark 混淆。

## 总优先级

1. 先定义编译期计划模型，并明确 constexpr-safe 与 runtime-only 元数据边界。
2. 再把正式 `REFLECTION_STRUCT` 主链切到编译期计划输入。
3. 然后让 `TypeBuilder` 退回到纯 materialization。
4. 最后做 mixed-mode 兼容、顺序问题处理和独立注册期基准。

## 任务拆解

| ID | 优先级 | 任务 | 主要文件 | 修改动作 | 预期收益 | 验证方式 |
|---|---|---|---|---|---|---|
| RCT1 | P0 | 定义 CTPlan 对齐层 | `src/EngineCore/reflection/DSL/TypeBuilder.h` `src/EngineCore/reflection/ReflectionCore.h` `src/EngineCore/reflection/Reflection.h` | 建立与 `FieldPlan` / `MethodPlan` / `EnumPlan` 一一对应的编译期描述模型，明确字段、方法、枚举、参数类型、内建 metadata 的静态表达方式 | 把目标数据模型先固定，避免后续接线反复返工 | 用一个真实测试类型覆盖字段、方法、枚举、参数和内建 metadata，不依赖 `ReflectionPlanBlock` |
| RCT2 | P0 | 拆分 constexpr-safe 元数据 | `src/EngineCore/reflection/DSL/TypeBuilder.h` | 把 `Category`、`DisplayName`、`EditCondition`、`Min`、`Max`、flags、参数类型、枚举标签等可静态化内容拆出来，把 runtime-only metadata 留在兜底通道 | 把大头计划信息前移到编译期，压缩注册期 staging 体积 | migrated 类型的核心 built-in metadata 不再经过 `runtimeMetadataEntries` |
| RCT3 | P0 | 统一正式宏入口 | `src/EngineCore/reflection/Core/ReflectionMacros.h` `src/EngineCore/reflection/Reflection.h` `src/EngineCore/reflection/DSL/TypeBuilder.h` | 让 `REFLECTION_STRUCT` / `REFLECT_FIELD` / `REFLECT_METHOD` 直接产出 CTPlan，旁路 `SHINE_REFLECT_CT_*` 只保留兼容层或被折叠 | 让编译期计划真正进入正式主链，避免两套声明方式长期并存 | 新接入类型使用正式宏后，不再依赖运行时 `Measure` 才能得到 plan |
| RCT4 | P0 | 让 TypeRegistrationGraph 退化为描述持有器 | `src/EngineCore/reflection/DSL/TypeBuilder.h` `src/EngineCore/reflection/Core/ReflectionMacros.h` | 保留外部调用形态，但让 `BuildTypeRegistrationGraph` 持有或转发静态 descriptor，不再在注册 lambda 内构造 staging plan | 以最小表面积切换主链，减少大规模 API 扰动 | migrated 类型的 graph 构建不再调用 `Measure`，也不再生成 `FieldPlanBlock` / `MethodPlanBlock` |
| RCT5 | P1 | 让 TypeBuilder 直接消费 CTPlan | `src/EngineCore/reflection/DSL/TypeBuilder.h` | 保留 `TypeInfo` 运行时形态，但把输入改成编译期 spans 或静态数组；`TypeBuilder` 只负责生成 `FieldInfo` / `MethodInfo` / `EnumEntry` | 启动阶段少一次 plan 构建与冻结，职责更清晰 | migrated 类型的 `BuildTypeInfo` 仅消费 CTPlan，不再通过 DSL 回放生成计划信息 |
| RCT6 | P1 | 让 cold data 直接从 CTPlan 发射 | `src/EngineCore/reflection/DSL/TypeBuilder.h` `src/EngineCore/reflection/ReflectionCore.h` | 字段名、方法名、参数类型、枚举标签、内建 metadata 尽量直接填充到 coldData，绕开中间 staging 容器 | 减少注册期分配、复制和冻结步骤，降低启动期内存抖动 | migrated 类型注册期间不再构造 `runtimeMetadataEntries` 或 `methodParamTypeEntries` staging block |
| RCT7 | P1 | 设计 mixed-mode 过渡期 | `src/EngineCore/reflection/Core/ReflectionMacros.h` `src/EngineCore/reflection/DSL/TypeBuilder.h` `src/EngineCore/reflection/TypeRegistry.h` | 允许旧链和 CTPlan 链按类型渐进共存，确保 `TypeRegistry` 不需要区分来源 | 降低切换风险，支持先迁核心类型 | 旧路径和新路径注册出的 `TypeInfo` 对外语义等价 |
| RCT8 | P1 | 处理跨 TU 顺序与依赖注册 | `src/EngineCore/reflection/Core/ReflectionMacros.h` `src/EngineCore/reflection/TypeRegistry.h` | 显式定义 enum、依赖类型、引用类型的注册顺序约束，避免静态初始化顺序问题被新链路放大 | 让“编译期 descriptor + 运行时注册”在多 TU 下保持可预测 | 跨类型依赖、枚举字段、`FindByNameFast` 与 `FindFast` 语义与现状一致 |
| RCT9 | P0 | 增加 TypeInfo 对等性测试 | `dev/test/ReflectionTest/ReflectionTest.cpp` `dev/test/common/reflection_test_fixture.h` | 对比旧链和 CTPlan 链最终产出的字段数、方法数、名字、flags、offset、参数类型、metadata、lookup 可用性 | 把“只换注册来源，不换运行时语义”变成硬约束 | 同一测试类型在新旧链下对外暴露的数据完全一致 |
| RCT10 | P1 | 拆分注册期与运行时基准 | `dev/test/ReflectionPerfTest/ReflectionPerfTest.cpp` | 保留现有字段查找/方法调用 benchmark，同时新增单类型注册、批量注册和注册期内存峰值统计 | 正确评估编译期容器接入收益，避免继续把启动期收益误判成运行时收益 | 至少能分别报告注册耗时、注册期临时分配量与现有 lookup/invoke 指标 |

## 分阶段执行建议

### 阶段 1：锁定编译期计划模型

先完成 `RCT1`、`RCT2`。

这一阶段决定“哪些东西真的能进编译期容器，哪些必须保留 runtime-only”，如果这里不先收口，后续主链接线一定会反复推倒。

### 阶段 2：切正式主链入口

完成 `RCT3`、`RCT4`。

这一阶段的目标不是立刻重写整个反射系统，而是让正式宏入口不再把运行时 `Measure` 当成唯一 plan 来源。

### 阶段 3：收缩 TypeBuilder 职责

完成 `RCT5`、`RCT6`。

这一阶段完成后，`TypeBuilder` 应当只负责把静态计划落成最终运行时描述，而不是继续兼任计划构建器。

### 阶段 4：处理兼容与验证

完成 `RCT7`、`RCT8`、`RCT9`、`RCT10`。

这一阶段决定这条路线能否安全落地，并且决定团队之后讨论性能时是否基于正确数据。

## 收益预期

### 会直接出现的收益

1. 减少注册期 `Measure` 计数遍历。
2. 减少 `ReflectionPlanBlock` staging 页和冻结步骤。
3. 减少注册期临时分配、复制和内存抖动。
4. 提高 descriptor 生成的确定性，降低双链路维护成本。

### 不一定直接出现在现有 ReflectionPerfTest 里的收益

1. 注册期耗时下降。
2. 注册期峰值内存下降。
3. 冷数据发射过程更稳定。
4. 静态初始化阶段中间对象更少。

### 只有运行时产物也变化时才可能出现的收益

1. 更紧凑的 hot layout。
2. 更少的间接层。
3. 更便宜的最终 lookup 表。
4. 更低的 cold data 跳转成本。

## 风险与约束

| 风险 | 说明 | 应对方式 |
|---|---|---|
| metadata 并非全部 constexpr-safe | `UI::Schema`、部分 metadata value、文本驻留策略未必能直接静态化 | 先做 built-in metadata 分层，runtime-only 走兜底通道 |
| 双路径过渡期过长 | 正式链和旁路线长期共存会放大测试矩阵和维护成本 | mixed-mode 只作为迁移期策略，明确下线旧链条件 |
| 跨 TU 顺序问题被放大 | 编译期 descriptor 接入后，静态初始化顺序问题仍可能存在 | 显式定义依赖顺序与回退策略，测试覆盖跨类型场景 |
| 收益评估继续失真 | 只看运行时 benchmark，会误判这条路线“没收益” | 增加独立注册期 benchmark 和临时分配统计 |
| TypeBuilder 改动面较大 | 该类当前同时承担计划构建和发射，拆分不当容易引入行为回归 | 先用 `TypeInfo` 对等性测试锁住外部语义 |

## 最小验收标准

1. 至少一个正式 `REFLECTION_STRUCT` 测试类型完成 CTPlan 接入，而不是旁路 demo。
2. 该类型不再依赖运行时 `Measure` 和 `ReflectionPlanBlock` 才能完成注册。
3. 该类型生成的 `TypeInfo` 在字段、方法、枚举、metadata 和 lookup 语义上与旧链完全对等。
4. 新增 benchmark 能分别报告注册期和运行时指标。
5. `InspectorView`、`ScriptView`、`ScriptSystem` 等运行时消费者无需感知 descriptor 来源变化。

## 建议首批迁移对象

建议先选一个字段、方法、metadata、enum 都比较完整，但体量仍可控的测试类型做第一批迁移，优先在测试程序里完成双链路对比，不要一上来直接切编辑器核心类型。

首批对象建议满足以下条件：

1. 有字段和方法混合注册。
2. 有内建 metadata。
3. 有枚举或枚举字段。
4. 已经有稳定测试覆盖。

## 与现有文档的关系

本文档负责“正式反射系统如何真正接入编译期容器”的迁移规划。

`doc/ConstexprReflectionFixPlan.md` 继续保留为基础设施修复记录，用于记录 `src/constexpr` 容器/字符串在反射场景下的语义修复与底层问题收敛。