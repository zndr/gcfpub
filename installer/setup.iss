; ============================================================================
; MilleWin CF Extractor - Inno Setup Script
; ============================================================================
; Questo script crea un installer per MilleWin CF Extractor
; con supporto per avvio automatico tramite collegamento Startup
; ============================================================================

#define MyAppName "MilleWin CF Extractor"
#define MyAppVersion "1.3.6"
#define MyAppPublisher "MWCFExtractor"
#define MyAppExeName "mwcf_extractor.exe"
#define MyAppId "MWCFExtractor"

[Setup]
; ID univoco dell'applicazione (GUID)
AppId={{B5E8F7D2-3A4C-4B5E-9F6A-1234567890AB}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
; Percorso del file exe compilato (relativo a questo script)
OutputDir=..\build\installer
OutputBaseFilename=MWCFExtractor_Setup_{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
; Richiedi privilegi admin per scrivere in Program Files e creare task
PrivilegesRequired=admin
; Architettura
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Icona
SetupIconFile=..\res\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
; Informazioni versione
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Setup
VersionInfoCopyright=Copyright (C) 2024 {#MyAppPublisher}
; Disabilita la pagina di selezione cartella per utenti non esperti
DisableDirPage=auto
; Wizard
WizardStyle=modern
; Chiudi automaticamente l'applicazione se in esecuzione
CloseApplications=force
CloseApplicationsFilter=*.exe

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
italian.BeveledLabel=MilleWin CF Extractor
english.BeveledLabel=MilleWin CF Extractor

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Avvia automaticamente all'accesso di Windows"; GroupDescription: "Opzioni:"; Flags: checkedonce

[Files]
; File principale
Source: "..\build\bin\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Icona nel Menu Start
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Disinstalla {#MyAppName}"; Filename: "{uninstallexe}"
; Icona sul Desktop (opzionale)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
; Collegamento nella cartella Startup (per avvio automatico)
Name: "{userstartup}\MWCFExtractor"; Filename: "{app}\{#MyAppExeName}"; Tasks: autostart

[Registry]
; Chiave per identificare l'installazione
Root: HKLM; Subkey: "SOFTWARE\{#MyAppId}"; Flags: uninsdeletekeyifempty
Root: HKLM; Subkey: "SOFTWARE\{#MyAppId}"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletevalue

[Run]
; Avvia l'applicazione dopo l'installazione (opzionale)
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Rimuovi il collegamento dalla cartella Startup
Type: files; Name: "{userstartup}\MWCFExtractor.lnk"
; Rimuovi la cartella di configurazione in AppData
Type: filesandordirs; Name: "{userappdata}\{#MyAppId}"

[Code]
// Import MessageBox da user32.dll per titolo personalizzato
function MessageBoxW(hWnd: Integer; lpText, lpCaption: String; uType: Cardinal): Integer;
external 'MessageBoxW@user32.dll stdcall';

// Funzione per verificare se Millewin e' installato nel sistema
function IsMillewinInstalled(): Boolean;
var
  KeyExists: Boolean;
begin
  Result := False;

  // Cerca nel Registry le chiavi di installazione di Millewin
  KeyExists := RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Millennium\Millewin');
  if KeyExists then
  begin
    Result := True;
    Exit;
  end;

  KeyExists := RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Millennium\Millewin');
  if KeyExists then
  begin
    Result := True;
    Exit;
  end;

  KeyExists := RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Dedalus\Millewin');
  if KeyExists then
  begin
    Result := True;
    Exit;
  end;

  KeyExists := RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Dedalus\Millewin');
  if KeyExists then
  begin
    Result := True;
    Exit;
  end;

  // Cerca l'eseguibile in percorsi comuni
  if FileExists('C:\Millewin\millewin.exe') then
  begin
    Result := True;
    Exit;
  end;

  if FileExists('C:\Program Files\Millewin\millewin.exe') then
  begin
    Result := True;
    Exit;
  end;

  if FileExists('C:\Program Files (x86)\Millewin\millewin.exe') then
  begin
    Result := True;
    Exit;
  end;

  if FileExists('C:\Programmi\Millewin\millewin.exe') then
  begin
    Result := True;
    Exit;
  end;
end;

// Funzione chiamata all'avvio dell'installazione
function InitializeSetup(): Boolean;
begin
  Result := True;

  // Verifica che Millewin sia installato
  if not IsMillewinInstalled() then
  begin
    // MB_OK = 0, MB_ICONERROR = $10
    MessageBoxW(0,
      'MWCF-Extractor e'' utilizzabile solo se Millewin e'' installato nel sistema.',
      'Millewin non trovato',
      $10);
    Result := False;
  end;
end;

// Funzione per verificare se l'applicazione e' in esecuzione
function IsAppRunning(): Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  // Usa tasklist per verificare se il processo e' in esecuzione
  if Exec('cmd.exe', '/c tasklist /FI "IMAGENAME eq {#MyAppExeName}" | find /I "{#MyAppExeName}"',
          '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Result := (ResultCode = 0);
  end;
end;

// Funzione chiamata prima dell'installazione
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
  WinDir: String;
begin
  Result := '';

  // Termina sempre il processo se esiste (non verificare prima, termina direttamente)
  // Usa PowerShell che e' piu' affidabile di taskkill
  WinDir := ExpandConstant('{sys}');
  Exec(WinDir + '\WindowsPowerShell\v1.0\powershell.exe',
       '-NoProfile -ExecutionPolicy Bypass -Command "Stop-Process -Name ''mwcf_extractor'' -Force -ErrorAction SilentlyContinue"',
       '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(500);

  // Secondo tentativo con taskkill come fallback
  Exec(WinDir + '\taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Sleep(500);
end;

// Funzione chiamata prima della disinstallazione
function InitializeUninstall(): Boolean;
var
  ResultCode: Integer;
begin
  Result := True;

  // Se l'applicazione e' in esecuzione, terminala
  if IsAppRunning() then
  begin
    Exec('taskkill.exe', '/F /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    Sleep(1000);
  end;
end;

// Pulisci la voce legacy dal Registry durante l'installazione
procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    // Rimuovi eventuale voce autostart legacy dal Registry
    RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'MWCFExtractor');
  end;
end;
