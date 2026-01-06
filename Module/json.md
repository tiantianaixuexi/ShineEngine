# Shine Engine 模块配置文档

## 概述

模块配置文件用于定义引擎中的各个功能模块，包括源码组织、依赖关系、构建类型和平台支持等。配置文件采用 JSON 格式，放置在 `Module/` 目录下。

### 基本概念

- **文件位置**: `Module/` 目录下，文件名即模块 target 名（不含扩展名）
- **强制校验**: JSON 文件名必须与 `name` 字段完全一致，否则构建报错退出
- **构建时机**: 所有模块在主引擎之前构建
- **产物路径**:
  - 动态库: `exe/<name>.dll` 和 `exe/<name>.lib`
  - 静态库: `build/<name>.lib`
  - 可执行文件: `exe/<name>.exe`
- **主引擎行为**: 主引擎会移除模块对应源文件的编译，仅链接其库文件

## 字段说明

### 核心字段

#### name [string, 必填]
模块名称，必须与 JSON 文件名相同。
```json
{
  "name": "string_util"
}
```

#### files [string[], 可选]
该模块包含的源文件列表。
- 支持 `.cpp`、`.ixx`、`.cppm`、`.h`、`.hpp` 等
- `.ixx/.cppm` 会以 C++20 模块单元方式添加
- 纯头文件模块无需源文件
```json
{
  "files": [
    "src/util/string_util.cpp",
    "src/util/string_util.h",
    "src/util/encoding/utf8.ixx"
  ]
}
```

#### dirs [string[], 可选]
自动收集指定目录下所有相关文件的目录列表。
- 递归搜索目录及其子目录
- 支持的文件类型：`.cpp`、`.h`、`.hpp`、`.ixx`、`.cppm`、`.inl`
- 与 `files` 字段合并使用，自动去重
- 相对于项目根目录的路径
```json
{
  "dirs": [
    "src/util",
    "src/math"
  ]
}
```

#### deps [string[], 可选]
依赖的其他 JSON 模块（按文件名）。
- 表示链接另一个独立模块的库文件
- 支持传递依赖（自动解析）
```json
{
  "deps": ["fmt", "shine_define"]
}
```

#### defines [string[], 可选]
编译时宏定义列表。
```json
{
  "defines": ["ENABLE_LOGGING", "USE_UTF8"]
}
```

### 构建配置

#### type [string|string[], 可选]
模块构建类型，默认 `"lib"`。
- `"lib"`: 默认库类型，根据源码情况创建静态库或接口库
- `"static"`: 强制创建静态库
- `"shared"`: 创建动态库
- `"exe"`: 创建可执行文件
- `"third"`: 第三方库（接口库 + 链接配置）

```json
{
  "type": "static"
}
```

#### platform [string[], 可选]
仅在指定平台启用该模块。
- 可用值: `"Windows"`, `"Wasm"`, `"Linux"` 等
- 支持通配: `"Windows"` 匹配所有 Windows 平台
```json
{
  "platform": ["Windows", "Linux"]
}
```

#### buildMode [string|string[], 可选]
指定模块在哪些构建模式下构建，默认 `"both"`。
- `"editor"`: 仅在 Editor 模式下构建（`SHINE_BUILD_EDITOR=ON`）
- `"runtime"`: 仅在 Runtime 模式下构建（`SHINE_BUILD_EDITOR=OFF`）
- `"both"`: 在两种模式下都构建（默认值）
- 数组形式: `["editor", "runtime"]` 等同于 `"both"`

```json
{
  "buildMode": "editor"
}
```

```json
{
  "buildMode": ["editor", "runtime"]
}
```

**使用场景**:
- Editor 专用模块（如编辑器 UI、调试工具）: `"buildMode": "editor"`
- Runtime 专用模块（如游戏运行时逻辑）: `"buildMode": "runtime"`
- 通用模块（如工具库、数学库）: 不设置或 `"buildMode": "both"`

### 第三方库配置

#### link [object, 可选]
第三方库链接配置，仅对 `type: "third"` 的模块有效。

支持嵌套结构，按构建类型和文件类型组织：

```json
{
  "link": {
    "release": {
      "lib": ["fmt.lib"],
      "dll": ["fmt.dll"]
    },
    "debug": {
      "lib": ["fmtd.lib"],
      "dll": ["fmtd.dll"]
    }
  }
}
```

- `release`/`debug`: 构建配置
- `lib`: 要链接的库文件（.lib/.a）
- `dll`: 要复制的动态库文件（.dll/.so）

### 废弃字段

#### imports [string[], 已废弃]
⚠️ 该字段已废弃，请使用 `deps` 替代。

## 模块类型详解

### 普通模块 (lib/static/shared)
大多数功能模块使用此类型：
```json
{
  "name": "math",
  "files": ["src/math/vector.cpp", "src/math/matrix.cpp"],
  "type": "static",
  "deps": ["shine_define"]
}
```

### 可执行文件模块 (exe)
启动器或工具程序：
```json
{
  "name": "EngineLauncher",
  "files": ["src/launcher.cpp"],
  "type": "exe",
  "deps": ["fmt", "imgui"]
}
```

### 第三方库模块 (third)
封装第三方库，提供头文件和库文件链接：
```json
{
  "name": "fmt",
  "files": [
    "src/third/fmt/args.h",
    "src/third/fmt/format.h"
    // ... 其他头文件
  ],
  "type": "third",
  "defines": ["FMT_SHARED"],
  "link": {
    "release": {"lib": ["fmt.lib"]},
    "debug": {"lib": ["fmtd.lib"]}
  }
}
```

## 完整示例

### 工具模块
```json
{
  "name": "string_util",
  "files": [
    "src/util/string_util.cpp",
    "src/util/string_util.h"
  ],
  "deps": ["fmt"]
}
```

### 使用目录收集的模块
```json
{
  "name": "math",
  "dirs": ["src/math"],
  "type": "static",
  "deps": ["shine_define"]
}
```

### 混合使用files和dirs
```json
{
  "name": "util",
  "dirs": ["src/util"],
  "files": [
    "src/third/some_special_file.cpp"
  ],
  "deps": ["fmt"]
}
```

### 第三方库
```json
{
  "name": "imgui",
  "files": ["src/third/imgui/imgui.h"],
  "type": "third",
  "platform": ["Windows"],
  "defines": ["IMGUI_DEFINE_MATH_OPERATORS"],
  "link": {
    "release": {
      "lib": ["imgui.lib"],
      "dll": ["imgui.dll"]
    },
    "debug": {
      "lib": ["imguid.lib"],
      "dll": ["imguid.dll"]
    }
  }
}
```

### 可执行文件
```json
{
  "name": "MyTool",
  "files": ["src/tools/my_tool.cpp"],
  "type": "exe",
  "deps": ["string_util", "fmt"]
}
```
