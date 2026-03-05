# AssetManager 与 TextureManager 对接方案

## 设计原则

- AssetManager 作为资产上层，仅暴露资产抽象句柄与资源接口。
- TextureManager 作为渲染资源下层，仅处理 GPU 纹理创建、更新、释放。
- 下层不依赖上层：TextureManager 不再依赖 AssetManager，不再包含资产类型头文件。
- 上层可调用下层：AssetManager 在实现文件中对接 TextureManager，将渲染句柄封装为 `TextureResourceHandle`。
- 禁止循环包含：AssetManager 头文件不引入 TextureManager 头文件，依赖仅在 `AssetManager.cpp`。

## 耦合现状与改造

改造前：
- TextureManager 直接依赖 `manager::AssetHandle`，内部通过 EngineContext 查找 AssetManager 并调用加载接口。
- 形成下层反向依赖上层，职责边界混杂。

改造后：
- TextureManager 移除 `AssetHandle` 相关 API，仅保留纹理资源生命周期 API。
- AssetManager 新增上层纹理封装接口：
  - `CreateTextureResource(const AssetHandle&)`
  - `CreateTextureResourceByPath(const std::string&)`
  - `GetTextureNativeId(const TextureResourceHandle&)`
  - `ReleaseTextureResource(const TextureResourceHandle&)`
- 纹理底层句柄通过 AssetManager 内部映射管理，不向外暴露 TextureManager 细节。

## Runtime/Editor 条件编译宏

文件：`Engine/Macro/RuntimeEditorSplit.h`

提供宏：
- `RUNTIME_DATA(...)`
- `EDITOR_DATA(...)`

行为：
- 在 `BUILD_EDITOR` 配置下：
  - `EDITOR_DATA(...)` 展开
  - `RUNTIME_DATA(...)` 置空
- 在非编辑器配置下：
  - `RUNTIME_DATA(...)` 展开
  - `EDITOR_DATA(...)` 置空

特性：
- 纯预处理器展开，无分支判断，无运行时开销。
- 支持成员变量、函数指针、嵌套结构、函数定义片段。

## 宏落地范围

- `TextureManager::TextureData`
  - 运行时字段：`textureId`、`streamable`、`debugLabel`
  - 编辑器字段：`importSettingsProfile`、`thumbnailCacheKey`、`debugLabel`
- `AssetManager`
  - 运行时字段：纹理资源映射表
  - 编辑器字段：纹理导入设置缓存
- `runtime_asset.h`
  - 运行时/编辑器字段统一在单个声明体内分离。

## 可维护性与性能评估

- 性能：宏展开后不生成多余分支或对象，数据布局在编译期固定。
- 维护性：运行时与编辑器字段在单处声明，减少双文件重复定义。
- 扩展性：新增资产类型可通过注册机制扩展，不改桥接主流程。
- 构建稳定性：Windows MSVC 与 Linux GCC/Clang 仅依赖标准预处理能力，兼容 C++23。

## 测试策略

新增测试：
- `Module/test/RuntimeEditorSplitTest.json`
- `dev/test/RuntimeEditorSplitTest/main.cpp`

覆盖点：
- 两种配置下结构布局差异（`sizeof` + 概念检测）。
- 符号可见性（`RuntimeOnlySymbol` / `EditorOnlySymbol` 条件编译）。
- 链接结果（对应配置下调用对应符号并返回 0）。
