# ============================================================================
# Prepare-Release.ps1 - Script completo per la release di MWCFExtractor
# ============================================================================
# Questo script automatizza il processo di release:
#   1. Aggiorna tutti i file di versione
#   2. Compila il progetto (CMake + MSVC)
#   3. Crea l'installer (Inno Setup)
#   4. Calcola hash SHA256
#   5. Aggiorna version.json
#   6. Commit e tag git
#   7. Pubblica su GitHub (repo privato e pubblico)
#
# Uso:
#   .\scripts\Prepare-Release.ps1 -NewVersion "1.3.0"
#   .\scripts\Prepare-Release.ps1 -NewVersion "1.3.0" -Publish
#   .\scripts\Prepare-Release.ps1 -NewVersion "1.3.0" -Publish -SkipBuild
# ============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$NewVersion,

    [Parameter(Mandatory = $false)]
    [string]$ReleaseNotes = "",

    [Parameter(Mandatory = $false)]
    [switch]$Publish,

    [Parameter(Mandatory = $false)]
    [switch]$SkipBuild,

    [Parameter(Mandatory = $false)]
    [switch]$SkipCommit
)

# Configurazione
$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $ProjectRoot

# Colori output
function Write-Step { param($step, $msg) Write-Host "`n[$step] " -ForegroundColor Cyan -NoNewline; Write-Host $msg }
function Write-OK { Write-Host "    OK - " -ForegroundColor Green -NoNewline; Write-Host $args[0] }
function Write-Warn { Write-Host "    WARNING - " -ForegroundColor Yellow -NoNewline; Write-Host $args[0] }
function Write-Err { Write-Host "    ERRORE - " -ForegroundColor Red -NoNewline; Write-Host $args[0] }

# Banner
Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "  MWCFExtractor - Release v$NewVersion" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

# ============================================================================
# STEP 1: Acquisizione Release Notes
# ============================================================================
Write-Step "1/13" "Acquisizione release notes..."

if ([string]::IsNullOrWhiteSpace($ReleaseNotes)) {
    Write-Host "    Inserisci le release notes (termina con CTRL+Z + INVIO):" -ForegroundColor Yellow
    $lines = @()
    while ($true) {
        $line = Read-Host
        if ($null -eq $line) { break }
        $lines += $line
    }
    $ReleaseNotes = $lines -join "`n"
}

if ([string]::IsNullOrWhiteSpace($ReleaseNotes)) {
    $ReleaseNotes = "Release v$NewVersion"
}

Write-OK "Release notes acquisite ($($ReleaseNotes.Length) caratteri)"

# ============================================================================
# STEP 2: Aggiornamento Version.props
# ============================================================================
Write-Step "2/13" "Aggiornamento Version.props..."

$versionPropsPath = Join-Path $ProjectRoot "Version.props"
$today = Get-Date -Format "yyyy-MM-dd"

$versionPropsContent = @"
<?xml version="1.0" encoding="utf-8"?>
<!--
  ============================================================================
  Version.props - File centralizzato per il versioning
  ============================================================================
  Questo file e' la FONTE UNICA DI VERITA' per la versione dell'applicazione.

  NON MODIFICARE MANUALMENTE - Usa lo script Prepare-Release.ps1

  Uso:
    .\scripts\Prepare-Release.ps1 -NewVersion "X.Y.Z"
  ============================================================================
-->
<Project>
  <PropertyGroup>
    <!-- Versione principale (Major.Minor.Patch) -->
    <VersionPrefix>$NewVersion</VersionPrefix>

    <!-- Data ultimo rilascio -->
    <ReleaseDate>$today</ReleaseDate>
  </PropertyGroup>
</Project>
"@

Set-Content -Path $versionPropsPath -Value $versionPropsContent -Encoding UTF8
Write-OK "Version.props aggiornato a $NewVersion"

# ============================================================================
# STEP 3: Aggiornamento setup.iss (Inno Setup)
# ============================================================================
Write-Step "3/13" "Aggiornamento setup.iss..."

$setupIssPath = Join-Path $ProjectRoot "installer\setup.iss"
$setupContent = Get-Content $setupIssPath -Raw

# Aggiorna #define MyAppVersion
$setupContent = $setupContent -replace '#define MyAppVersion "[^"]*"', "#define MyAppVersion `"$NewVersion`""

Set-Content -Path $setupIssPath -Value $setupContent -Encoding UTF8 -NoNewline
Write-OK "setup.iss aggiornato a $NewVersion"

# ============================================================================
# STEP 4: Aggiornamento resource.h
# ============================================================================
Write-Step "4/13" "Aggiornamento resource.h..."

$resourceHPath = Join-Path $ProjectRoot "src\resource.h"
$resourceContent = Get-Content $resourceHPath -Raw

# Aggiorna APP_VERSION
$resourceContent = $resourceContent -replace '#define APP_VERSION\s+L"[^"]*"', "#define APP_VERSION         L`"$NewVersion`""

