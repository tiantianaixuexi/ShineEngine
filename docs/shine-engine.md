---
name: ShineEngine-Developer-Guide
description: ShineEngine 开发综合指南，涵盖项目结构、构建命令、模块管理及工作流
---

# ShineEngine 开发指南

## 描述
这是一个用于 ShineEngine 开发的综合指南，旨在帮助开发者和 AI 助手理解项目结构、构建系统以及如何扩展功能。它提供了从环境搭建到代码提交的全流程参考。

## 使用场景
- 当需要构建项目（Debug/Release/WASM）时
- 当需要创建新模块或添加第三方依赖时
- 当需要了解项目目录结构与配置文件格式时
- 当进行测试或跨平台开发调试时

## 新建分支
- git checkout -b support-clang-21.1.7

## 详情

### 1. 项目概览 (Project Overview)

ShineEngine 是一个现代 C++ 游戏引擎（C++23/C17），支持 Windows 和 WebAssembly (WASM) 平台。
构建系统采用**数据驱动**的方式：`CMakeLists.txt` 动态读取 JSON 配置文件来生成构建目标。

#### 关键目录
- **`src/`**: 引擎核心源代码。
- **`Module/`**: 定义库模块的 JSON 配置文件。CMake 自动扫描此目录。
- **`Program/`**: 定义可执行程序（如 Launcher, Demos）的 JSON 配置文件。
- **`Build.bat`**: 核心构建脚本，封装了 CMake 操作。
- **`.vscode/module.schema.json`**: 模块配置文件的 JSON Schema 定义。

### 2. 构建命令 (Build Commands)

使用项目根目录下的 `Build.bat` 进行所有构建操作。

| 任务 | 命令 | 说明 |
| :--- | :--- | :--- |
| **构建并运行 (Debug)** | `Build.bat run` | 编译 MainEngine (Debug) 并启动 |
| **构建 (Debug Only)** | `Build.bat x64` | 仅编译，不运行 |
| **构建 Release 版** | `Build.bat release` | 编译 Release 版本，询问是否运行 |
| **构建 WebAssembly** | `Build.bat wasm [name]` | 编译 WASM 版本 (默认: smallwasm) |
| **构建指定模块** | `Build.bat module <name>` | 仅编译指定模块 (如 `Build.bat module math`) |
| **构建指定 EXE** | `Build.bat exe <name>` | 编译指定可执行文件 (如 `Build.bat exe EngineLauncher`) |
| **运行测试** | `Build.bat test` | 编译并运行测试 (TestRunner) |
| **清理项目** | `Build.bat clean` | 删除 build 目录和生成的文件 |
| **生成 Clangd 配置** | `Build.bat compile_commands`| 生成 compile_commands.json |

**常用参数**:
- `--no-editor`: 禁用编辑器功能（构建 Runtime 模式）。
- `--release`: 强制使用 Release 配置（适用于 module/wasm/test）。
- `--no-pause`: 脚本结束后不暂停（适合 CI 环境）。
- `--msvc` / `--clang` / `--gcc`: 选择编译器（默认 MSVC）。

### 3. 模块管理 (Module Management)

ShineEngine 的模块化是通过 JSON 配置实现的。要添加新功能，必须创建源码并添加对应的 JSON 定义。

#### 3.1 创建新模块
1.  **创建源码**: 在 `src/` 下建立目录，例如 `src/physics/`。
2.  **创建配置**: 在 `Module/` 下建立 JSON，例如 `Module/physics.json`。

**JSON 模板**:
```json
{
  "name": "physics",
  "type": "static",
  "dirs": ["src/physics"],
  "deps": ["shine_define", "math"],
  "defines": ["ENABLE_PHYSICS"]
}
```

#### 3.2 关键字段说明 (Schema Reference)
参考 `.vscode/module.schema.json`：

##### 基础字段
- **`name`** (Required): 模块名，必须与文件名一致。
- **`type`**: 构建类型，默认 `"lib"`。
    - `static`/`lib`: 静态库 (.lib/.a)。
    - `shared`: 动态库 (.dll/.so)。
    - `exe`: 可执行文件。
    - `third`: 第三方库引用（不编译源码，只链接预编译库）。
    - `interface`: 接口库（仅头文件，无编译输出）。
    - `subcmake`: 独立子 CMake 工程（如 WASM 模块）。
- **`dirs`**: 递归扫描源码的目录列表。
- **`files`**: 显式指定源文件列表（当不想扫描整个目录时使用）。
- **`deps`**: 依赖的其他模块名称。
- **`defines`**: 预处理器宏定义列表。
- **`comment`**: 模块注释说明。

##### 源文件字段
- **`files_module`**: 启用 C++20 模块时使用的文件（`.ixx`）。
- **`files_header`**: 不使用模块时使用的头文件（`.h`）。
- **`files_windows` / `files_wasm` / `files_android`**: 特定平台的源文件。

