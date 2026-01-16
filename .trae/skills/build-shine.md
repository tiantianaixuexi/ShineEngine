# Build ShineEngine

## Description
Build the ShineEngine project using the provided batch script `Build.bat`.

## Usage
`Build.bat [command] [options]`

## Commands
- `run`             : Build MainEngine (Debug) and run immediately.
- `x64`             : Build MainEngine (Debug) without running.
- `release`         : Build MainEngine (Release).
- `wasm [name]`     : Build WebAssembly target (default: `smallwasm`).
- `exe [name]`      : Build a specific executable (e.g., `EngineLauncher`).
- `module [name]`   : Build a specific module.
- `test`            : Build and run tests.
- `clean`           : Clean all build files and artifacts.
- `compile_commands`: Generate `compile_commands.json` for clangd/IntelliSense.

## Options
- `--release`       : Use Release configuration (applies to module/wasm/test/exe).
- `--runtime`       : Build in runtime mode (excludes editor-only features).
- `--no-editor`     : Same as `--runtime`.
- `--editor`        : Enable editor mode (default).
- `--clean-first`   : Force a clean before building.
- `--no-pause`      : Don't pause the console at the end (useful for CI).

## Examples
### Common Workflows
- **Run Editor (Debug)**:
  ```cmd
  .\Build.bat run
  ```
- **Build Editor (Release)**:
  ```cmd
  .\Build.bat release
  ```
- **Build Runtime Executable**:
  ```cmd
  .\Build.bat exe MainEngine --runtime
  ```
- **Build WASM**:
  ```cmd
  .\Build.bat wasm --release
  ```
- **Clean and Rebuild**:
  ```cmd
  .\Build.bat run --clean-first
  ```
