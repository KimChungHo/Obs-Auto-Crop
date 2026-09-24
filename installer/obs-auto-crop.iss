#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

[Setup]
AppId=OBS-Auto-Crop
AppName=OBS Auto Crop
AppVersion={#MyAppVersion}
AppPublisher=OBS Auto Crop
DefaultDirName={autopf}\obs-studio
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=..\release
OutputBaseFilename=obs-auto-crop-{#MyAppVersion}-windows-x64-setup
UninstallFilesDir={app}\data\obs-plugins\obs-auto-crop\uninstall
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
Compression=lzma2
SolidCompression=yes

[Files]
; OBS loads these paths relative to the selected OBS installation directory.
Source: "..\stage\plugins\obs-auto-crop\bin\64bit\obs-auto-crop.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "..\stage\plugins\obs-auto-crop\data\*"; DestDir: "{app}\data\obs-plugins\obs-auto-crop"; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
function IsObsDirectory(const Directory: String): Boolean;
begin
  Result := FileExists(AddBackslash(Directory) + 'bin\64bit\obs64.exe');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (CurPageID = wpSelectDir) and not IsObsDirectory(WizardDirValue) then
  begin
    MsgBox('Select the OBS Studio installation folder containing bin\64bit\obs64.exe.', mbError, MB_OK);
    Result := False;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if not IsObsDirectory(ExpandConstant('{app}')) then
    Result := 'The selected folder is not an OBS Studio installation. Select the folder containing bin\64bit\obs64.exe.';
end;
