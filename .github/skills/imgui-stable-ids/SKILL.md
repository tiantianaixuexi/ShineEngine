---
name: imgui-stable-ids
description: "ImGui stable ID and UI hotpath rules for editor code. Use when adding or refactoring TreeNode, Selectable, InputText, Checkbox, CollapsingHeader, custom IconTreeNode/IconLeafNode, or any per-frame editor UI that currently builds display##id strings, fmt::format labels, or temporary IDs every frame."
---

# ImGui Stable IDs

## Invoke When

- Editing any editor UI rendered every frame
- You see display##id string concatenation
- You see fmt::format used only to create hidden ImGui IDs
- You add dynamic TreeNode, Selectable, InputText, Checkbox, CollapsingHeader, BeginTable IDs
- You work in AssetsBorwer, PropertiesView, SceneHierarchyView, WidgetDesigner, or similar panels

## Core Rule

Separate visible text from unique ID.

Prefer these patterns in order:

1. Use object pointer IDs when the UI item already maps to a live object
2. Use integer or index IDs when the parent scope is already stable
3. Use PushID or ScopedImGuiID around a widget and keep the widget label static
4. For custom widgets that do not expose an explicit ID, add an overload that accepts an ID seed

Avoid building strings like these every frame:

```cpp
fmt::format("{}##{}", name, uuid)
name.to_string() + "##" + path.string()
fmt::format("##val_{}", key)
```

## Preferred Helpers In This Repo

General widgets:

```cpp
#include "editor/util/ImGuiIdScope.h"

shine::editor::util::ScopedImGuiID idScope(static_cast<int>(index));
ImGui::InputText("##Value", &text);
```

Custom asset tree widgets:

```cpp
shine::widget::IconTreeNodeEx(static_cast<int>(PathToID(path)), displayLabel.c_str(), openIcon, closeIcon, flags);
shine::widget::IconLeafNodeEx(uuid.c_str(), displayName.c_str(), icon);
```

ImGui tree widgets with visible formatting but stable ID stack:

```cpp
shine::editor::util::ScopedImGuiID nodeId(widget.id);
ImGui::TreeNodeEx("WidgetNode", flags, "%s%s", icon, widget.name.c_str());
```

## Panel Recipes

### Arrays and maps

- Property row: push one stable property ID once
- Element rows: push index or key ID per child
- Use fixed labels such as "##ArrayElement" or "##MapValue"

### Collapsing groups

- Push a stable group ID with group index or stable key
- Use visible group title directly
- Reuse a fixed table ID inside that scope

### Custom tree widgets

- Do not require callers to concatenate display##id
- Add Ex overloads that push and pop ID internally

## Review Checklist

1. Does this widget rebuild a hidden ID string every frame?
2. Can the unique identity come from pointer, integer, path hash, UUID, or parent scope instead?
3. Is the dynamic string only for visible text? If yes, keep it display-only.
4. If a custom widget only accepts a label, should it gain an explicit ID overload?
5. Are internal strings kept in SString or STextView unless crossing an external API boundary?

## Anti-Patterns

- Caching display##id strings as the first choice when PushID would solve it cleanly
- Using item index as the only ID in mutable or reorderable collections without a stable parent key
- Reusing the same static hidden label in a loop without PushID
- Converting to std::string just to append ##...

## Notes

- Visible text may still need to be rebuilt when the content itself changes every frame; the optimization target is hidden ID construction and unnecessary temporary strings.
- For filesystem paths, prefer a stable hashed ID helper over path.string plus ## concatenation.