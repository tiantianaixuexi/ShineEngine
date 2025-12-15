@echo off

setlocal EnableExtensions EnableDelayedExpansion



REM Always run relative to this .bat location

set "ROOT=%~dp0"

REM Trim trailing backslash for nicer paths (optional)

if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"



set "BUILD_DIR=%ROOT%\build-wasm"



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

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja -DSMALLWASM_DEBUG=ON %*

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

