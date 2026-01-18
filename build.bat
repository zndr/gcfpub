@echo off
setlocal enabledelayedexpansion

echo ============================================
echo  MilleWin CF Extractor - Build Script
echo ============================================
echo.

:: Check for Visual Studio
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo ERRORE: Visual Studio non trovato.
    echo Esegui questo script dal "Developer Command Prompt for VS 2022"
    echo oppure installa Visual Studio con il supporto C++.
    echo.
    pause
    exit /b 1
)

:: Create tools directory if not exists
if not exist tools mkdir tools

:: Generate icon if not exists or if source is newer
echo [1/4] Generazione icona...
if not exist "res\app.ico" (
    echo   Compilazione generate_icon.exe...
    pushd tools
    cl /nologo /EHsc /O2 generate_icon.cpp /Fe:generate_icon.exe user32.lib gdi32.lib >nul 2>&1
    if %errorlevel% neq 0 (
        echo   ATTENZIONE: Impossibile compilare il generatore di icone.
        echo   L'applicazione usera' un'icona generata dinamicamente.
        popd
    ) else (
        echo   Esecuzione generate_icon.exe...
        generate_icon.exe
        popd
    )
) else (
    echo   Icona esistente: res\app.ico
)

:: Create build directory
if not exist build mkdir build
cd build

:: Configure with CMake
echo.
echo [2/4] Configurazione CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %errorlevel% neq 0 (
    echo ERRORE: Configurazione CMake fallita.
    cd ..
    pause
    exit /b 1
)

:: Build Release
echo.
echo [3/4] Compilazione Release...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo ERRORE: Compilazione fallita.
    cd ..
    pause
    exit /b 1
)

:: Return to project root
cd ..

:: Check if executable was created
if exist mwcf_extractor.exe (
    echo.
    echo [4/4] Build completata con successo!
    echo.
    echo Eseguibile creato: mwcf_extractor.exe
    echo Dimensione:
    for %%A in (mwcf_extractor.exe) do echo   %%~zA bytes
) else (
    echo.
    echo ATTENZIONE: L'eseguibile non e' stato copiato nella root.
    echo Controlla la cartella build\bin\Release\
)

echo.
echo ============================================
pause
