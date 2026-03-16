---
name: shine-asset-editor-ui
description: "ShineAsset editor UI panels. Invoke when working with AssetsBrower, AssetDependencyView, AssetsItem, the import-settings popup, OS file-drop integration, the SHINE_ASSET_PATH drag-and-drop payload, or thumbnail provider system (IAssetThumbnailProvider)."
---

# ShineAsset — Editor UI Panels

所有 UI 面板均为编辑器专属，不得从运行时代码包含。

---

## 文件位置

| 文件 | 命名空间 | 用途 |
|------|----------|------|
| `src/editor/browers/AssetsBrower.h/.cpp` | `shine::editor::assets_brower` | 资产浏览器主面板（目录树 + 资产网格 + 导入弹窗）|
| `src/editor/browers/AssetsItem.h` | `shine::editor::assets_item` | 资产列表项排序辅助类 |
| `src/editor/browers/IAssetThumbnailProvider.h` | `shine::editor::assets_brower` | 缩略图提供者抽象基类 |
| `src/editor/browers/ThumbnailProviderRegistry.h` | `shine::editor::assets_brower` | 缩略图提供者注册表（header-only）|
| `src/editor/browers/builtin_thumbnail_providers.h/.cpp` | `shine::editor::assets_brower` | 内置提供者：`ImageThumbnailProvider`、`ModelThumbnailProvider` |
| `src/editor/views/AssetDependencyView.h/.cpp` | `shine::editor::views` | 资产依赖关系面板 |

---

## AssetsBrower

### 依赖的子系统

```cpp
#include "editor/ShineAsset/registry/EditorAssetRegistry.h"
#include "editor/ShineAsset/importers/ImportPipeline.h"
#include "util/EngineDirectoryService.h"
```

`onInit()` 内自动从 `EngineContext` 获取上述子系统；也可在初始化前手动注入：

```cpp
browser.SetEditorAssetRegistry(&editorRegistry);
```

### OS 文件拖入（跨线程安全）

Win32 `WM_DROPFILES` 处理函数调用：

```cpp
#include "editor/browers/AssetsBrower.h"

shine::editor::assets_brower::EnqueueExternalDrop(droppedPaths);
// droppedPaths: std::vector<std::filesystem::path>
```

浏览器在每帧 `onRender()` 开头消费队列（每帧处理一条），自动走导入弹窗流程。

### 导入弹窗流程

```
EnqueueExternalDrop(paths)
    └─ (下一帧 onRender) 弹出队列首条路径
           ├─ importPipeline_->FindImporter(path)   → IAssetImporter* 或 nullptr
           ├─ 设置 pendingImport_
           └─ requestImportPopup_ = true
                   └─ ImGui::OpenPopup("##ImportSettings")
                           ├─ importer->RenderImportSettingsUI(pending.settingsJson)  // 委托给导入器
                           └─ [点击"导入"]
                                   └─ importPipeline_->ExecuteImport(
                                          *importer,
                                          pending.sourcePath,
                                          selectedDirectory_,   // 目标目录
                                          contentRoot,          // Content/ 根目录
                                          pending.settingsJson,
                                          editorAssetRegistry_)
```

无匹配导入器时弹窗显示警告，"导入"按钮禁用。

### 拖拽移动（面板内）

浏览器同时作为 DragDrop 目标接受 `"SHINE_ASSET_PATH"` 负载：

```cpp
// 发起拖拽（资产网格内每个条目）
if (ImGui::BeginDragDropSource())
{
    const std::string pathStr = entry.path().string();
    ImGui::SetDragDropPayload("SHINE_ASSET_PATH", pathStr.c_str(), pathStr.size() + 1);
    ImGui::EndDragDropSource();
}

// 接受拖拽（目录树节点 / 网格区域）
if (ImGui::BeginDragDropTarget())
{
    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("SHINE_ASSET_PATH"))
    {
        std::filesystem::path src(static_cast<const char*>(p->Data));
        browser.MoveEntry(src, targetDirectory);
    }
    ImGui::EndDragDropTarget();
}
```

移动后 `SyncAssetRecordMove(oldPath, newPath)` 同步更新 `EditorAssetRegistry`。

### 面板布局

```
ImGui::Begin("资产浏览器")
    ├── ImGui::BeginChild("AssetsTree", {260, 0}, Borders | ResizeX)
    │       ├── InputTextWithHint("##asset_search", "搜索文件")
    │       └── RenderDirectoryNode(contentRoot_)   // 从固定根目录渲染，DefaultOpen
    └── ImGui::BeginChild("AssetsList", {0, 0}, Borders)
            ├── TextUnformatted(当前目录路径)
            ├── Separator
            ├── RenderAssetGrid()           // 多选网格（ImGuiMultiSelect + 虚拟裁剪）
            └── RenderOperationsPopup()     // 右键菜单 + 目录 CRUD 弹窗
```

图标尺寸通过菜单栏 `视图 → SliderFloat("图标尺寸")` 调节（32–128 px），或 Ctrl+滚轮缩放。

### ImportPending 内部结构

```cpp
struct ImportPending {
    std::filesystem::path  sourcePath;
    asset::IAssetImporter* importer;      // 非拥有，生命周期由 ImportPipeline 管理
    glz::raw_json          settingsJson;  // 由 RenderImportSettingsUI 每帧更新
};
```

---

## AssetDependencyView

展示选中资产的正向 / 反向依赖，并高亮悬挂引用。

