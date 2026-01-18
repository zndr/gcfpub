#include "install_mode.h"
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace installmode {

static const wchar_t* CONFIG_FILENAME = L"mwcf_extractor.ini";

static std::wstring getExeDir() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return std::wstring(path);
}

std::wstring getRegistryInstallPath() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        INSTALL_REGISTRY_KEY,
        0, KEY_READ | KEY_WOW64_64KEY, &hKey
    );

    if (result != ERROR_SUCCESS) {
        // Prova anche senza KEY_WOW64_64KEY per compatibilita'
        result = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            INSTALL_REGISTRY_KEY,
            0, KEY_READ, &hKey
        );
        if (result != ERROR_SUCCESS) {
            return L"";
        }
    }

    wchar_t value[MAX_PATH] = {0};
    DWORD size = sizeof(value);
    DWORD type = 0;

    result = RegQueryValueExW(hKey, INSTALL_REGISTRY_VALUE, NULL, &type,
                              (LPBYTE)value, &size);

    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS && type == REG_SZ) {
        return std::wstring(value);
    }

    return L"";
}

bool isInProgramFiles() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Converti in lowercase per confronto case-insensitive
    std::wstring exePathLower(exePath);
    for (auto& c : exePathLower) {
        c = towlower(c);
    }

    // Ottieni il percorso di Program Files
    wchar_t programFiles[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILES, NULL, 0, programFiles))) {
        std::wstring pfLower(programFiles);
        for (auto& c : pfLower) {
            c = towlower(c);
        }
        if (exePathLower.find(pfLower) == 0) {
            return true;
        }
    }

    // Ottieni il percorso di Program Files (x86)
    wchar_t programFilesX86[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, programFilesX86))) {
        std::wstring pfx86Lower(programFilesX86);
        for (auto& c : pfx86Lower) {
            c = towlower(c);
        }
        if (exePathLower.find(pfx86Lower) == 0) {
            return true;
        }
    }

    return false;
}

Mode getMode() {
    // Prima controlla se esiste la chiave nel registry
    std::wstring installPath = getRegistryInstallPath();
    if (!installPath.empty()) {
        return Mode::Installed;
    }

    // Altrimenti controlla se siamo in Program Files
    if (isInProgramFiles()) {
        return Mode::Installed;
    }

    return Mode::Portable;
}

std::wstring getConfigDir() {
    if (getMode() == Mode::Portable) {
        return getExeDir();
    }

    // Modalita' installata: usa %APPDATA%\MWCFExtractor
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::wstring configDir = std::wstring(appDataPath) + L"\\" + APPDATA_FOLDER;
        return configDir;
    }

    // Fallback alla cartella dell'exe
    return getExeDir();
}

std::wstring getConfigFilePath() {
    std::wstring configDir = getConfigDir();
    return configDir + L"\\" + CONFIG_FILENAME;
}

bool ensureConfigDirExists() {
    if (getMode() == Mode::Portable) {
        // La cartella dell'exe esiste sempre
        return true;
    }

    std::wstring configDir = getConfigDir();

    // Verifica se la cartella esiste
    DWORD attrs = GetFileAttributesW(configDir.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    // Crea la cartella
    return CreateDirectoryW(configDir.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

} // namespace installmode
