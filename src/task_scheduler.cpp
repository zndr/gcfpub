#include "task_scheduler.h"
#include <comdef.h>
#include <taskschd.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

namespace taskscheduler {

static const wchar_t* AUTOSTART_KEY_NAME = L"MWCFExtractor";
static const wchar_t* TASK_FOLDER = L"\\";
static const wchar_t* TASK_AUTHOR = L"MWCFExtractor";
static const wchar_t* TASK_DESCRIPTION = L"Avvia automaticamente MilleWin CF Extractor al login";

// Variabili per tracciare gli errori
static HRESULT g_lastError = S_OK;
static std::wstring g_lastErrorMessage;

static void setError(HRESULT hr, const wchar_t* context) {
    g_lastError = hr;
    g_lastErrorMessage = context;
    g_lastErrorMessage += L" (HRESULT: 0x";

    wchar_t hexBuf[16];
    swprintf_s(hexBuf, L"%08X", (unsigned int)hr);
    g_lastErrorMessage += hexBuf;
    g_lastErrorMessage += L")";
}

static std::wstring getExePath() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

// Ottiene il percorso della cartella Startup dell'utente
static std::wstring getStartupFolder() {
    wchar_t path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

// Ottiene il percorso del collegamento nella cartella Startup
static std::wstring getStartupShortcutPath() {
    std::wstring startupFolder = getStartupFolder();
    if (startupFolder.empty()) {
        return L"";
    }
    return startupFolder + L"\\MWCFExtractor.lnk";
}

bool isTaskEnabled() {
    // Verifica se esiste il collegamento nella cartella Startup
    std::wstring shortcutPath = getStartupShortcutPath();
    if (shortcutPath.empty()) {
        return false;
    }

    // Verifica se il file esiste
    DWORD attrs = GetFileAttributesW(shortcutPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    // Il collegamento esiste - verifica che punti all'exe corrente
    HRESULT hr = CoInitialize(NULL);
    bool needsUninit = SUCCEEDED(hr);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return true; // Assume abilitato se non possiamo verificare
    }

    bool result = false;
    IShellLinkW* pShellLink = nullptr;
    IPersistFile* pPersistFile = nullptr;

    do {
        hr = CoCreateInstance(
            CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IShellLinkW,
            (void**)&pShellLink
        );
        if (FAILED(hr)) { result = true; break; } // Assume abilitato

        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (FAILED(hr)) { result = true; break; }

        hr = pPersistFile->Load(shortcutPath.c_str(), STGM_READ);
        if (FAILED(hr)) { result = true; break; }

        wchar_t targetPath[MAX_PATH] = {0};
        hr = pShellLink->GetPath(targetPath, MAX_PATH, NULL, 0);
        if (SUCCEEDED(hr)) {
            std::wstring exePath = getExePath();
            result = (_wcsicmp(targetPath, exePath.c_str()) == 0);
        } else {
            result = true; // Assume abilitato
        }

    } while (false);

    if (pPersistFile) pPersistFile->Release();
    if (pShellLink) pShellLink->Release();

    if (needsUninit) {
        CoUninitialize();
    }

    return result;
}

// Crea un collegamento (.lnk)
static bool createShortcut(const std::wstring& shortcutPath, const std::wstring& targetPath) {
    HRESULT hr = CoInitialize(NULL);
    bool needsUninit = SUCCEEDED(hr);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    bool result = false;
    IShellLinkW* pShellLink = nullptr;
    IPersistFile* pPersistFile = nullptr;

    do {
        // Crea l'oggetto ShellLink
        hr = CoCreateInstance(
            CLSID_ShellLink,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IShellLinkW,
            (void**)&pShellLink
        );
        if (FAILED(hr)) break;

        // Imposta il percorso del target
        hr = pShellLink->SetPath(targetPath.c_str());
        if (FAILED(hr)) break;

        // Imposta la directory di lavoro
        wchar_t workDir[MAX_PATH] = {0};
        wcscpy_s(workDir, targetPath.c_str());
        PathRemoveFileSpecW(workDir);
        pShellLink->SetWorkingDirectory(workDir);

        // Imposta la descrizione
        pShellLink->SetDescription(L"MilleWin CF Extractor - Avvio automatico");

        // Ottieni l'interfaccia IPersistFile per salvare
        hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
        if (FAILED(hr)) break;

        // Salva il collegamento
        hr = pPersistFile->Save(shortcutPath.c_str(), TRUE);
        result = SUCCEEDED(hr);

    } while (false);

    if (pPersistFile) pPersistFile->Release();
    if (pShellLink) pShellLink->Release();

    if (needsUninit) {
        CoUninitialize();
    }

    return result;
}

bool setTaskEnabled(bool enable) {
    g_lastError = S_OK;
    g_lastErrorMessage.clear();

    std::wstring shortcutPath = getStartupShortcutPath();
    if (shortcutPath.empty()) {
        setError(E_FAIL, L"Impossibile ottenere cartella Startup");
        return false;
    }

    if (enable) {
        // Crea il collegamento nella cartella Startup
        std::wstring exePath = getExePath();
        if (!createShortcut(shortcutPath, exePath)) {
            setError(E_FAIL, L"Impossibile creare collegamento in Startup");
            return false;
        }
    } else {
        // Elimina il collegamento
        if (!DeleteFileW(shortcutPath.c_str())) {
            DWORD err = GetLastError();
            // Se il file non esiste, consideriamo successo
            if (err != ERROR_FILE_NOT_FOUND) {
                setError(HRESULT_FROM_WIN32(err), L"Impossibile eliminare collegamento");
                return false;
            }
        }
    }

    return true;
}

bool hasLegacyRegistryAutostart() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[MAX_PATH] = {0};
    DWORD size = sizeof(value);
    DWORD type = 0;

    result = RegQueryValueExW(hKey, AUTOSTART_KEY_NAME, NULL, &type,
                              (LPBYTE)value, &size);

    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS && type == REG_SZ);
}

