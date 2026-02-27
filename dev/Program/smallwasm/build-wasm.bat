@echo off

setlocal EnableExtensions EnableDelayedExpansion



REM Always run relative to this .bat location

set "ROOT=%~dp0"

REM Trim trailing backslash for nicer paths (optional)

if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"



set "BUILD_DIR=%ROOT%\build-wasm"

set "SMALLWASM_ANALYZE=ON"
set "SMALLWASM_DEBUG_WEB=ON"

set "CMAKE_ARGS="
:parse_args
if "%~1"=="" goto after_parse_args
if /I "%~1"=="--debug" (
  set "SMALLWASM_ANALYZE=ON"
  set "SMALLWASM_DEBUG_WEB=ON"
  shift
  goto parse_args
)
if /I "%~1"=="--release" (
  set "SMALLWASM_ANALYZE=OFF"
  set "SMALLWASM_DEBUG_WEB=OFF"
  shift
  goto parse_args
)
set "CMAKE_ARGS=!CMAKE_ARGS! %1"
shift
goto parse_args
:after_parse_args
set "CMAKE_ARGS=!CMAKE_ARGS! -DSMALLWASM_ANALYZE=%SMALLWASM_ANALYZE% -DSMALLWASM_DEBUG_WEB=%SMALLWASM_DEBUG_WEB%"


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

cmake -S "%ROOT%" -B "%BUILD_DIR%" -G Ninja %CMAKE_ARGS%

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



REM Report wasm size and compare to last build

set "WASM_PATH=%ROOT%\web\dist\output.gl.wasm"
set "WASM_SIZE_FILE=%ROOT%\web\dist\output.gl.wasm.size"

if exist "%WASM_PATH%" (

  for %%I in ("%WASM_PATH%") do set "WASM_SIZE=%%~zI"

  set "WASM_DELTA=0"
  if exist "%WASM_SIZE_FILE%" (
    set /p "WASM_PREV_SIZE="<"%WASM_SIZE_FILE%"
    for /f "delims=0123456789" %%N in ("!WASM_PREV_SIZE!") do set "WASM_PREV_SIZE="
    if defined WASM_PREV_SIZE (
      if !WASM_SIZE! GTR !WASM_PREV_SIZE! (
        set "WASM_DELTA=100"
      ) else if !WASM_SIZE! LSS !WASM_PREV_SIZE! (
        set "WASM_DELTA=-100"
      )
    )
  )

  echo [smallwasm] output.gl.wasm size = !WASM_SIZE! bytes
  echo [smallwasm] size delta = !WASM_DELTA!

  >"%WASM_SIZE_FILE%" echo !WASM_SIZE!

) else (

  echo [smallwasm] WARNING: "%WASM_PATH%" not found.

)



echo [smallwasm] OK

exit /b 0

