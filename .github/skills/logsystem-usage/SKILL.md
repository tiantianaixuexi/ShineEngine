---
name: logsystem-usage
description: "Registers LogSystem groups and categories with console output options. Invoke when adding module logs, configuring category visibility, or replacing fmt::print debug output."
---

# LogSystem Usage

Use this skill when adding structured runtime logs in ShineEngine.

## Register Group

In header:

```cpp
REGISTER_LOG_GROUP(EditorLog)
```

In source:

```cpp
REGISTER_LOG_GROUP_END(EditorLog)
```

## Add Categories

Default category:

```cpp
ADD_LOG_CATEGORY(EditorLog, "init")
```

Category with explicit console switch:

```cpp
ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Rendering", true)
ADD_LOG_CATEGORY_WITH_CONSOLE(EditorLog, "Memory", false)
```

## Emit Logs

```cpp
SHINE_LOG_INFO(EditorLog, "init", "MainEditor constructor called");
SHINE_LOG_WARN(EditorLog, "Rendering", "Framebuffer resize failed: {}", reason);
SHINE_LOG_ERROR(EditorLog, "Assets", "Model load failed: {}", path);
SHINE_LOG_DEBUG(EditorLog, "Input", "Mouse x={}, y={}", x, y);
```

## Notes
- `ADD_LOG_CATEGORY_WITH_CONSOLE(..., false)` disables console output for that category.
- Logs still go through `ShineLogManager` and remain available in in-engine log views.