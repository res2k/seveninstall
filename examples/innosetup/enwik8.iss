#define MyAppName "enwik8 compression test data (InnoSetup)"
#define MyAppVersion "1.0"
#define MyAppPublisher "Frank Richter"

[Setup]
AppId={{D620AE9C-A09B-47B2-95CE-C6D8B754D2F6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={pf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=out
OutputBaseFilename=setup
Compression=lzma2/ultra64
LZMAUseSeparateProcess=yes
InternalCompressLevel=ultra64
ShowLanguageDialog=no
DisableWelcomePage=yes
TimeStampsInUTC=true
DisableReadyPage=yes
ExtraDiskSpaceRequired={#100000000-FileSize("..\enwik8.7z")}

[Files]
Source: "..\..\out\Release\SevenInstall.exe"; DestDir: {app}; Attribs: Hidden
Source: "..\enwik8.7z"; DestDir: "{tmp}"; Flags: nocompression deleteafterinstall

#define SEVENINSTALL_GUID   "SevenInstall.Example.InnoSetup"
[Run]
Filename: "{app}\SevenInstall.exe"; Parameters: "install -g{#SEVENINSTALL_GUID} ""-o{app}"" ""{tmp}\enwik8.7z"""; Flags: runhidden
[UninstallRun]
Filename: "{app}\SevenInstall.exe"; Parameters: "remove -g{#SEVENINSTALL_GUID}"; Flags: runhidden

[Code]
procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = wpSelectDir then
    WizardForm.NextButton.Caption := SetupMessage(msgButtonInstall)
  else
    WizardForm.NextButton.Caption := SetupMessage(msgButtonNext);
end;
