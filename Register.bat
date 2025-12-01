@echo off
echo Setting up MainEngined project file association...

REM 获取当前目录的完整路径
set "ENGINE_DIR=%~dp0"
set "ENGINE_EXE=%ENGINE_DIR%exe\MainEngined.exe"

REM 检查引擎是否存在
if not exist "%ENGINE_EXE%" (
    echo Error: MainEngined.exe not found in %ENGINE_DIR%exe\
    echo Please build the engine first.
    pause
    exit /b 1
)

REM 创建文件类型关联
reg add "HKEY_CLASSES_ROOT\.SProject" /ve /d "ShineEngine.Project" /f

REM 创建文件类型描述
reg add "HKEY_CLASSES_ROOT\ShineEngine.Project" /ve /d "ShineEngine Project File" /f
reg add "HKEY_CLASSES_ROOT\ShineEngine.Project\DefaultIcon" /ve /d "\"%ENGINE_EXE%\",0" /f

REM 设置打开命令 - 直接使用 MainEngine.exe
reg add "HKEY_CLASSES_ROOT\ShineEngine.Project\shell\open\command" /ve /d "\"%ENGINE_EXE%\" \"%%1\"" /f

REM 设置默认程序
reg add "HKEY_CLASSES_ROOT\ShineEngine.Project\shell" /ve /d "open" /f

echo File association setup complete!
echo You can now double-click .SProject files to open them directly with ShineEngine.
pause