@echo off
chcp 65001 >nul 2>nul
setlocal enabledelayedexpansion

:: 自动检测CPU核心数用于并行编译
for /f "tokens=2 delims==" %%i in ('wmic cpu get NumberOfLogicalProcessors /value ^| findstr "NumberOfLogicalProcessors"') do set PARALLEL_CORES=%%i
if not defined PARALLEL_CORES set PARALLEL_CORES=4


goto main

:error_exit
echo Error: %~1
echo.
if not defined NO_PAUSE pause
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

:setup_compiler
call :log_info "Setting up compiler: %COMPILER%"
if /i "%COMPILER%"=="msvc" (
    set GENERATOR_NAME=Visual Studio 18 2026
    set ARCH_ARGS=-A x64
    set BUILD_DIR=build_msvc
    call :log_info "Using MSVC (Visual Studio 2026) generator"
) else if /i "%COMPILER%"=="clang" (
    set GENERATOR_NAME=Ninja
    set ARCH_ARGS=
    set BUILD_DIR=build_clang
    set CMAKE_COMMON_FLAGS=%CMAKE_COMMON_FLAGS% -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
    clang --version > nul 2>&1 || call :error_exit "Clang not found, please ensure clang is installed and added to PATH"
    call :log_info "Using Clang compiler with Ninja generator"
) else if /i "%COMPILER%"=="gcc" (
    set GENERATOR_NAME=Ninja
    set ARCH_ARGS=
    set BUILD_DIR=build_gcc
    set CMAKE_COMMON_FLAGS=%CMAKE_COMMON_FLAGS% -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
    g++ --version > nul 2>&1 || call :error_exit "GCC not found, please ensure g++ is installed and added to PATH"
    call :log_info "Using GCC compiler with Ninja generator"
) else (
    call :log_warning "Unknown compiler '%COMPILER%', defaulting to MSVC"
    set GENERATOR_NAME=Visual Studio 18 2026
    set ARCH_ARGS=-A x64
    set BUILD_DIR=build_msvc
)
goto :eof

:main
set GENERATOR_NAME=Visual Studio 18 2026
set BUILD_DIR=build
set ARCH_ARGS=-A x64
set CMAKE_COMMON_FLAGS=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
set CLEAN_FIRST=
set CMD=
set TARGET_ARG=
set MODULE_CONFIG=Debug
set NO_PAUSE=1
set SHINE_BUILD_EDITOR=ON
set SHINE_BUILD_MODULE=OFF
set BUILD_MODE=editor
set COMPILER=msvc

:parse_args
if "%~1"=="" goto args_done
if /i "%~1"=="--no-editor" (
    set SHINE_BUILD_EDITOR=OFF
    set BUILD_MODE=runtime
    shift
    goto parse_args
)
if /i "%~1"=="--runtime" (
    set SHINE_BUILD_EDITOR=OFF
    set BUILD_MODE=runtime
    shift
    goto parse_args
)
if /i "%~1"=="--editor" (
    set SHINE_BUILD_EDITOR=ON
    set BUILD_MODE=editor
    shift
    goto parse_args
)
if /i "%~1"=="--enable-module" (
    set SHINE_BUILD_MODULE=ON
    shift
    goto parse_args
)
if /i "%~1"=="--no-pause" (
    set NO_PAUSE=1
    shift
    goto parse_args
)
if "%~1"=="--clean-first" (
    set CLEAN_FIRST=--clean-first
    shift
    goto parse_args
)
if "%~1"=="--help" goto show_help
if "%~1"=="-h" goto show_help
if /i "%~1"=="--release" (
    set MODULE_CONFIG=Release
    shift
    goto parse_args
)
if /i "%~1"=="--compiler" (
    set COMPILER=%~2
    shift
    shift
    goto parse_args
)
if /i "%~1"=="--msvc" (
    set COMPILER=msvc
    shift
    goto parse_args
)
if /i "%~1"=="--clang" (
    set COMPILER=clang
    shift
    goto parse_args
)
if /i "%~1"=="--gcc" (
    set COMPILER=gcc
    shift
    goto parse_args
)
if /i "%~1"=="--run" (
    set RUN_AFTER_BUILD=TRUE
    shift
    goto parse_args
)

if defined CMD (
    if /i "%~1"=="release" (
        set MODULE_CONFIG=Release
        shift
        goto parse_args
    )
    if /i "%~1"=="debug" (
        set MODULE_CONFIG=Debug
        shift
        goto parse_args
    )
)

