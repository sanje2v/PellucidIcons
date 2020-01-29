; Script for installing 'PellucidIcons'

#define MyAppName "Pellucid Icons"
#define MyAppVersion "1.0"
#define MyAppPublisher "Sanjeev Sharma"
#define MyAppURL "http://sanje2v.wordpress.com/"

; NOTE: Un/comment the following lines to produce setup for different archs
;#define X86
#ifndef X86
#define X64
#endif

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{BBB60B71-54FB-4FCB-8537-689BEC7256B3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
CreateAppDir=no
Compression=lzma
SolidCompression=yes
#ifdef X86
OutputBaseFilename=PellucidIconsSetup_x86
#else
OutputBaseFilename=PellucidIconsSetup_x64
; Use full 64-bit mode when running in 64-bit systems
ArchitecturesInstallIn64BitMode=x64
#endif
; This setup requires administrative privileges
PrivilegesRequired=admin
; Minimum OS version is Windows 8
MinVersion=6.2
; Only 32-bit and 64-bit architecture are supported, no Itanium support
ArchitecturesAllowed=x86 x64
; Explorer.exe only loads icon overlay handlers on next restart
AlwaysRestart=true

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
#ifdef X86

Source:"..\x64\Release\PellucidIcons.dll";DestDir:"{sys}";Flags:regserver uninsrestartdelete

#else
; X64

Source:"..\x64\Release\PellucidIcons.dll";DestDir:"{sys}";Flags:regserver uninsrestartdelete

#endif

[Registry]
; Enable Pellucid Icons for current user, other users need to enable it manually
Root: HKCU; Subkey: "SOFTWARE\PellucidIcons\Settings";
Root: HKCU; Subkey: "SOFTWARE\PellucidIcons\Settings"; ValueType:dword; ValueName:"Enabled"; ValueData:1

[Code]
function InitializeSetup(): Boolean;

var ErrorCode: Integer;
begin
  (*
    Check for Visual C++ 2015 redistributable dll libraries. They need to be installed
    before the setup begins installing because registering 'PellucidIcons.dll' requires them. 
  *)
  Result := true;

#ifdef X86
  if IsWin64() then begin
    Msgbox('This installer is meant for 32-bit systems. Please download the installer meant for 64-bit architecture.', mbCriticalError, MB_OK);
    Result := false;
  end else
#else
  if not IsWin64() then begin
    Msgbox('This installer is meant for 64-bit systems. Please download the installer meant for 32-bit architecture.', mbCriticalError, MB_OK);
    Result := false;
  end else
#endif
  if IsWin64() then begin
    (* For 64-bit Windows, we need x64 versions *)
    if not RegValueExists(HKLM, 'SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed') then begin
        Result := false;
        MsgBox('Installation of 64-bit Visual C++ 2015 dlls are required. Redirecting your browser to the download site. Please install them and try running this setup again.', mbInformation, MB_OK);
        ShellExec('open', 'https://www.microsoft.com/en-us/download/details.aspx?id=48145', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
    end 
  end else
    (* For 32-bit Windows, we need x86 versions *)
    if not RegValueExists(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86', 'Installed') then begin
        Result := false;
        MsgBox('Installation of 32-bit Visual C++ 2015 dlls are required. Redirecting your browser to the download site. Please install them and try running this setup again.', mbInformation, MB_OK);
        ShellExec('open', 'https://www.microsoft.com/en-us/download/details.aspx?id=48145', '', '', SW_SHOWNORMAL, ewNoWait, ErrorCode);
    end
end;