; Windows installer for AVM EMON. Built by .gitlab-ci.yml on the "windows" runner:
;   ISCC /DMyAppVersion=1.2.3 /DDeployDir=<dir with exe + windeployqt output> /DOutDir=<dist> avm-emon.iss
#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef DeployDir
  #define DeployDir "..\..\deploy"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist"
#endif
#define Name "AVM EMON"
#define ExeName "avm-emon.exe"

[Setup]
AppId={{B3A91C57-4E08-4D6F-8A2D-7F15C0E64B93}
AppName={#Name}
AppVersion={#MyAppVersion}
AppVerName={#Name} {#MyAppVersion}
AppPublisher=AVM-Energo LLC
AppPublisherURL=https://www.avmenergo.ru
DefaultDirName={autopf}\AVM-EMON
DefaultGroupName={#Name}
UsedUserAreasWarning=no
Compression=lzma
SolidCompression=yes
WizardStyle=modern
OutputBaseFilename=avm-emon-setup-{#MyAppVersion}
OutputDir={#OutDir}
VersionInfoVersion={#MyAppVersion}
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#DeployDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#Name}"; Filename: "{app}\{#ExeName}"
Name: "{group}\{cm:UninstallProgram,{#Name}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#Name}"; Filename: "{app}\{#ExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; Flags: unchecked

[Run]
Filename: "{app}\{#ExeName}"; Description: "{cm:LaunchProgram,{#Name}}"; Flags: nowait postinstall skipifsilent
