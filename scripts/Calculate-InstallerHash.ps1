# ============================================================================
# Calculate-InstallerHash.ps1 - Calcola hash SHA256 degli eseguibili
# ============================================================================
# Questo script calcola gli hash SHA256 per gli eseguibili della release.
#
# Uso:
#   .\scripts\Calculate-InstallerHash.ps1
#   .\scripts\Calculate-InstallerHash.ps1 -Version "1.3.0"
#   .\scripts\Calculate-InstallerHash.ps1 -UpdateVersionJson
# ============================================================================

[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$Version = "",

    [Parameter(Mandatory = $false)]
    [switch]$UpdateVersionJson
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $ProjectRoot

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "  MWCFExtractor - Calcolo Hash SHA256" -ForegroundColor Cyan
Write-Host "============================================`n" -ForegroundColor Cyan

# Se la versione non e' specificata, leggi da Version.props
if ([string]::IsNullOrWhiteSpace($Version)) {
    $versionPropsPath = Join-Path $ProjectRoot "Version.props"
    if (Test-Path $versionPropsPath) {
        [xml]$versionProps = Get-Content $versionPropsPath
        $Version = $versionProps.Project.PropertyGroup.VersionPrefix
    } else {
        Write-Host "Errore: Version.props non trovato e versione non specificata" -ForegroundColor Red
        exit 1
    }
}

Write-Host "Versione: $Version`n" -ForegroundColor Yellow

# Percorsi file
$releasesDir = Join-Path $ProjectRoot "releases\v$Version"
$installerPath = Join-Path $releasesDir "MWCFExtractor_Setup_$Version.exe"
$portablePath = Join-Path $releasesDir "MWCFExtractor_Portable_$Version.exe"

# Se i file non sono nella cartella releases, cerca in build
if (-not (Test-Path $installerPath)) {
    $installerPath = Join-Path $ProjectRoot "build\installer\MWCFExtractor_Setup_$Version.exe"
}
if (-not (Test-Path $portablePath)) {
    $portablePath = Join-Path $ProjectRoot "build\bin\Release\mwcf_extractor.exe"
}

$results = @{}

# Calcola hash installer
if (Test-Path $installerPath) {
    $installerHash = (Get-FileHash -Path $installerPath -Algorithm SHA256).Hash
    $installerSize = (Get-Item $installerPath).Length
    $installerSizeMB = [math]::Round($installerSize / 1MB, 2)

    Write-Host "Installer:" -ForegroundColor Green
    Write-Host "  File:   $(Split-Path $installerPath -Leaf)"
    Write-Host "  Size:   $installerSizeMB MB ($installerSize bytes)"
    Write-Host "  SHA256: $installerHash"
    Write-Host ""

    $results["InstallerHash"] = $installerHash
    $results["InstallerSize"] = $installerSize
} else {
    Write-Host "Installer non trovato: $installerPath" -ForegroundColor Yellow
}

# Calcola hash portable
if (Test-Path $portablePath) {
    $portableHash = (Get-FileHash -Path $portablePath -Algorithm SHA256).Hash
    $portableSize = (Get-Item $portablePath).Length
    $portableSizeKB = [math]::Round($portableSize / 1KB, 2)

    Write-Host "Portable:" -ForegroundColor Green
    Write-Host "  File:   $(Split-Path $portablePath -Leaf)"
    Write-Host "  Size:   $portableSizeKB KB ($portableSize bytes)"
    Write-Host "  SHA256: $portableHash"
    Write-Host ""

    $results["PortableHash"] = $portableHash
    $results["PortableSize"] = $portableSize
} else {
    Write-Host "Portable non trovato: $portablePath" -ForegroundColor Yellow
}

# Aggiorna version.json se richiesto
if ($UpdateVersionJson -and $results.Count -gt 0) {
    $versionJsonPath = Join-Path $ProjectRoot "version.json"

    if (Test-Path $versionJsonPath) {
        $versionJson = Get-Content -Path $versionJsonPath -Raw | ConvertFrom-Json

        if ($results.ContainsKey("InstallerHash")) {
            $versionJson.InstallerSha256 = $results["InstallerHash"]
            $versionJson.InstallerSize = $results["InstallerSize"]
        }
        if ($results.ContainsKey("PortableHash")) {
            $versionJson.PortableSha256 = $results["PortableHash"]
            $versionJson.PortableSize = $results["PortableSize"]
        }

        $versionJson | ConvertTo-Json -Depth 10 | Set-Content -Path $versionJsonPath -Encoding UTF8

        Write-Host "version.json aggiornato" -ForegroundColor Green
    } else {
        Write-Host "version.json non trovato" -ForegroundColor Yellow
    }
}

# Genera file checksums
if (Test-Path $releasesDir) {
    $checksumPath = Join-Path $releasesDir "checksums.sha256"
    $checksumContent = @"
# SHA256 Checksums for MWCFExtractor v$Version
# Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")

"@

    if ($results.ContainsKey("InstallerHash")) {
        $checksumContent += "$($results['InstallerHash'])  MWCFExtractor_Setup_$Version.exe`n"
    }
    if ($results.ContainsKey("PortableHash")) {
        $checksumContent += "$($results['PortableHash'])  MWCFExtractor_Portable_$Version.exe`n"
    }

    Set-Content -Path $checksumPath -Value $checksumContent -Encoding UTF8
    Write-Host "Checksum salvato in: $checksumPath" -ForegroundColor Green
}

Write-Host "`n============================================" -ForegroundColor Cyan
Write-Host "  Calcolo hash completato" -ForegroundColor Cyan
Write-Host "============================================`n" -ForegroundColor Cyan