```cpp
#include "editor/views/AssetDependencyView.h"

shine::editor::views::AssetDependencyView depView;

// 注入注册表（可在 onInit 后任意时刻调用）
depView.SetEditorAssetRegistry(&editorRegistry);

// 切换当前选中资产
depView.SetSelectedAssetUUID("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
```

面板窗口名称固定为 `"资产依赖"`（在 `onInit()` 内设置）。

**渲染内容**：

| 区域 | 描述 |
|------|------|
| Header | UUID、类型、磁盘路径；若为悬挂引用显示红色警告 |
| 依赖项（可折叠） | 本资产依赖的 UUID 列表；缺失项红色标注 `[缺失]` |
| 被依赖项（可折叠）| 依赖本资产的 UUID 列表 |

---

## 资产缩略图系统

### 架构概述

```
IAssetThumbnailProvider   ← 抽象基类，用户继承并重写
        ▲
        │  内置实现
        ├── ImageThumbnailProvider   (.png / .jpg / .jpeg → 真实缩略图)
        └── ModelThumbnailProvider   (.obj / .gltf / .glb / .fbx / .dae → 等轴测方块图标)

ThumbnailProviderRegistry  ← 保存有序 provider 列表，first-match 策略
        ▲
        │  成员
AssetsBrower::thumbnailRegistry_
```

### 使用内置提供者（默认，无需额外代码）

`onInit()` 中已自动调用 `RegisterBuiltinThumbnailProviders(thumbnailRegistry_)`，
图片和模型文件自动显示缩略图/图标，其他类型回退到默认颜色方块。

### 注册自定义提供者

```cpp
#include "editor/browers/IAssetThumbnailProvider.h"
#include "editor/browers/ThumbnailProviderRegistry.h"

class ShaderThumbnailProvider : public shine::editor::assets_brower::IAssetThumbnailProvider
{
public:
    bool CanHandle(const std::filesystem::path& path) const override
    {
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".glsl" || ext == ".hlsl";
    }

    bool DrawThumbnail(ImDrawList* drawList,
                       const std::filesystem::path& path,
                       ImVec2 iconMin, ImVec2 iconMax,
                       bool isSelected) override
    {
        drawList->AddRectFilled(iconMin, iconMax, IM_COL32(180, 80, 255, 220), 6.0f);
        // ... 自定义绘制
        return true;  // 返回 true = 已绘制，跳过默认图标
    }
};

// 在 onInit() 之后注册（低于内置优先级）
browser.RegisterThumbnailProvider(std::make_unique<ShaderThumbnailProvider>());

// 在 onInit() 之前注册（高于内置优先级，可覆盖内置行为）
// 注意：需在 RegisterBuiltinThumbnailProviders 调用之前调用
browser.RegisterThumbnailProvider(std::make_unique<MyHighPriorityProvider>());
```

### 扩展内置提供者（示例：为图片添加格式支持）

```cpp
class ExtendedImageProvider : public shine::editor::assets_brower::ImageThumbnailProvider
{
public:
    bool CanHandle(const std::filesystem::path& path) const override
    {
        // 先检查父类支持的格式
        if (ImageThumbnailProvider::CanHandle(path)) return true;
        // 再添加 .bmp 支持（只需重写 CanHandle，DrawThumbnail 复用父类）
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".bmp";
    }

protected:
    shine::render::TextureHandle LoadOrGetTexture(
        const std::filesystem::path& path) override
    {
        // 处理 .bmp 加载，其他格式交给父类
        auto ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".bmp")
        {
            // 自定义加载逻辑 ...
        }
        return ImageThumbnailProvider::LoadOrGetTexture(path);
    }
};
```

### 关键约定

| 约定 | 说明 |
|------|------|
| 线程安全 | `CanHandle` / `DrawThumbnail` / `Tick` 均在主线程调用，可安全使用 ImGui/GL |
| GPU 资源生命周期 | 析构时通过 `TextureManager::ReleaseTexture` 释放；`onShutDown()` 中调用 `thumbnailRegistry_.Clear()` 确保 EngineContext 仍有效时释放 |
| 返回 false | `DrawThumbnail` 返回 `false` 时调用方自动回退到默认颜色方块 + 类型文字 |
| 加载失败缓存 | `ImageThumbnailProvider` 对失败路径记入 `failedPaths_`，避免每帧重试 |
| 优先级顺序 | `ThumbnailProviderRegistry::Find` 返回第一个 `CanHandle == true` 者；先 `Register` 优先级更高 |

---

## AssetsItem（排序辅助）

```cpp
#include "editor/browers/AssetsItem.h"

// 构造：ID = ImGuiID，Type = 资产类型枚举值
shine::editor::assets_item::AssetsItem item(id, type);

// 配合 ImGui 表格排序规格批量排序
AssetsItem::SortWithSortSpecs(sort_specs, items_array, item_count);
```

排序键：列 0 = `ID`，列 1 = `Type`。

---

## 约定

- `EnqueueExternalDrop` 可在任意线程调用（内部 mutex 保护）。
- `AssetsBrower::onRender()` 每帧只消费一条拖入文件，避免连续弹窗。
- 移动 / 删除操作会同步调用 `EditorAssetRegistry::OnFileMoved` / `OnFileDeleted`，保持注册表一致。
- 导入成功后弹窗自动关闭；失败时保持打开并显示错误信息，允许用户修改设置后重试。
- 资产网格支持框选（`BoxSelect2d`）、键盘导航（NavWrapX）、Ctrl+滚轮缩放（锚定鼠标位置）。