Set-Content -Path $resourceHPath -Value $resourceContent -Encoding UTF8 -NoNewline
Write-OK "resource.h aggiornato a $NewVersion"

# ============================================================================
# STEP 5: Aggiornamento CMakeLists.txt
# ============================================================================
Write-Step "5/13" "Aggiornamento CMakeLists.txt..."

$cmakePath = Join-Path $ProjectRoot "CMakeLists.txt"
$cmakeContent = Get-Content $cmakePath -Raw

# Aggiorna project VERSION
$cmakeContent = $cmakeContent -replace 'project\(mwcf_extractor VERSION [^\s]+ LANGUAGES', "project(mwcf_extractor VERSION $NewVersion LANGUAGES"

Set-Content -Path $cmakePath -Value $cmakeContent -Encoding UTF8 -NoNewline
Write-OK "CMakeLists.txt aggiornato a $NewVersion"

# ============================================================================
# STEP 6: Build del progetto
# ============================================================================
if (-not $SkipBuild) {
    Write-Step "6/13" "Compilazione progetto (CMake + MSVC)..."

    $buildDir = Join-Path $ProjectRoot "build"
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    # CMake configure
    Push-Location $buildDir
    $cmakeResult = & cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Err "Configurazione CMake fallita"
        Write-Host $cmakeResult
        exit 1
    }

    # CMake build
    $buildResult = & cmake --build . --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Pop-Location
        Write-Err "Compilazione fallita"
        Write-Host $buildResult
        exit 1
    }
    Pop-Location

    $exePath = Join-Path $ProjectRoot "build\bin\Release\mwcf_extractor.exe"
    if (-not (Test-Path $exePath)) {
        Write-Err "Eseguibile non trovato: $exePath"
        exit 1
    }

    Write-OK "Eseguibile compilato"
} else {
    Write-Step "6/13" "Build saltato (SkipBuild)"
}

# ============================================================================
# STEP 7: Creazione Installer (Inno Setup)
# ============================================================================
if (-not $SkipBuild) {
    Write-Step "7/13" "Creazione installer (Inno Setup)..."

    # Cerca Inno Setup
    $isccPaths = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe"
    )
    $iscc = $null
    foreach ($path in $isccPaths) {
        if (Test-Path $path) {
            $iscc = $path
            break
        }
    }
    if (-not $iscc) {
        $iscc = Get-Command iscc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    }

    if (-not $iscc) {
        Write-Err "Inno Setup non trovato"
        exit 1
    }

    $setupScript = Join-Path $ProjectRoot "installer\setup.iss"
    $isccResult = & $iscc /Q $setupScript 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Creazione installer fallita"
        Write-Host $isccResult
        exit 1
    }

    Write-OK "Installer creato"
} else {
    Write-Step "7/13" "Creazione installer saltata (SkipBuild)"
}

# ============================================================================
# STEP 8: Preparazione cartella releases
# ============================================================================
Write-Step "8/13" "Preparazione cartella releases..."

$releasesDir = Join-Path $ProjectRoot "releases\v$NewVersion"
if (-not (Test-Path $releasesDir)) {
    New-Item -ItemType Directory -Path $releasesDir -Force | Out-Null
}

# Copia versione portabile con nome appropriato
$sourceExe = Join-Path $ProjectRoot "build\bin\Release\mwcf_extractor.exe"
$portableExe = Join-Path $releasesDir "MWCFExtractor_Portable_$NewVersion.exe"
Copy-Item -Path $sourceExe -Destination $portableExe -Force

# Copia installer
$sourceInstaller = Join-Path $ProjectRoot "build\installer\MWCFExtractor_Setup_$NewVersion.exe"
$destInstaller = Join-Path $releasesDir "MWCFExtractor_Setup_$NewVersion.exe"
Copy-Item -Path $sourceInstaller -Destination $destInstaller -Force

Write-OK "File copiati in releases\v$NewVersion\"

# ============================================================================
# STEP 9: Calcolo hash SHA256
# ============================================================================
Write-Step "9/13" "Calcolo hash SHA256..."