if not defined CMD (
    set CMD=%~1
    if /i "%~1"=="module" set TARGET_ARG=%~2
    if /i "%~1"=="wasm" set TARGET_ARG=%~2
    if /i "%~1"=="exe" set TARGET_ARG=%~2
    if /i "%~1"=="test" set TARGET_ARG=%~2
) else if "%TARGET_ARG%"=="" (
    set TARGET_ARG=%~1
)

shift
goto parse_args

:args_done
set CMAKE_COMMON_FLAGS=%CMAKE_COMMON_FLAGS% -DSHINE_BUILD_EDITOR=%SHINE_BUILD_EDITOR%
set CMAKE_COMMON_FLAGS=%CMAKE_COMMON_FLAGS% -DSHINE_BUILD_MODULE=%SHINE_BUILD_MODULE%

call :setup_compiler

call :log_info "Checking build tools..."
cmake --version > nul 2>&1 || call :error_exit "CMake not found, please ensure CMake is installed and added to PATH"

if "%CMD%"=="run" goto cmd_run
if "%CMD%"=="x64" goto cmd_x64
if "%CMD%"=="release" goto cmd_release
if "%CMD%"=="wasm" goto cmd_wasm
if "%CMD%"=="exe" goto cmd_exe
if "%CMD%"=="module" goto cmd_module
if "%CMD%"=="clean" goto cmd_clean
if "%CMD%"=="list" goto cmd_list
if "%CMD%"=="test" goto cmd_test
if "%CMD%"=="compile_commands" goto cmd_compile_commands
goto show_help

:build_generic
set TARGET_NAME=%~1
set BUILD_CONFIG=%~2
set RUN_AFTER=%~3

if "%TARGET_NAME%"=="" (
    set TARGET_PARAM=
    set TARGET_DISPLAY=All Projects
    set CMAKE_TARGET_MODULE=
) else (
    set TARGET_PARAM=--target %TARGET_NAME%
    set TARGET_DISPLAY=Target %TARGET_NAME%
    set CMAKE_TARGET_MODULE=-DSHINE_TARGET_MODULE=%TARGET_NAME%
)

call :log_info "Build Configuration: %BUILD_CONFIG% - %TARGET_DISPLAY%"
call :log_info "Configuring CMake project..."

cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR_NAME%" %ARCH_ARGS% %CMAKE_COMMON_FLAGS% %CMAKE_TARGET_MODULE% -DREBUILD_DEPS=ON || call :error_exit "CMake configuration failed"

call :log_info "Starting build..."

:: Pre-build cleanup as requested
if "%TARGET_NAME%"=="MainEngine" (
    if exist "exe\MainEngine.exe" (
        call :log_info "Deleting existing exe\MainEngine.exe..."
        del /q "exe\MainEngine.exe"
    )
    if exist "exe\MainEngined.exe" (
        call :log_info "Deleting existing exe\MainEngined.exe..."
        del /q "exe\MainEngined.exe"
    )
)

cmake --build "%BUILD_DIR%" --config "%BUILD_CONFIG%" %TARGET_PARAM% --parallel %CLEAN_FIRST%
if errorlevel 1 call :error_exit "Build failed" & exit /b 1

call :log_success "Build successful!"

if exist "%BUILD_DIR%\compile_commands.json" (
    copy /Y "%BUILD_DIR%\compile_commands.json" "compile_commands.json" > nul 2>&1
    if not errorlevel 1 call :log_info "Copied compile_commands.json for clangd usage"
)

if "%RUN_AFTER%"=="TRUE" call :run_executable "%TARGET_NAME%" "%BUILD_CONFIG%"
goto :eof

:run_executable
set T_NAME=%~1
set T_CONFIG=%~2
if "%T_NAME%"=="" set T_NAME=MainEngine

set SUFFIX=
if /i "%T_CONFIG%"=="Debug" set SUFFIX=d

set FOUND_EXE=
for %%p in (
    "%BUILD_DIR%\exe\%T_NAME%%SUFFIX%.exe"
    "%BUILD_DIR%\exe\%T_NAME%.exe"
    "exe\%T_NAME%%SUFFIX%.exe"
    "exe\%T_NAME%.exe"
) do (
    if not defined FOUND_EXE (
        if exist %%p (
            call :log_info "Running: %%p"
            "%%p"
            set FOUND_EXE=1
        )
    )
)

if not defined FOUND_EXE call :log_warning "Executable not found: %T_NAME%"
goto :eof

:cmd_run
call :build_generic "MainEngine" "Debug" "TRUE"
goto end_script

