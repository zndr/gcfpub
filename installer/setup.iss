; ============================================================================
; MilleWin CF Extractor - Inno Setup Script
; ============================================================================
; Questo script crea un installer per MilleWin CF Extractor
; con supporto per avvio automatico tramite collegamento Startup
; ============================================================================

#define MyAppName "MilleWin CF Extractor"
#define MyAppVersion "1.3.10"
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
CloseApplicationsFilter=mwcf_extractor.exe

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

// Funzione per verificare se l'applicazione e' in esecuzione
function IsAppRunning(): Boolean;
var
  ResultCode: Integer;
begin
  Result := False;
  if Exec('cmd.exe', '/c tasklist /FI "IMAGENAME eq {#MyAppExeName}" 2>nul | find /I "{#MyAppExeName}" >nul',
          '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    Result := (ResultCode = 0);
  end;
end;

// Funzione per terminare l'applicazione con retry e verifica
// Ritorna True se l'app e' stata terminata o non era in esecuzione
function KillAppWithRetry(MaxAttempts: Integer; ShowError: Boolean): Boolean;
var
  ResultCode: Integer;
  Attempts: Integer;
begin
  Result := True;
  Attempts := 0;

  while IsAppRunning() and (Attempts < MaxAttempts) do
  begin
    Attempts := Attempts + 1;
    Log('KillAppWithRetry: Tentativo ' + IntToStr(Attempts) + ' di ' + IntToStr(MaxAttempts));

    if Attempts = 1 then
    begin
      // Primo tentativo: chiusura graceful (senza /F)
      Exec('taskkill.exe', '/IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
      Log('KillAppWithRetry: taskkill graceful, ResultCode=' + IntToStr(ResultCode));
      Sleep(1500);
    end
    else
    begin
      // Tentativi successivi: forza chiusura con /F e /T (termina processi figli)
      Exec('taskkill.exe', '/F /T /IM {#MyAppExeName}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
      Log('KillAppWithRetry: taskkill /F /T, ResultCode=' + IntToStr(ResultCode));
      Sleep(1000);
    end;
  end;

  // Verifica finale
  if IsAppRunning() then
  begin
    Result := False;
    Log('KillAppWithRetry: FALLITO - processo ancora in esecuzione dopo ' + IntToStr(Attempts) + ' tentativi');

    if ShowError then
    begin
      MessageBoxW(0,
        'Non e'' stato possibile chiudere automaticamente MilleWin CF Extractor.' + #13#10 + #13#10 +
        'Questo puo'' accadere se:' + #13#10 +
        '- L''applicazione e'' stata avviata con privilegi elevati' + #13#10 +
        '- Un antivirus sta proteggendo il processo' + #13#10 + #13#10 +
        'Chiudi manualmente l''applicazione dalla system tray e riprova.',
        'Impossibile chiudere l''applicazione',
        $10); // MB_ICONERROR
    end;
  end
  else
  begin
    Log('KillAppWithRetry: OK - processo terminato dopo ' + IntToStr(Attempts) + ' tentativi');
  end;
end;

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

  // PRIMA DI TUTTO: termina l'applicazione se in esecuzione
  // Usa 3 tentativi, non mostrare errore qui (lo faremo in PrepareToInstall se necessario)
  if not KillAppWithRetry(3, False) then
  begin
    Log('InitializeSetup: Prima terminazione fallita, riprovera'' in PrepareToInstall');
  end;

  // Verifica che Millewin sia installato
  if not IsMillewinInstalled() then
  begin
    MessageBoxW(0,
      'MWCF-Extractor e'' utilizzabile solo se Millewin e'' installato nel sistema.',
      'Millewin non trovato',
      $10);
    Result := False;
  end;
end;

// Funzione chiamata prima dell'installazione
function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';

  // Secondo tentativo di terminazione con feedback utente se fallisce
  if IsAppRunning() then
  begin
    Log('PrepareToInstall: App ancora in esecuzione, tentativo finale');
    if not KillAppWithRetry(3, True) then
    begin
      // L'utente e' stato avvisato, blocca l'installazione
      Result := 'Chiudi MilleWin CF Extractor dalla system tray prima di procedere.';
    end;
  end;
end;

// Funzione chiamata prima della disinstallazione
function InitializeUninstall(): Boolean;
begin
  Result := True;

  // Se l'applicazione e' in esecuzione, terminala
  if IsAppRunning() then
  begin
    if not KillAppWithRetry(3, True) then
    begin
      // L'utente e' stato avvisato, blocca la disinstallazione
      Result := False;
    end;
  end;
end;

// Pulisci la voce legacy dal Registry durante l'installazione
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Rimuovi eventuale voce autostart legacy dal Registry
    RegDeleteValue(HKEY_CURRENT_USER, 'Software\Microsoft\Windows\CurrentVersion\Run', 'MWCFExtractor');
  end;
end;
