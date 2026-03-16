---
name: shine-asset-editor-ui
description: "ShineAsset editor UI panels. Invoke when working with AssetsBrower, AssetDependencyView, AssetsItem, the import-settings popup, OS file-drop integration, or the SHINE_ASSET_PATH drag-and-drop payload."
---

# ShineAsset — Editor UI Panels

所有 UI 面板均为编辑器专属，不得从运行时代码包含。

---

## 文件位置

| 文件 | 命名空间 | 用途 |
|------|----------|------|
| `src/editor/browers/AssetsBrower.h/.cpp` | `shine::editor::assets_brower` | 资产浏览器主面板（目录树 + 资产网格 + 导入弹窗）|
| `src/editor/browers/AssetsItem.h` | `shine::editor::assets_item` | 资产列表项排序辅助类 |
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
    │       └── TreeNode("Content") → RenderDirectoryNode(root)
    └── ImGui::BeginChild("AssetsList", {0, 0}, Borders)
            ├── TextUnformatted(当前目录路径)
            ├── Separator
            ├── RenderAssetGrid()           // 图标网格
            └── RenderOperationsPopup()     // 右键菜单：重命名/删除/移动
```

图标尺寸通过菜单栏 `视图 → SliderFloat("图标尺寸")` 调节，范围 32–128 px。

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
