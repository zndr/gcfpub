@echo off
REM ============================================================================
REM MilleWin CF Extractor - Build Installer Script
REM ============================================================================
REM Questo script compila il progetto e crea l'installer
REM Requisiti:
REM   - CMake
REM   - Visual Studio Build Tools o Visual Studio
REM   - Inno Setup 6 (ISCC.exe nel PATH o in posizione standard)
REM ============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================
echo  MilleWin CF Extractor - Build Installer
echo ============================================
echo.

REM Vai alla cartella del progetto
cd /d "%~dp0.."

REM Verifica se CMake e' disponibile
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] CMake non trovato nel PATH
    echo Installa CMake da https://cmake.org/download/
    goto :error
)

REM Verifica se Inno Setup e' disponibile
set "ISCC="
where iscc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "ISCC=iscc"
) else (
    REM Cerca nelle posizioni standard
    if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
        set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    ) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
        set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
    )
)

if "%ISCC%"=="" (
    echo [ERRORE] Inno Setup non trovato
    echo Installa Inno Setup 6 da https://jrsoftware.org/isinfo.php
    goto :error
)

echo [1/4] Creazione cartella build...
if not exist "build" mkdir build
cd build

echo [2/4] Configurazione CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Configurazione CMake fallita
    goto :error
)

echo [3/4] Compilazione progetto...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Compilazione fallita
    goto :error
)

REM Verifica che l'exe sia stato creato
if not exist "bin\Release\mwcf_extractor.exe" (
    echo [ERRORE] Eseguibile non trovato in bin\Release\
    goto :error
)

REM Crea la cartella per l'output dell'installer
if not exist "installer" mkdir installer

echo [4/4] Creazione installer...
cd ..
"%ISCC%" /Q "installer\setup.iss"
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Creazione installer fallita
    goto :error
)

echo.
echo ============================================
echo  Build completata con successo!
echo ============================================
echo.
echo Installer creato in: build\installer\
echo.
dir /b "build\installer\*.exe" 2>nul
echo.

goto :end

:error
echo.
echo ============================================
echo  Build fallita!
echo ============================================
echo.
exit /b 1

:end
endlocal
exit /b 0
