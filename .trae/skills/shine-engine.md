# ShineEngine Developer Skill

这是一个用于 ShineEngine 开发的综合指南，旨在帮助开发者和 AI 助手理解项目结构、构建系统以及如何扩展功能。

## 1. 项目概览 (Project Overview)

ShineEngine 是一个现代 C++ 游戏引擎（C++23/C17），支持 Windows 和 WebAssembly (WASM) 平台。
构建系统采用**数据驱动**的方式：`CMakeLists.txt` 动态读取 JSON 配置文件来生成构建目标。

### 关键目录
- **`src/`**: 引擎核心源代码。
- **`Module/`**: 定义库模块的 JSON 配置文件。CMake 自动扫描此目录。
- **`Program/`**: 定义可执行程序（如 Launcher, Demos）的 JSON 配置文件。
- **`Build.bat`**: 核心构建脚本，封装了 CMake 操作。
- **`.vscode/module.schema.json`**: 模块配置文件的 JSON Schema 定义。

## 2. 构建命令 (Build Commands)

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

## 3. 模块管理 (Module Management)

ShineEngine 的模块化是通过 JSON 配置实现的。要添加新功能，必须创建源码并添加对应的 JSON 定义。

### 3.1 创建新模块
1.  **创建源码**: 在 `src/` 下建立目录，例如 `src/physics/`。
2.  **创建配置**: 在 `Module/` 下建立 JSON，例如 `Module/physics.json`。

**JSON 模板**:
```json
{
  "name": "physics",
  "type": "static",           // static, shared, lib, exe, third
  "dirs": ["src/physics"],    // 自动扫描该目录下的源码
  "deps": ["shine_define", "math"], // 依赖模块
  "defines": ["ENABLE_PHYSICS"]     // 预处理器定义
}
```

### 3.2 关键字段说明 (Schema Reference)
参考 `.vscode/module.schema.json`：
- **`name`** (Required): 模块名，必须与文件名一致。
- **`type`**: 构建类型。
    - `static`/`lib`: 静态库 (.lib/.a)。
    - `shared`: 动态库 (.dll/.so)。
    - `exe`: 可执行文件。
    - `third`: 第三方库引用（不编译源码，只链接）。
- **`dirs`**: 递归扫描源码的目录列表。
- **`files`**: 显式指定源文件列表（当不想扫描整个目录时使用）。
- **`deps`**: 依赖的其他模块名称。
- **`platform`**: 平台限制，例如 `["Windows"]` 或 `["Wasm"]`。
- **`files_wasm` / `files_windows`**: 特定平台的源文件。

### 3.3 添加第三方库
将库文件放入 `src/third/`，并在 `Module/third/` 创建配置。
示例 (`Module/third/fmt.json`):
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

## 4. 开发工作流 (Development Workflow)

### 添加新功能
1.  确定功能所属模块（现有模块或新模块）。
2.  如果是新模块，按 3.1 步骤创建 JSON。
3.  编写 C++ 代码 (`.cpp`, `.h`, `.ixx`)。使用 c++23/26 标准。c17 标准.
4.  运行 `Build.bat compile_commands` 更新 LSP 支持。
5.  运行 `Build.bat module <name>` 验证编译。

### 调试 WASM
1.  项目使用 clang 编译wasm
2.  运行 `Build.bat wasm`。
3.  生成的 HTML/JS/WASM 文件位于 `Program/smallwasm/web/` 或构建目录中。

### 运行测试
1.  在 `test/` 目录下添加测试代码。
2.  确保 `Module/test/` 下有对应的测试模块配置。
3.  运行 `Build.bat test`。
