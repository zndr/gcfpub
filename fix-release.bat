@echo off
REM ============================================================================
REM Fix Release v1.3.0 - Ricompila e ricarica la release su GitHub
REM ============================================================================

setlocal enabledelayedexpansion

set "VERSION=1.3.0"

echo.
echo ============================================
echo  Fix Release v%VERSION%
echo ============================================
echo.

cd /d "%~dp0"

REM ============================================================================
REM FASE 1: Pulisci build precedente
REM ============================================================================

echo [1/7] Pulizia build precedente...
if exist "build" rmdir /s /q "build"
echo       OK - Cartella build rimossa

REM ============================================================================
REM FASE 2: Configura CMake
REM ============================================================================

echo [2/7] Configurazione CMake...
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Configurazione CMake fallita
    goto :error
)
cd ..
echo       OK - CMake configurato

REM ============================================================================
REM FASE 3: Compila il progetto
REM ============================================================================

echo [3/7] Compilazione progetto...
cd build
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Compilazione fallita
    goto :error
)
cd ..

if not exist "build\bin\Release\mwcf_extractor.exe" (
    echo [ERRORE] Eseguibile non trovato
    goto :error
)
echo       OK - Eseguibile compilato

REM ============================================================================
REM FASE 4: Crea installer
REM ============================================================================

echo [4/7] Creazione installer...

set "ISCC="
where iscc >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set "ISCC=iscc"
) else (
    if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
        set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    ) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
        set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
    )
)

if "%ISCC%"=="" (
    echo [ERRORE] Inno Setup non trovato
    goto :error
)

"%ISCC%" /Q "installer\setup.iss"
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Creazione installer fallita
    goto :error
)
echo       OK - Installer creato

REM ============================================================================
REM FASE 5: Copia file nella cartella releases
REM ============================================================================

echo [5/7] Copia file in releases\v%VERSION%\...

if not exist "releases\v%VERSION%" mkdir "releases\v%VERSION%"

copy /Y "build\bin\Release\mwcf_extractor.exe" "releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Copia portable fallita
    goto :error
)

copy /Y "build\installer\MWCFExtractor_Setup_%VERSION%.exe" "releases\v%VERSION%\" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Copia installer fallita
    goto :error
)

echo       OK - File copiati
echo       - MWCFExtractor_Portable_%VERSION%.exe
echo       - MWCFExtractor_Setup_%VERSION%.exe

REM ============================================================================
REM FASE 6: Elimina vecchia release da GitHub
REM ============================================================================

echo [6/7] Rimozione vecchia release da GitHub...

gh release delete "v%VERSION%" --repo zndr/gcfpub --yes >nul 2>&1
echo       OK - Vecchia release rimossa (se esisteva)

REM ============================================================================
REM FASE 7: Carica nuova release su GitHub
REM ============================================================================

echo [7/7] Caricamento nuova release su GitHub...

gh release create "v%VERSION%" ^
    --repo zndr/gcfpub ^
    --title "MWCFExtractor v%VERSION%" ^
    --notes "### Novita
- **Hotkey completamente personalizzabile**: ora e possibile usare qualsiasi combinazione di tasti
  - Supporto per modificatori multipli (CTRL+ALT, CTRL+SHIFT+ALT, WIN+SHIFT, ecc.)
  - Supporto per tutti i tasti: lettere A-Z, numeri 0-9, tasti funzione F1-F24, frecce, tasti speciali
  - Nuova interfaccia di cattura diretta: basta premere la combinazione desiderata
  - Compatibilita con configurazioni precedenti (formato legacy)

### Miglioramenti
- Dialog di configurazione hotkey piu intuitivo e compatto" ^
    "releases\v%VERSION%\MWCFExtractor_Setup_%VERSION%.exe" ^
    "releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe"

if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Caricamento release fallito
    goto :error
)

echo       OK - Release caricata

REM ============================================================================
REM COMPLETATO
REM ============================================================================

echo.
echo ============================================
echo  Fix Release v%VERSION% completato!
echo ============================================
echo.
echo Release pubblica: https://github.com/zndr/gcfpub/releases/tag/v%VERSION%
echo.
echo Ora puoi testare scaricando l'installer dal link sopra.
echo.

goto :end

:error
echo.
echo ============================================
echo  Fix Release fallito!
echo ============================================
echo.
cd /d "%~dp0"
exit /b 1

:end
endlocal
exit /b 0