##### 包含目录
- **`include_dirs`**: 额外的头文件包含目录列表。用于第三方库或特殊目录结构，编译时会添加 `-I` 或 `/I` 选项。
  
  **示例**（mimalloc 使用 Single Source 方式编译）:
  ```json
  {
    "name": "mimalloc",
    "files": ["src/third/mimalloc/src/static.c"],
    "dirs": ["src/third/mimalloc"],
    "defines": ["MI_STATIC_LIB"],
    "type": "static",
    "include_dirs": [
      "src/third/mimalloc",
      "src/third/mimalloc/src"
    ],
    "comment": "第三方库，使用 Single Source 方式编译"
  }
  ```

##### 构建模式
- **`buildMode`**: 控制模块在 Editor 或 Runtime 模式下是否构建。
    - `editor`: 仅 Editor 模式构建。
    - `runtime`: 仅 Runtime 模式构建。
    - `both`: 两种模式都构建（默认）。
  
  **示例**:
  ```json
  {
    "name": "editor_ui",
    "dirs": ["src/editor/ui"],
    "type": "lib",
    "buildMode": "editor",
    "comment": "仅 Editor 模式构建的模块"
  }
  ```

##### 平台限制
- **`platform`**: 平台限制，例如 `["Windows"]` 或 `["Wasm", "Linux"]`。

##### 第三方库链接
- **`link`**: 第三方库链接配置，仅对 `type: "third"` 的模块有效。
    - `debug.lib`: Debug 配置要链接的库文件。
    - `release.lib`: Release 配置要链接的库文件。
    - `debug.dll` / `release.dll`: 要复制的动态库文件。

  **示例**:
  ```json
  {
    "name": "fmt",
    "type": "third",
    "files": ["src/third/fmt/format.h"],
    "link": {
      "debug": { "lib": ["fmtd.lib"] },
      "release": { "lib": ["fmt.lib"] }
    }
  }
  ```

##### 子 CMake 工程
- **`subcmake`**: 独立子 CMake 工程配置，用于构建独立的子项目（如 WASM 模块）。
    - `source`: 子工程源码目录，默认 `Program/{模块名}`。
    - `build`: 子工程构建目录，默认 `Program/{模块名}/build`。
    - `generator`: CMake 生成器，默认 `Ninja`。
    - `target`: 要构建的目标名称，默认 `all`。
    - `configure`: 传递给 CMake configure 阶段的额外参数。

  **示例**:
  ```json
  {
    "name": "smallwasm",
    "type": "subcmake",
    "dirs": ["Program/smallwasm/src"],
    "subcmake": {
      "source": "Program/smallwasm",
      "build": "Program/smallwasm/build",
      "generator": "Ninja",
      "target": "all",
      "configure": ["-DSMALLWASM_DEBUG=ON"]
    },
    "comment": "独立的 WASM 子工程"
  }
  ```

#### 3.3 添加第三方库

##### 方式一：链接预编译库
将库文件放入 `src/third/lib/`，并在 `Module/third/` 创建配置。适用于 MSVC/Clang 预编译的库。

**注意**: MSVC 和 GCC 的 C++ ABI 不兼容，预编译库不能混用。

##### 方式二：从源码编译（推荐）
对于需要跨编译器兼容的第三方库，推荐从源码编译。许多库支持 Single Source 方式：

- **mimalloc**: 使用 `src/static.c` 单文件编译
- **fmt**: 可以从源码编译
- **stb**: 单头文件库，直接包含即可

### 4. 开发工作流 (Development Workflow)

#### 添加新功能
1.  确定功能所属模块（现有模块或新模块）。
2.  如果是新模块，按 3.1 步骤创建 JSON。
3.  编写 C++ 代码 (`.cpp`, `.h`, `.ixx`)。使用 c++23/26 标准。
4.  运行 `Build.bat compile_commands` 更新 LSP 支持。
5.  运行 `Build.bat module <name>` 验证编译。

#### 调试 WASM
1.  项目使用 clang 编译 wasm。
2.  运行 `Build.bat wasm`。
3.  生成的 HTML/JS/WASM 文件位于 `Program/smallwasm/web/` 或构建目录中。

#### 运行测试
1.  在 `test/` 目录下添加测试代码。
2.  确保 `Module/test/` 下有对应的测试模块配置。
3.  运行 `Build.bat test`。

### 5. 编译器兼容性说明

不同编译器的 C++ ABI 不兼容，以下情况需要注意：

| 编译器 | 预编译库兼容性 |
| :--- | :--- |
| MSVC | 仅兼容 MSVC 编译的库 |
| Clang (Windows) | 可兼容 MSVC 库（使用 `-fms-compatibility`） |
| GCC (MinGW) | **不兼容** MSVC 库，需从源码编译 |

**解决方案**:
1. 对于 GCC 编译，使用 `include_dirs` 和源码编译方式。
2. 使用 Single Source 方式编译第三方库（如 mimalloc 的 `static.c`）。
3. 或者为不同编译器准备不同的预编译库。
