---
name: shine-asset-system
description: "ShineAsset 系统全局架构总览与 Cooking 管线。Invoke when adding a new asset type end-to-end, working with IAssetCooker/CookingPipeline/StaticMeshCooker, understanding subsystem lifecycle, or navigating the overall pipeline (import → register → cook → runtime resolve). For import pipeline details see shine-asset-import; for registry/runtime details see shine-asset-registry; for editor UI panels see shine-asset-editor-ui."
---

# ShineAsset System — 架构总览

> 详细 API 分布在三个专项技能中：  
> - **shine-asset-import** — 导入管线（`IAssetImporter`, `ImportPipeline`, `REGISTER_IMPORTER`）  
> - **shine-asset-registry** — 注册表与运行时（`EditorAssetRegistry`, `RuntimeAssetRegistry`, `AssetHandle`, `AssetMetadata`, UUID）  
> - **shine-asset-editor-ui** — 编辑器 UI 面板（`AssetsBrower`, `AssetDependencyView`）

Module: `Module/editor/ShineAsset.json`（静态库，仅编辑器）。  
所有头文件 / 源文件位于 `src/editor/ShineAsset/`。  
依赖：`glaze`, `gltf_loader`, `engine_log`, `fmt`。  
系统库：`bcrypt.lib`（链接到 MainEngine，用于 UUID 生成）。

---

## 完整管线示意图

```
源文件 (.gltf/.obj/…)
    │  IAssetImporter::Import()            ← shine-asset-import
    ▼
.sasset（磁盘 JSON 元数据）
    │  EditorAssetRegistry::Register()     ← shine-asset-registry
    ▼
EditorAssetRegistry  ──→  AssetDependencyGraph
    │  IAssetCooker::Cook()
    ▼
Cooked Binary (.bin)
    │  RuntimeAssetRegistry::RequestLoad() ← shine-asset-registry
    ▼
RuntimeAssetRegistry  ──→  AssetBase (shared_ptr, 已加载)
    │  AssetHandle<T>::Resolve()
    ▼
游戏逻辑 / 渲染器使用
```

两注册表模式：
- **EditorAssetRegistry** — 仅编辑器，UUID ↔ 磁盘路径 + 元数据，依赖图，文件监视器。向 `EditorCompositionRoot` 注册为 Subsystem。
- **RuntimeAssetRegistry** — 发行版 + 编辑器均可用，UUID → 已加载的 `AssetBase`，线程安全。向 `EditorCompositionRoot` 注册为 Subsystem。

---

## 文件目录

| 子目录 | 内容 |
|--------|------|
| `core/` | `AssetBase`, `AssetHandle`, `AssetTypes`, `AssetUuidHelper`, `RuntimeAssetRegistry`, `AssetFactory` |
| `registry/` | `EditorAssetRegistry`, `EditorAssetRegistryIndex`, `AssetDependencyGraph` |
| `metadata/` | `AssetMetadata`, `AssetMetadataIO`（ReadAssetMetadataFile / WriteAssetMetadataFile） |
| `importers/` | `IAssetImporter`, `AssetImportSettings`, `ImporterAutoRegistry`, `ImportPipeline`, 具体导入器 |
| `cookers/` | `IAssetCooker`, `CookingPipeline`, `StaticMeshCooker` |

---

## Cooking Pipeline

### IAssetCooker 接口

```cpp
#include "editor/ShineAsset/cookers/IAssetCooker.h"

class IAssetCooker {
public:
    virtual std::string_view GetName() const noexcept = 0;
    // 返回此 cooker 负责的资产类型 ID 列表
    virtual std::vector<std::string_view> SupportedTypeIds() const noexcept = 0;
    // 在工作线程调用
    virtual CookResult Cook(const AssetCookContext& ctx) = 0;
};
```

`AssetCookContext` 关键字段：

| 字段 | 类型 | 描述 |
|------|------|------|
| `metadata` | `AssetMetadata` | 待 cook 资产的元数据 |
| `outputDir` | `std::filesystem::path` | 写出 .bin 的目录 |
| `platform` | `std::string_view` | 目标平台（`"win64"` / `"android"` 等）|

```cpp
struct CookResult {
    bool                               succeeded = false;
    std::vector<std::filesystem::path> outputFiles; // 生成的文件列表
    std::string                        errorMessage;
};
```

### CookingPipeline

