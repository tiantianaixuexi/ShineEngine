@echo off
chcp 65001 >nul 2>nul
setlocal enabledelayedexpansion

goto :main

::: ================================
::: Helper Functions
::: ================================
:error_exit
echo Error: %~1
echo.
pause
exit /b 1

:log_info
echo Info: %~1
goto :eof

:log_success
echo Success: %~1
goto :eof

:log_warning
echo Warning: %~1
goto :eof

:main
::: ================================
::: Initialize Configuration
::: ================================
set "GENERATOR_NAME=Visual Studio 18 2026"
set "BUILD_DIR=build"
set "ARCH_ARGS=-A x64"
set "CMAKE_COMMON_FLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
set "CLEAN_FIRST="
set "CMD="
set "MODULE_ARG="
set "EXE_ARG="
set "MODULE_REBUILD=ON"
set "MODULE_CONFIG=Debug"

::: Parse Arguments
:parse_args
if "%~1"=="" goto args_done
if "%~1"=="--clean-first" (
    set "CLEAN_FIRST=--clean-first"
    shift
    goto parse_args
)
if "%~1"=="--help" goto show_help
if "%~1"=="-h" goto show_help
if /i "%~1"=="--no-rebuild-deps" (
    if "%CMD%"=="module" set "MODULE_REBUILD=OFF"
    shift
    goto parse_args
)
if /i "%~1"=="--release" (
    if "%CMD%"=="module" set "MODULE_CONFIG=Release"
    shift
    goto parse_args
)
if not defined CMD (
    set "CMD=%~1"
    if "%~1"=="module" (
        rem Save next argument as module name (using %~2, since %~1 is "module")
        if not "%~2"=="" set "MODULE_ARG=%~2"
    )
    if "%~1"=="exe" (
        rem Save next argument as executable name (using %~2, since %~1 is "exe")
        if not "%~2"=="" set "EXE_ARG=%~2"
    )
) else (
    rem If CMD is already set to "module", the next argument is the module name
    if "%CMD%"=="module" (
        if "%MODULE_ARG%"=="" (
            if not "%~1"=="" set "MODULE_ARG=%~1"
        )
    )
    rem If CMD is already set to "exe", the next argument is the executable name
    if "%CMD%"=="exe" (
        if "%EXE_ARG%"=="" (
            if not "%~1"=="" set "EXE_ARG=%~1"
        )
    )
)
shift
goto parse_args

:args_done

::: Check Tools
call :log_info "Checking build tools..."
cmake --version > nul 2>&1 || call :error_exit "CMake not found, please ensure CMake is installed and added to PATH"

::: Command Dispatch
if "%CMD%"=="run" goto cmd_run
if "%CMD%"=="x64" goto cmd_x64
if "%CMD%"=="release" goto cmd_release
if "%CMD%"=="wasm" goto cmd_wasm
if "%CMD%"=="exe" goto cmd_exe
if "%CMD%"=="module" goto cmd_module
if "%CMD%"=="clean" goto cmd_clean
if "%CMD%"=="list" goto cmd_list
if "%CMD%"=="compile_commands" goto cmd_compile_commands
goto show_help

::: ================================
::: Generic Build Function
::: Parameters: 1=Target, 2=Config, 3=RebuildDeps, 4=RunAfter
::: ================================
:build_generic
set "TARGET_NAME=%~1"
set "BUILD_CONFIG=%~2"
set "REBUILD_DEPS=%~3"
set "RUN_AFTER=%~4"

if "%TARGET_NAME%"=="" (
    set "TARGET_PARAM=" 
    set "TARGET_DISPLAY=All Projects"
) else (
    set "TARGET_PARAM=--target %TARGET_NAME%"
    set "TARGET_DISPLAY=Target %TARGET_NAME%"
)

call :log_info "Build Configuration: %BUILD_CONFIG% - %TARGET_DISPLAY%"
call :log_info "Configuring CMake project..."

if "%REBUILD_DEPS%"=="" set "REBUILD_DEPS=ON"

