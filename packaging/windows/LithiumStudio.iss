; Lithium Studio installer for Windows (Inno Setup 6 or 7), in the style of
; Lithium Drums' installer.
;
; packaging/windows/build-installer.ps1 builds the app, stages it (with
; build-zip.ps1) and compiles this with
;   ISCC /DAppVersion=<version> /DStaging=<staged folder> /DOutputDir=<output folder> LithiumStudio.iss
; The staged folder holds Lithium Studio.exe, Sounds\, LICENSE.txt and CREDITS.txt.
;
; The wizard: welcome, licence, where to install, a desktop shortcut and the
; session file types, install, and a finish page that offers to open the app.
; The artwork (installer-*.bmp, ..\..\resources\icons\LithiumStudio.ico) is
; drawn by tools/make_installer_art.py.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef Staging
  #define Staging "..\..\build-zip\Lithium Studio"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

#define AppName "Lithium Studio"
#define AppExe "Lithium Studio.exe"
#define Company "Calamity Audio"
#define Author "Isaac Joseph"
#define Website "https://github.com/thecalamityjoe87"
#define ProgId "LithiumStudio.Session"

[Setup]
; Never change AppId: Windows uses it to find an earlier install to upgrade.
AppId={{4A54A19C-3963-4E12-B60D-0A524533EDCC}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#Company}
AppPublisherURL={#Website}
AppSupportURL={#Website}
AppCopyright=Copyright (C) 2026 {#Author}
VersionInfoVersion={#AppVersion}
DefaultDirName={autopf}\{#Company}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
LicenseFile={#Staging}\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename=Lithium_Studio-{#AppVersion}-windows-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; For everyone (Program Files) by default; the wizard also offers "only for
; me", which needs no admin rights. {autopf} and HKA follow that choice.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppExe}
SetupIconFile=..\..\resources\icons\LithiumStudio.ico
WizardImageFile=installer-side.bmp,installer-side-2x.bmp
WizardSmallImageFile=installer-small.bmp,installer-small-2x.bmp

[Messages]
WelcomeLabel2=This installs [name/ver] by {#Company}.%n%nLithium Studio is a music studio: record instruments and vocals, play virtual instruments, arrange tracks, add effects, mix, and export your songs.%n%nIt comes with a small set of sounds. More sound packs can be downloaded from inside the app (Sounds > Manage Sounds).
FinishedLabel=Lithium Studio is installed.%n%nOpen it from the Start menu, or double-click a Lithium Studio session (.lisession).

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "associate"; Description: "Session files handler: double-click a session (.lisession) to open it in Lithium Studio"

[Files]
Source: "{#Staging}\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion
; The bundled sound pack: the app looks for it next to itself.
Source: "{#Staging}\Sounds\*"; DestDir: "{app}\Sounds"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Staging}\LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#Staging}\CREDITS.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; The session file types. Older sessions (.iksession, saved by Ikaros Studio)
; open as they are.
Root: HKA; Subkey: "Software\Classes\.lisession"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.iksession"; ValueType: string; ValueName: ""; ValueData: "{#ProgId}"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\{#ProgId}"; ValueType: string; ValueName: ""; ValueData: "Lithium Studio session"; Flags: uninsdeletekey; Tasks: associate
Root: HKA; Subkey: "Software\Classes\{#ProgId}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExe},0"; Tasks: associate
Root: HKA; Subkey: "Software\Classes\{#ProgId}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""; Tasks: associate

[Run]
Filename: "{app}\{#AppExe}"; Description: "Open Lithium Studio now"; Flags: postinstall nowait skipifsilent unchecked

; Uninstalling removes only what the installer put there. Sessions, sound
; packs and settings (in Documents, %APPDATA% and %LOCALAPPDATA%) stay.
