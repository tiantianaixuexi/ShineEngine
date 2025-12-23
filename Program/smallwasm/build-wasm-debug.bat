@echo off

setlocal EnableExtensions EnableDelayedExpansion



REM Always run relative to this .bat location

set "ROOT=%~dp0"

REM Trim trailing backslash for nicer paths (optional)

if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"



set "BUILD_DIR=%ROOT%\build-wasm"

set "SMALLWASM_DEBUG=ON"
set "CMAKE_ARGS="
:parse_args
if "%~1"=="" goto after_parse_args
if /I "%~1"=="--debug" (
  set "SMALLWASM_DEBUG=ON"
  shift
  goto parse_args
)
if /I "%~1"=="--nodebug" (
  set "SMALLWASM_DEBUG=OFF"
  shift
  goto parse_args
)
if /I "%~1"=="--debug=ON" (
  set "SMALLWASM_DEBUG=ON"
  shift
  goto parse_args
)
if /I "%~1"=="--debug=OFF" (
  set "SMALLWASM_DEBUG=OFF"
  shift
  goto parse_args
)
set "CMAKE_ARGS=!CMAKE_ARGS! %1"
shift
goto parse_args
:after_parse_args


echo [smallwasm] root      = "%ROOT%"

echo [smallwasm] build dir = "%BUILD_DIR%"



REM Clean build cache

if exist "%BUILD_DIR%\" (

  echo [smallwasm] cleaning "%BUILD_DIR%" ...

  rmdir /s /q "%BUILD_DIR%"

  if exist "%BUILD_DIR%\" (

    echo [smallwasm] ERROR: failed to remove build dir.

    exit /b 1

  )

)



REM Configure

echo [smallwasm] configuring ...

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DSMALLWASM_DEBUG=%SMALLWASM_DEBUG% %CMAKE_ARGS%

if errorlevel 1 (

  echo [smallwasm] ERROR: cmake configure failed.

  exit /b 1

)



REM Build

echo [smallwasm] building ...

cmake --build "%BUILD_DIR%"

if errorlevel 1 (

  echo [smallwasm] ERROR: build failed.

  exit /b 1

)



echo [smallwasm] OK

exit /b 0