bool removeLegacyRegistryAutostart() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegDeleteValueW(hKey, AUTOSTART_KEY_NAME);

    RegCloseKey(hKey);

    // Se il valore non esiste, consideriamo comunque successo
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

HRESULT getLastError() {
    return g_lastError;
}

std::wstring getLastErrorMessage() {
    return g_lastErrorMessage;
}

// Ottiene il percorso della cartella Desktop dell'utente
static std::wstring getDesktopFolder() {
    wchar_t path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path))) {
        return std::wstring(path);
    }
    return L"";
}

// Ottiene il percorso del collegamento sul Desktop
static std::wstring getDesktopShortcutPath() {
    std::wstring desktopFolder = getDesktopFolder();
    if (desktopFolder.empty()) {
        return L"";
    }
    return desktopFolder + L"\\MilleWin CF Extractor.lnk";
}

bool hasDesktopShortcut() {
    std::wstring shortcutPath = getDesktopShortcutPath();
    if (shortcutPath.empty()) {
        return false;
    }
    DWORD attrs = GetFileAttributesW(shortcutPath.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES);
}

bool setDesktopShortcut(bool create) {
    g_lastError = S_OK;
    g_lastErrorMessage.clear();

    std::wstring shortcutPath = getDesktopShortcutPath();
    if (shortcutPath.empty()) {
        setError(E_FAIL, L"Impossibile ottenere cartella Desktop");
        return false;
    }

    if (create) {
        std::wstring exePath = getExePath();
        if (!createShortcut(shortcutPath, exePath)) {
            setError(E_FAIL, L"Impossibile creare collegamento sul Desktop");
            return false;
        }
    } else {
        if (!DeleteFileW(shortcutPath.c_str())) {
            DWORD err = GetLastError();
            if (err != ERROR_FILE_NOT_FOUND) {
                setError(HRESULT_FROM_WIN32(err), L"Impossibile eliminare collegamento");
                return false;
            }
        }
    }

    return true;
}

} // namespace taskscheduler