rem Visual Studio 生成器是多配置生成器，不需要 CMAKE_BUILD_TYPE
rem 配置类型通过 --config 参数在构建时指定
cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR_NAME%" %ARCH_ARGS% %CMAKE_COMMON_FLAGS% -DREBUILD_DEPS=%REBUILD_DEPS% || call :error_exit "CMake configuration failed"

call :log_info "Starting build..."
cmake --build %BUILD_DIR% --config %BUILD_CONFIG% %TARGET_PARAM% --parallel %CLEAN_FIRST% || call :error_exit "Build failed"

call :log_success "Build successful!"

rem Attempt to copy compile_commands.json to project root (if exists)
if exist "%BUILD_DIR%\compile_commands.json" (
    copy /Y "%BUILD_DIR%\compile_commands.json" "compile_commands.json" > nul 2>&1
    if not errorlevel 1 (
        call :log_info "Copied compile_commands.json for clangd usage"
    )
)

if "%RUN_AFTER%"=="TRUE" (
    call :run_executable "%TARGET_NAME%" "%BUILD_CONFIG%"
)
goto :eof

::: ================================
::: Run Executable
::: ================================
:run_executable
set "T_NAME=%~1"
set "T_CONFIG=%~2"
if "%T_NAME%"=="" set "T_NAME=MainEngine"

set "SUFFIX="
if /i "%T_CONFIG%"=="Debug" set "SUFFIX=d"

rem Find executable by priority
for %%p in (
    "%BUILD_DIR%\exe\%T_NAME%%SUFFIX%.exe"
    "%BUILD_DIR%\exe\%T_NAME%.exe"
    "exe\%T_NAME%%SUFFIX%.exe"
    "exe\%T_NAME%.exe"
) do (
    if exist %%p (
        call :log_info "Running: %%p"
        "%%p"
        goto :eof
    )
)

call :log_warning "Executable not found: %T_NAME%"
goto :eof

::: ================================
::: Command Implementation
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
set /p "RUN_CHOICE=Run Release version? (y/N): "
if /i "!RUN_CHOICE!"=="y" call :run_executable "MainEngine" "Release"
goto end_script

:cmd_wasm
call :log_info "WASM build feature not yet implemented, stay tuned"
goto end_script

:cmd_exe
if "%EXE_ARG%"=="" call :error_exit "Executable name not specified"
call :build_generic "%EXE_ARG%" "Debug" "ON" "FALSE"
goto end_script

:cmd_module
if "%MODULE_ARG%"=="" call :error_exit "Module name not specified"
set "MODULE_NAME=%MODULE_ARG%"
rem MODULE_REBUILD and MODULE_CONFIG are set in the argument parsing loop
if not defined MODULE_REBUILD set "MODULE_REBUILD=ON"
if not defined MODULE_CONFIG set "MODULE_CONFIG=Debug"

call :log_info "Building module: %MODULE_NAME%"
call :log_info "Configuration: %MODULE_CONFIG%, Rebuild dependencies: %MODULE_REBUILD%"

call :build_generic "%MODULE_NAME%" "%MODULE_CONFIG%" "%MODULE_REBUILD%" "FALSE"
goto end_script

:cmd_clean
call :log_info "Cleaning build files..."
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
if exist "exe" (
    del /q "exe\*.exe" 2>nul
    del /q "exe\*.dll" 2>nul
    del /q "exe\*.pdb" 2>nul
)
call :log_success "Cleanup complete"
goto end_script

:cmd_list
call :log_info "Module list:"
if exist "Module" (
    dir /b /s "Module\*.json"
) else (
    call :log_warning "Module directory does not exist"
)
goto end_script

:show_help
echo Usage: build.bat [run|x64|release|exe|module|clean|list|compile_commands] [args]
echo.
echo   run             Build MainEngine (Debug) and run
echo   x64             Build MainEngine (Debug)
echo   release         Build MainEngine (Release)
echo   exe [name]      Build specified executable (e.g. EngineLauncher)
echo   module [name]   Build specified module
echo   clean           Clean build files
echo   compile_commands Generate compile_commands.json for clangd
echo.
goto end_script

:end_script
pause
exit /b 0