# Funzione per calcolare hash usando certutil (fallback per ambienti senza Get-FileHash)
function Get-SHA256Hash {
    param([string]$FilePath)

    # Prova prima Get-FileHash
    try {
        $hash = (Get-FileHash -Path $FilePath -Algorithm SHA256 -ErrorAction Stop).Hash
        return $hash
    } catch {
        # Fallback a certutil
        $output = & certutil -hashfile $FilePath SHA256 2>&1
        if ($LASTEXITCODE -eq 0) {
            # Estrai l'hash dalla seconda riga dell'output
            $lines = $output -split "`n"
            foreach ($line in $lines) {
                $line = $line.Trim()
                if ($line -match '^[a-fA-F0-9]{64}$') {
                    return $line.ToUpper()
                }
            }
        }
        return ""
    }
}

$installerHash = Get-SHA256Hash -FilePath $destInstaller
$portableHash = Get-SHA256Hash -FilePath $portableExe
$installerSize = (Get-Item $destInstaller).Length
$portableSize = (Get-Item $portableExe).Length

Write-OK "Installer: $installerHash"
Write-OK "Portable:  $portableHash"

# Salva file .sha256
$hashContent = @"
# SHA256 Checksums for MWCFExtractor v$NewVersion
# Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")

$installerHash  MWCFExtractor_Setup_$NewVersion.exe
$portableHash  MWCFExtractor_Portable_$NewVersion.exe
"@
Set-Content -Path (Join-Path $releasesDir "checksums.sha256") -Value $hashContent -Encoding UTF8

# ============================================================================
# STEP 10: Aggiornamento version.json
# ============================================================================
Write-Step "10/13" "Aggiornamento version.json..."

$versionJsonPath = Join-Path $ProjectRoot "version.json"
$versionJson = @{
    Version = $NewVersion
    DownloadUrl = "https://github.com/zndr/gcfpub/releases/download/v$NewVersion/MWCFExtractor_Setup_$NewVersion.exe"
    PortableUrl = "https://github.com/zndr/gcfpub/releases/download/v$NewVersion/MWCFExtractor_Portable_$NewVersion.exe"
    ReleaseDate = $today
    InstallerSize = $installerSize
    PortableSize = $portableSize
    InstallerSha256 = $installerHash
    PortableSha256 = $portableHash
    ReleaseNotes = $ReleaseNotes
    MinimumWindowsVersion = "10.0"
}

$versionJson | ConvertTo-Json -Depth 10 | Set-Content -Path $versionJsonPath -Encoding UTF8
Write-OK "version.json aggiornato"

# ============================================================================
# STEP 11: Verifica integrita file
# ============================================================================
Write-Step "11/13" "Verifica integrita file..."

$filesToCheck = @(
    $versionPropsPath,
    $setupIssPath,
    $resourceHPath,
    $cmakePath,
    $versionJsonPath,
    $destInstaller,
    $portableExe
)

$allOk = $true
foreach ($file in $filesToCheck) {
    if (Test-Path $file) {
        Write-Host "    OK " -ForegroundColor Green -NoNewline
        Write-Host (Split-Path $file -Leaf)
    } else {
        Write-Host "    MANCANTE " -ForegroundColor Red -NoNewline
        Write-Host (Split-Path $file -Leaf)
        $allOk = $false
    }
}

if (-not $allOk) {
    Write-Err "Alcuni file mancanti"
    exit 1
}

# ============================================================================
# STEP 12: Riepilogo
# ============================================================================
Write-Step "12/13" "Riepilogo release..."

Write-Host ""
Write-Host "    Versione:  " -NoNewline; Write-Host $NewVersion -ForegroundColor Green
Write-Host "    Data:      " -NoNewline; Write-Host $today -ForegroundColor Green
Write-Host ""
Write-Host "    File generati:"
Write-Host "      - releases\v$NewVersion\MWCFExtractor_Setup_$NewVersion.exe" -ForegroundColor Yellow
Write-Host "      - releases\v$NewVersion\MWCFExtractor_Portable_$NewVersion.exe" -ForegroundColor Yellow
Write-Host "      - releases\v$NewVersion\checksums.sha256" -ForegroundColor Yellow
Write-Host ""

