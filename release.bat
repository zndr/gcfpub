@echo off
REM ============================================================================
REM MilleWin CF Extractor - Release Script
REM ============================================================================
REM Questo script:
REM   1. Compila il progetto
REM   2. Crea l'installer
REM   3. Prepara i file per la release
REM   4. Crea tag git
REM   5. Push su repository privato
REM   6. Crea GitHub Release su repo privato (trigger workflow)
REM   7. Push su repository pubblico
REM   8. Crea GitHub Release su repo pubblico
REM
REM Requisiti:
REM   - CMake
REM   - Inno Setup 6
REM   - GitHub CLI (gh) - https://cli.github.com/
REM
REM Uso: release.bat <versione>
REM Esempio: release.bat 1.0.0
REM ============================================================================

setlocal enabledelayedexpansion

REM Verifica parametro versione
if "%~1"=="" (
    echo.
    echo Uso: release.bat ^<versione^>
    echo Esempio: release.bat 1.0.0
    echo.
    exit /b 1
)

set "VERSION=%~1"

echo.
echo ============================================
echo  MilleWin CF Extractor - Release v%VERSION%
echo ============================================
echo.

REM Vai alla cartella del progetto
cd /d "%~dp0"

REM ============================================================================
REM FASE 1: Build del progetto
REM ============================================================================

echo [1/8] Compilazione progetto...
if not exist "build" mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Configurazione CMake fallita
    goto :error
)
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
REM FASE 2: Build dell'installer
REM ============================================================================

echo [2/8] Creazione installer...

REM Cerca Inno Setup
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
REM FASE 3: Prepara cartella releases
REM ============================================================================

echo [3/8] Preparazione release...

if not exist "releases" mkdir releases
if not exist "releases\v%VERSION%" mkdir "releases\v%VERSION%"

REM Copia versione portabile con nome appropriato
copy /Y "build\bin\Release\mwcf_extractor.exe" "releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Copia eseguibile fallita
    goto :error
)

REM Copia installer
copy /Y "build\installer\MWCFExtractor_Setup_%VERSION%.exe" "releases\v%VERSION%\" >nul
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Copia installer fallita
    goto :error
)

echo       OK - File copiati in releases\v%VERSION%\
echo       - MWCFExtractor_Portable_%VERSION%.exe
echo       - MWCFExtractor_Setup_%VERSION%.exe

REM ============================================================================
REM FASE 4: Commit e tag
REM ============================================================================

echo [4/8] Commit e tag git...

REM Aggiungi i file della release
git add releases\v%VERSION%\* >nul 2>&1
git add README.md >nul 2>&1
git add -A >nul 2>&1

REM Verifica se ci sono modifiche da committare
git diff --cached --quiet
if %ERRORLEVEL% neq 0 (
    git commit -m "Release v%VERSION%" >nul 2>&1
    if %ERRORLEVEL% neq 0 (
        echo [WARNING] Commit fallito o nessuna modifica
    )
)

REM Crea il tag
git tag -a "v%VERSION%" -m "Release v%VERSION%" 2>nul
if %ERRORLEVEL% neq 0 (
    echo [WARNING] Tag v%VERSION% gia' esistente, lo aggiorno
    git tag -d "v%VERSION%" >nul 2>&1
    git tag -a "v%VERSION%" -m "Release v%VERSION%"
)

echo       OK - Tag v%VERSION% creato

REM ============================================================================
REM FASE 5: Push su repository privato
REM ============================================================================

echo [5/8] Push su repository privato (origin)...

git push origin --all >nul 2>&1
git push origin --tags >nul 2>&1

if %ERRORLEVEL% neq 0 (
    echo [WARNING] Push su origin fallito - verifica la configurazione
) else (
    echo       OK - Pushato su origin
)

REM ============================================================================
REM FASE 6: Crea GitHub Release su repo privato
REM ============================================================================

echo [6/8] Creazione GitHub Release su repo privato...

REM Verifica se gh CLI e' installato
where gh >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERRORE] GitHub CLI (gh) non trovato. Installalo da https://cli.github.com/
    goto :error
)

REM Elimina release esistente se presente
gh release delete "v%VERSION%" --repo zndr/gcf --yes >nul 2>&1

REM Crea la release con gli asset
gh release create "v%VERSION%" ^
    --repo zndr/gcf ^
    --title "v%VERSION%" ^
    --notes "Release v%VERSION%" ^
    "releases\v%VERSION%\MWCFExtractor_Setup_%VERSION%.exe" ^
    "releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe"

if %ERRORLEVEL% neq 0 (
    echo [ERRORE] Creazione GitHub Release fallita
    goto :error
)

echo       OK - GitHub Release creata su repo privato

REM ============================================================================
REM FASE 7: Push su repository pubblico
REM ============================================================================

echo [7/8] Push su repository pubblico (public)...

REM Verifica se il remote 'public' esiste
git remote get-url public >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo       Aggiungo remote 'public'...
    git remote add public https://github.com/zndr/gcfpub.git
)

git push public --all >nul 2>&1
git push public --tags >nul 2>&1

if %ERRORLEVEL% neq 0 (
    echo [WARNING] Push su public fallito - verifica la configurazione
) else (
    echo       OK - Pushato su public
)

REM ============================================================================
REM FASE 8: Crea GitHub Release su repo pubblico (diretto)
REM ============================================================================

echo [8/8] Creazione GitHub Release su repo pubblico...

REM Elimina release esistente se presente
gh release delete "v%VERSION%" --repo zndr/gcfpub --yes >nul 2>&1

REM Crea la release con gli asset
gh release create "v%VERSION%" ^
    --repo zndr/gcfpub ^
    --title "MWCFExtractor v%VERSION%" ^
    --notes "Release v%VERSION%" ^
    "releases\v%VERSION%\MWCFExtractor_Setup_%VERSION%.exe" ^
    "releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe"

if %ERRORLEVEL% neq 0 (
    echo [WARNING] Creazione release su repo pubblico fallita
    echo          Il workflow GitHub Actions dovrebbe comunque crearla automaticamente
) else (
    echo       OK - GitHub Release creata su repo pubblico
)

REM ============================================================================
REM COMPLETATO
REM ============================================================================

echo.
echo ============================================
echo  Release v%VERSION% completata!
echo ============================================
echo.
echo File della release:
echo   - releases\v%VERSION%\MWCFExtractor_Portable_%VERSION%.exe (Portabile)
echo   - releases\v%VERSION%\MWCFExtractor_Setup_%VERSION%.exe (Installer)
echo.
echo GitHub Releases create:
echo   - Privato:  https://github.com/zndr/gcf/releases/tag/v%VERSION%
echo   - Pubblico: https://github.com/zndr/gcfpub/releases/tag/v%VERSION%
echo.
echo Repository aggiornati:
echo   - origin (privato): https://github.com/zndr/gcf.git
echo   - public (pubblico): https://github.com/zndr/gcfpub.git
echo.

goto :end

:error
echo.
echo ============================================
echo  Release fallita!
echo ============================================
echo.
exit /b 1

:end
endlocal
exit /b 0
