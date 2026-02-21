[Setup]
AppName=ShineEngine Launcher
AppVersion=1.0.0
AppPublisher=ShineEngine Team
AppSupportURL=https://github.com/shineengine
AppUpdatesURL=https://github.com/shineengine
DefaultDirName={autopf}\ShineEngine
DefaultGroupName=ShineEngine
AllowNoIcons=yes
OutputBaseFilename=ShineEngineLauncher
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
DisableProgramGroupPage=no
UsePreviousAppDir=no
UsePreviousGroup=no

[Languages]
Name: "chinese"; MessagesFile: "compiler:Languages\chinese.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "快捷方式:"; Flags: unchecked
Name: "envvar"; Description: "添加到 SHINEENGINE_ROOT 环境变量"; GroupDescription: "环境设置:"; Flags: checkedonce

[Files]
; Engine Launcher executable
Source: "exe\EngineLauncherd.exe"; DestDir: "{app}\Launcher"; Flags: ignoreversion

; Required DLLs for launcher
Source: "exe\glew32d.dll"; DestDir: "{app}\Launcher"; Flags: ignoreversion
Source: "exe\fmtd.lib"; DestDir: "{app}\Launcher"; Flags: ignoreversion
Source: "exe\imguid.lib"; DestDir: "{app}\Launcher"; Flags: ignoreversion
Source: "exe\log_uid.lib"; DestDir: "{app}\Launcher"; Flags: ignoreversion
Source: "exe\file_utild.lib"; DestDir: "{app}\Launcher"; Flags: ignoreversion

; Launcher config
Source: "exe\imgui.ini"; DestDir: "{app}\Launcher"; Flags: ignoreversion



[Icons]
Name: "{group}\ShineEngine Launcher"; Filename: "{app}\Launcher\EngineLauncher.exe"
Name: "{group}\Uninstall ShineEngine Launcher"; Filename: "{uninstallexe}"
Name: "{commondesktop}\ShineEngine Launcher"; Filename: "{app}\Launcher\EngineLauncher.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\Launcher\EngineLauncher.exe"; Description: "Launch ShineEngine"; Flags: postinstall shellexec

[UninstallRun]
; Clean up file associations
Filename: "reg"; Parameters: "delete ""HKEY_CLASSES_ROOT\.SProject"" /f"; RunOnceId: "RemoveFileAssoc"
Filename: "reg"; Parameters: "delete ""HKEY_CLASSES_ROOT\ShineEngine.Project"" /f"; RunOnceId: "RemoveProjectType"

[Code]

var
  ErrorCode: Integer;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Register file associations
    try
      RegWriteStringValue(HKEY_CLASSES_ROOT, '.SProject', '', 'ShineEngine.Project');
      RegWriteStringValue(HKEY_CLASSES_ROOT, 'ShineEngine.Project', '', 'ShineEngine Project File');
      RegWriteStringValue(HKEY_CLASSES_ROOT, 'ShineEngine.Project\DefaultIcon', '', ExpandConstant('{app}\Launcher\EngineLauncher.exe,0'));
      RegWriteStringValue(HKEY_CLASSES_ROOT, 'ShineEngine.Project\shell\open\command', '', ExpandConstant('"{app}\Launcher\EngineLauncher.exe" "%1"'));
      RegWriteStringValue(HKEY_CLASSES_ROOT, 'ShineEngine.Project\shell', '', 'open');
    except
    end;

    // Register environment variable if user chose to
    if WizardIsTaskSelected('envvar') then
    begin
      // Set SHINEENGINE_ROOT for current user
      RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'SHINEENGINE_ROOT', ExpandConstant('{app}'));

      // Also set SHINEENGINE_LAUNCHER for convenience
      RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'SHINEENGINE_LAUNCHER', ExpandConstant('{app}\Launcher'));

      // Notify explorer to refresh environment
      ShellExec('runas', 'reg', 'add "HKCU\Environment" /v "SHINEENGINE_ROOT" /t REG_SZ /d "' + ExpandConstant('{app}') + '" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
      ShellExec('runas', 'reg', 'add "HKCU\Environment" /v "SHINEENGINE_LAUNCHER" /t REG_SZ /d "' + ExpandConstant('{app}\Launcher') + '" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);

      // Broadcast environment change
      ShellExec('runas', 'reg', 'add "HKU\.DEFAULT\Environment" /v "SHINEENGINE_ROOT" /t REG_SZ /d "' + ExpandConstant('{app}') + '" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
    end;
  end;
end;

procedure DeinitializeUninstall();
var
  ErrorCode: Integer;
begin
  // Clean up file associations
  ShellExec('runas', 'reg', 'delete "HKEY_CLASSES_ROOT\.SProject" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  ShellExec('runas', 'reg', 'delete "HKEY_CLASSES_ROOT\ShineEngine.Project" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);

  // Clean up environment variables
  ShellExec('runas', 'reg', 'delete "HKCU\Environment" /v "SHINEENGINE_ROOT" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  ShellExec('runas', 'reg', 'delete "HKCU\Environment" /v "SHINEENGINE_LAUNCHER" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);

  // Clean up .DEFAULT environment variables
  ShellExec('runas', 'reg', 'delete "HKU\.DEFAULT\Environment" /v "SHINEENGINE_ROOT" /f', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
end;
