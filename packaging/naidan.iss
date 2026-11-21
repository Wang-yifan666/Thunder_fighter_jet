; Naidan 的 Inno Setup 安装脚本。
;
; 设计要点：
; - 每用户安装（PrivilegesRequired=lowest），装到 %LOCALAPPDATA%\Programs\Naidan，
;   全程不需要管理员权限，也不会触发 UAC。
; - 自带卸载程序，开始菜单有入口，桌面图标是可选项。
; - 中英双语；简体中文语言包放在 packaging\languages\ 随仓库分发。
; - 源文件目录由 /DSourceDir=... 传入（tools\build_installer.ps1 负责准备），
;   默认值指向 build\package。

#define AppNameZh "奶蛋的火力不足恐惧症"
#define AppNameEn "Naidan's Fear of Insufficient Firepower"
; 版本由 tools\build_installer.ps1 的 -Version 传入（CI 从 tag 取数字部分），
; 直接编译本文件时用下面的默认值。
#ifndef AppVersion
  #define AppVersion "1.8"
#endif
#define AppExeName "Naidan.exe"
#define AppPublisher "Wang-yifan666"
#define AppUrl "https://github.com/Wang-yifan666/Thunder_fighter_jet"

#ifndef SourceDir
  #define SourceDir "..\build\package"
#endif

[Setup]
; AppId 必须稳定，升级时才能正确覆盖安装而不是并存
AppId={{8598AEE3-C43E-4B9B-847A-FD9B5A9977B2}
AppName={#AppNameZh}
AppVersion={#AppVersion}
AppVerName={#AppNameZh} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}
AppUpdatesURL={#AppUrl}/releases
DefaultDirName={localappdata}\Programs\Naidan
DefaultGroupName=Naidan
DisableProgramGroupPage=yes
; 每用户安装：不需要管理员权限
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=..\build\installer
OutputBaseFilename=Naidan-{#AppVersion}-setup
SetupIconFile=..\assets\icons\app.ico
UninstallDisplayName={#AppNameZh}
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; 允许用户改安装目录（便携用户的习惯）
AllowNoIcons=yes

; 版本资源：FileVersion 留空会让部分安全软件把安装包当"来路不明的封装器"，
; 这里把四项版本信息补全（Inno 默认只写 ProductVersion）。
VersionInfoVersion={#AppVersion}.0.0
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppNameEn} Setup
VersionInfoCopyright={#AppPublisher}
VersionInfoProductName={#AppNameZh}
VersionInfoProductVersion={#AppVersion}

[Languages]
; 第一个语言是默认语言
Name: "chinesesimplified"; MessagesFile: "languages\ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppNameZh}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{cm:UninstallProgram,{#AppNameZh}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppNameZh}"; Filename: "{app}\{#AppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#AppNameZh}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 卸载时清掉运行时可能残留的便携存档（用户数据目录里的不动）
Type: files; Name: "{app}\highscore.txt"