:cmd_x64
call :build_generic "MainEngine" "Debug" "FALSE"
call :run_executable "MainEngine" "Debug"
goto end_script

:cmd_release
call :build_generic "MainEngine" "Release" "FALSE"
set /p RUN_CHOICE=Run Release version? (y/N): 
if /i "!RUN_CHOICE!"=="y" call :run_executable "MainEngine" "Release"
goto end_script

:cmd_wasm
if "%TARGET_ARG%"=="" set TARGET_ARG=smallwasm
call :log_info "Building wasm target (%TARGET_ARG%) ..."

if exist "%BUILD_DIR%\CMakeCache.txt" (
    call :log_info "Using existing build directory: %BUILD_DIR%"
    cmake --build "%BUILD_DIR%" --config "%MODULE_CONFIG%" --target "%TARGET_ARG%" --parallel %CLEAN_FIRST% || call :error_exit "Build failed"
    call :log_success "Build successful!"
) else (
    call :build_generic "%TARGET_ARG%" "%MODULE_CONFIG%" "FALSE"
)
goto end_script

:cmd_exe
if "%TARGET_ARG%"=="" call :error_exit "Executable name not specified"
call :build_generic "%TARGET_ARG%" "%MODULE_CONFIG%" "%RUN_AFTER_BUILD%"
goto end_script

:cmd_module
if "%TARGET_ARG%"=="" call :error_exit "Module name not specified"
call :log_info "Building module: %TARGET_ARG%"
call :log_info "Configuration: %MODULE_CONFIG%, Rebuild dependencies: ON"
call :build_generic "%TARGET_ARG%" "%MODULE_CONFIG%" "FALSE"
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

:cmd_test
if "%TARGET_ARG%"=="" set TARGET_ARG=TestRunner
set CMAKE_COMMON_FLAGS=%CMAKE_COMMON_FLAGS% -DBUILD_TESTING=ON
call :build_generic "%TARGET_ARG%" "%MODULE_CONFIG%" "TRUE"
goto end_script

:cmd_compile_commands
cmake -B "%BUILD_DIR%" -S . -G "%GENERATOR_NAME%" %ARCH_ARGS% %CMAKE_COMMON_FLAGS% -DREBUILD_DEPS=ON || call :error_exit "CMake configuration failed"
if exist "%BUILD_DIR%\compile_commands.json" (
    copy /Y "%BUILD_DIR%\compile_commands.json" "compile_commands.json" > nul 2>&1
    if not errorlevel 1 call :log_info "Copied compile_commands.json for clangd usage"
)
goto end_script

:show_help
echo Usage: build.bat [run^|x64^|release^|wasm^|exe^|module^|clean^|list^|test^|compile_commands] [args]
echo.
echo   run             Build MainEngine (Debug) and run
echo   x64             Build MainEngine (Debug)
echo   release         Build MainEngine (Release)
echo   wasm [name]     Build wasm target (default: smallwasm; add --release for Release)
echo   exe [name]      Build specified executable (e.g. EngineLauncher)
echo   module [name]   Build specified module
echo   test            Build and run tests
echo   clean           Clean build files
echo   compile_commands Generate compile_commands.json for clangd
echo.
echo Global flags:
echo   --release        Use Release config (for module/wasm/test)
echo   --run            Run executable after building (for exe command)
echo   --no-pause       Do not pause at script end (CI-friendly)
echo   --no-editor      Disable editor features (removes BUILD_EDITOR macro)
echo   --editor         Enable editor mode (default, same as without flag)
echo   --runtime        Build in runtime mode (excludes editor-only modules)
echo   --enable-module  Enable C++20 modules (adds SHINE_BUILD_MODULE macro)
echo.
echo Compiler selection:
echo   --msvc           Use MSVC compiler (default)
echo   --clang          Use Clang compiler (requires Ninja and clang in PATH)
echo   --gcc            Use GCC compiler (requires Ninja and g++ in PATH)
echo   --compiler NAME  Specify compiler: msvc, clang, or gcc
echo.
echo Examples:
echo   build.bat run                    Build and run with MSVC (default)
echo   build.bat run --clang            Build and run with Clang
echo   build.bat module MyMod --gcc     Build module with GCC
echo   build.bat release --clang        Build release with Clang
echo.
echo Examples:
echo   build.bat exe EngineLauncher          - Build EngineLauncher
echo   build.bat exe EngineLauncher --run    - Build and run EngineLauncher
echo.
goto end_script

:end_script
if not defined NO_PAUSE pause
exit /b 0