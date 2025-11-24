@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

goto :main

::: ================================
::: 工具函数
::: ================================
:error_exit
echo 错误: %~1
echo.
pause
exit /b 1

:log_info
echo  信息: %~1
goto :eof

:log_success
echo 成功: %~1
goto :eof

:log_warning
echo 警告: %~1
goto :eof

:main
::: ================================
::: 配置设置
::: ================================
set "GENERATOR_NAME=Visual Studio 18 2026"
set "BUILD_DIR=build"
set "ARCH_ARGS=-A x64"
set "CMAKE_COMMON_FLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
set "CLEAN_FIRST="
set "CMD="

::: 参数解析
:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--clean-first" (
    set "CLEAN_FIRST=--clean-first"
    shift
    goto parse_args
)
if "%~1"=="--help" goto show_help
if "%~1"=="-h" goto show_help
if not defined CMD set "CMD=%~1"
shift
goto parse_args

:args_done

::: 工具检查
call :log_info "检查构建工具..."
cmake --version > nul 2>&1 || call :error_exit "未找到CMake，请确保CMake已安装并添加到PATH"

::: 命令分发
if "%CMD%"=="run" goto cmd_run
if "%CMD%"=="x64" goto cmd_x64
if "%CMD%"=="release" goto cmd_release
if "%CMD%"=="wasm" goto cmd_wasm
if "%CMD%"=="exe" goto cmd_exe
if "%CMD%"=="module" goto cmd_module
if "%CMD%"=="clean" goto cmd_clean
if "%CMD%"=="list" goto cmd_list
goto show_help

::: ================================
::: 通用构建函数
::: 参数: 1=Target, 2=Config, 3=RebuildDeps, 4=RunAfter
::: ================================
:build_generic
set "TARGET_NAME=%~1"
set "BUILD_CONFIG=%~2"
set "REBUILD_DEPS=%~3"
set "RUN_AFTER=%~4"

if "%TARGET_NAME%"=="" (
    set "TARGET_PARAM=" 
    set "TARGET_DISPLAY=完整项目"
) else (
    set "TARGET_PARAM=--target %TARGET_NAME%"
    set "TARGET_DISPLAY=目标 %TARGET_NAME%"
)

call :log_info "构建配置: %BUILD_CONFIG% - %TARGET_DISPLAY%"
call :log_info "配置CMake项目..."

if "%REBUILD_DEPS%"=="" set "REBUILD_DEPS=ON"

cmake -B %BUILD_DIR% -S . -G "%GENERATOR_NAME%" %ARCH_ARGS% %CMAKE_COMMON_FLAGS% -DCMAKE_BUILD_TYPE=%BUILD_CONFIG% -DREBUILD_DEPS=%REBUILD_DEPS% || call :error_exit "CMake配置失败"

call :log_info "开始编译..."
cmake --build %BUILD_DIR% --config %BUILD_CONFIG% %TARGET_PARAM% --parallel %CLEAN_FIRST% || call :error_exit "编译失败"

call :log_success "编译成功！"

if "%RUN_AFTER%"=="TRUE" (
    call :run_executable "%TARGET_NAME%" "%BUILD_CONFIG%"
)
goto :eof

::: ================================
::: 运行可执行文件
::: ================================
:run_executable
set "T_NAME=%~1"
set "T_CONFIG=%~2"
if "%T_NAME%"=="" set "T_NAME=MainEngine"

set "SUFFIX="
if /i "%T_CONFIG%"=="Debug" set "SUFFIX=d"

rem 按优先级查找可执行文件
for %%p in (
    "%BUILD_DIR%\exe\%T_NAME%%SUFFIX%.exe"
    "%BUILD_DIR%\exe\%T_NAME%.exe"
    "exe\%T_NAME%%SUFFIX%.exe"
    "exe\%T_NAME%.exe"
) do (
    if exist %%p (
        call :log_info "运行: %%p"
        "%%p"
        goto :eof
    )
)

call :log_warning "未找到可执行文件: %T_NAME%"
goto :eof

::: ================================
::: 命令实现
::: ================================
:cmd_run
call :build_generic "MainEngine" "Debug" "ON" "TRUE"
goto end_script

:cmd_x64
call :build_generic "MainEngine" "Debug" "ON" "FALSE"
call :run_executable "MainEngine" "Debug"
goto end_script

:cmd_release
call :build_generic "MainEngine" "Release" "ON" "FALSE"
set /p "RUN_CHOICE=是否运行Release版本? (y/N): "
if /i "!RUN_CHOICE!"=="y" call :run_executable "MainEngine" "Release"
goto end_script

:cmd_wasm
call :log_info "WASM版本构建暂未实现完整自动化"
goto end_script

:cmd_exe
if "%~2"=="" call :error_exit "请指定可执行文件名称"
call :build_generic "%~2" "Debug" "ON" "FALSE"
goto end_script

:cmd_module
if "%~2"=="" call :error_exit "请指定模块名称"
set "MODULE_NAME=%~2"
set "MODULE_REBUILD=ON"
set "MODULE_CONFIG=Debug"

if /i "%~3"=="--no-rebuild-deps" set "MODULE_REBUILD=OFF"
if /i "%~3"=="--release" set "MODULE_CONFIG=Release"
if /i "%~4"=="--release" set "MODULE_CONFIG=Release"

call :build_generic "%MODULE_NAME%" "%MODULE_CONFIG%" "%MODULE_REBUILD%" "FALSE"
goto end_script

:cmd_clean
call :log_info "清理构建文件..."
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "exe" (
    del /q "exe\*.exe" 2>nul
    del /q "exe\*.dll" 2>nul
    del /q "exe\*.pdb" 2>nul
)
call :log_success "清理完成"
goto end_script

:cmd_list
call :log_info "模块列表:"
if exist "Module" (
    dir /b /s "Module\*.json"
) else (
    call :log_warning "Module 目录不存在"
)
goto end_script

:show_help
echo 用法: build.bat [run|x64|release|exe|module|clean|list] [args]
echo.
echo   run             构建 MainEngine (Debug) 并运行
echo   x64             构建 MainEngine (Debug)
echo   release         构建 MainEngine (Release)
echo   exe [name]      构建指定可执行文件 (如 EngineLauncher)
echo   module [name]   构建指定模块
echo   clean           清理
echo.
goto end_script

:end_script
pause
exit /b 0