```cpp
#include "editor/ShineAsset/cookers/CookingPipeline.h"
using namespace shine::editor::asset;

auto& cp = ctx.GetSystem<CookingPipeline>();

// 注册 cooker
cp.RegisterCooker(std::make_shared<StaticMeshCooker>());

// Cook 全部已注册资产
cp.CookAll(editorRegistry, outputDir, "win64");

// 单次 cook
CookResult r = cp.CookSingle(metadata, outputDir, "win64");
```

### 新建 Cooker

```cpp
#include "editor/ShineAsset/cookers/IAssetCooker.h"
#include "editor/ShineAsset/core/AssetTypes.h"

namespace shine::editor::asset {

class TextureCooker final : public IAssetCooker {
public:
    std::string_view GetName() const noexcept override { return "Texture Cooker"; }
    std::vector<std::string_view> SupportedTypeIds() const noexcept override {
        return { AssetTypeId::Texture };
    }
    CookResult Cook(const AssetCookContext& ctx) override {
        CookResult result;
        // 1. 读取 ctx.metadata.asset.sourceFile
        // 2. 按 ctx.platform 转码 / 压缩
        // 3. 写出到 ctx.outputDir
        // 4. 填充 result.outputFiles
        result.succeeded = true;
        return result;
    }
};

} // namespace shine::editor::asset
```

注册：`pipeline.RegisterCooker(std::make_shared<TextureCooker>());`

---

## 新增资产类型（端到端）

1. 在 `core/AssetTypes.h` 添加 type ID：
   ```cpp
   namespace shine::editor::asset::AssetTypeId {
       inline constexpr std::string_view ParticleSystem = "particle_system";
   }
   ```

2. 创建继承 `AssetBase` 的运行时类（可放在非编辑器代码）：
   ```cpp
   #include "editor/ShineAsset/core/AssetBase.h"

   class ParticleSystemAsset : public shine::asset::AssetBase {
   public:
       explicit ParticleSystemAsset(STextView uuid)
           : AssetBase(uuid, "particle_system") {}
       // 运行时数据成员 …
   };
   ```

3. 向 `RuntimeAssetRegistry` 注册工厂（启动时）：
   ```cpp
   rr.RegisterFactory("particle_system", [](STextView uuid) {
       return std::make_shared<ParticleSystemAsset>(uuid);
   });
   ```

4. 实现 `IAssetImporter`（参见 **shine-asset-import** 技能）。

5. 可选：实现 `IAssetCooker` 并注册到 `CookingPipeline`。

6. 用 `AssetHandle<ParticleSystemAsset>` 作为可序列化引用（参见 **shine-asset-registry** 技能）。

---

## Subsystem 生命周期

```
EditorCompositionRoot::RegisterSystems(ctx)
    ├── ctx.Register(new RuntimeAssetRegistry())
    ├── ctx.Register(new EditorAssetRegistry())
    └── ctx.Register(new ImportPipeline())

ImportPipeline::Init(ctx)
    └── ImporterAutoRegistry::CreateAll() → 实例化所有 REGISTER_IMPORTER 类

EditorAssetRegistry::Init(ctx)
    ├── 尝试 LoadRegistryIndex("Content/.assetindex") — 快速启动
    ├── 降级：Scan("Content/")
    └── 订阅 FileWatchService::OnFileChanged → OnFileChangeEvent()

EditorAssetRegistry::Shutdown(ctx)
    ├── SaveRegistryIndex("Content/.assetindex")
    └── 取消订阅文件监视器
```

---

## 构建 & 测试

```powershell
# 构建主引擎（ShineAsset 作为链接的静态库）
.\build.bat run --release --msvc

# 构建并运行资产测试
.\build.bat test ShineAssetTest
.\build.bat test ShineAssetTest --release
```

测试文件：`dev/test/ShineAssetTest/ShineAssetTest.cpp`  
Module：`Module/test/ShineAssetTest.json`（依赖：`ShineAsset`, `fmt`, `glaze`）

---

## 惯例

- 内部代码用 `SString` / `STextView`；`AssetMetadata.h` 的结构体用 `std::string`（Glaze 序列化边界）。
- UUID 格式：RFC 9562 规范小写加连字符 `xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`。
- 新建资产时**优先使用 V7 UUID**（`GenerateV7UUIDString()`）——时间有序，索引局部性更好。
- `RuntimeAssetRegistry` 有互斥锁保护；`EditorAssetRegistry` 单线程（编辑器主线程）。
- `src/editor/ShineAsset/` 下的新文件由 Module JSON 的 `dirs` glob **自动发现**，无需手动列举。
