@echo off
REM ============================================================================
REM release.bat - Wrapper per Prepare-Release.ps1
REM ============================================================================
REM Uso:
REM   release.bat 1.3.0           - Prepara release senza pubblicare
REM   release.bat 1.3.0 -Publish  - Prepara e pubblica release
REM ============================================================================

setlocal

if "%~1"=="" (
    echo.
    echo Uso: release.bat ^<versione^> [-Publish]
    echo.
    echo Esempi:
    echo   release.bat 1.3.0           - Prepara release senza pubblicare
    echo   release.bat 1.3.0 -Publish  - Prepara e pubblica release
    echo.
    exit /b 1
)

set "VERSION=%~1"
set "EXTRA_ARGS="

if /i "%~2"=="-Publish" set "EXTRA_ARGS=-Publish"
if /i "%~2"=="/Publish" set "EXTRA_ARGS=-Publish"

powershell -ExecutionPolicy Bypass -File "%~dp0Prepare-Release.ps1" -NewVersion "%VERSION%" %EXTRA_ARGS%

endlocal