# ============================================================================
# STEP 13: Pubblicazione (opzionale)
# ============================================================================
if ($Publish) {
    Write-Step "13/13" "Pubblicazione su GitHub..."

    # 13a: Commit e push su repo privato
    if (-not $SkipCommit) {
        Write-Host "    [13a] Commit e push su repo privato (origin)..." -ForegroundColor Cyan

        git add -A 2>$null
        git commit -m "chore: Preparazione release v$NewVersion" 2>$null

        # Crea tag
        git tag -d "v$NewVersion" 2>$null
        git tag -a "v$NewVersion" -m "Release v$NewVersion"

        git push origin --all 2>$null
        git push origin --tags 2>$null

        Write-OK "Pushato su origin"
    }

    # 13b: Crea release su repo privato
    Write-Host "    [13b] Creazione release su repo privato..." -ForegroundColor Cyan

    $releaseNotesFile = Join-Path $env:TEMP "release_notes_temp.md"
    Set-Content -Path $releaseNotesFile -Value $ReleaseNotes -Encoding UTF8

    # Elimina release esistente se presente
    gh release delete "v$NewVersion" --repo zndr/gcf --yes 2>$null

    gh release create "v$NewVersion" `
        --repo zndr/gcf `
        --title "v$NewVersion" `
        --notes-file $releaseNotesFile `
        $destInstaller `
        $portableExe

    if ($LASTEXITCODE -eq 0) {
        Write-OK "Release creata su repo privato"
    } else {
        Write-Warn "Release su repo privato potrebbe essere fallita"
    }

    # 13c: Crea release su repo pubblico
    Write-Host "    [13c] Creazione release su repo pubblico..." -ForegroundColor Cyan

    # Elimina release esistente se presente
    gh release delete "v$NewVersion" --repo zndr/gcfpub --yes 2>$null

    gh release create "v$NewVersion" `
        --repo zndr/gcfpub `
        --title "MWCFExtractor v$NewVersion" `
        --notes-file $releaseNotesFile `
        $destInstaller `
        $portableExe

    if ($LASTEXITCODE -eq 0) {
        Write-OK "Release creata su repo pubblico"
    } else {
        Write-Warn "Release su repo pubblico potrebbe essere fallita"
    }

    # 13d: Aggiorna version.json nel repo pubblico
    Write-Host "    [13d] Aggiornamento version.json nel repo pubblico..." -ForegroundColor Cyan

    try {
        # Leggi il contenuto corrente per ottenere lo SHA
        $existingFile = gh api repos/zndr/gcfpub/contents/version.json 2>$null | ConvertFrom-Json
        $currentSha = $existingFile.sha

        # Prepara il contenuto in base64
        $versionJsonContent = Get-Content -Path $versionJsonPath -Raw
        $base64Content = [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($versionJsonContent))

        # Aggiorna il file
        $updateBody = @{
            message = "chore: Aggiorna version.json per v$NewVersion"
            content = $base64Content
            sha = $currentSha
        } | ConvertTo-Json

        $updateResult = $updateBody | gh api repos/zndr/gcfpub/contents/version.json -X PUT --input - 2>&1

        Write-OK "version.json aggiornato nel repo pubblico"
    } catch {
        # Se il file non esiste, crealo
        try {
            $versionJsonContent = Get-Content -Path $versionJsonPath -Raw
            $base64Content = [Convert]::ToBase64String([System.Text.Encoding]::UTF8.GetBytes($versionJsonContent))

            $createBody = @{
                message = "chore: Aggiungi version.json per v$NewVersion"
                content = $base64Content
            } | ConvertTo-Json

            $createResult = $createBody | gh api repos/zndr/gcfpub/contents/version.json -X PUT --input - 2>&1

            Write-OK "version.json creato nel repo pubblico"
        } catch {
            Write-Warn "Impossibile aggiornare version.json nel repo pubblico: $_"
        }
    }

    # Cleanup
    Remove-Item $releaseNotesFile -Force -ErrorAction SilentlyContinue

    Write-Host ""
    Write-Host "    Release pubblicate:" -ForegroundColor Green
    Write-Host "      - Privato: https://github.com/zndr/gcf/releases/tag/v$NewVersion" -ForegroundColor Yellow
    Write-Host "      - Pubblico: https://github.com/zndr/gcfpub/releases/tag/v$NewVersion" -ForegroundColor Yellow
} else {
    Write-Step "13/13" "Pubblicazione saltata (usa -Publish per pubblicare)"
}

# ============================================================================
# COMPLETATO
# ============================================================================
Write-Host "`n============================================" -ForegroundColor Green
Write-Host "  Release v$NewVersion completata!" -ForegroundColor Green
Write-Host "============================================`n" -ForegroundColor Green

if (-not $Publish) {
    Write-Host "Per pubblicare la release, esegui:" -ForegroundColor Yellow
    Write-Host "  .\scripts\Prepare-Release.ps1 -NewVersion `"$NewVersion`" -Publish -SkipBuild`n" -ForegroundColor Yellow
}
