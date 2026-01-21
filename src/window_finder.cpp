#include "window_finder.h"
#include <algorithm>
#include <cctype>
#include <psapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "psapi.lib")

namespace windowfinder {

// Costanti per la ricerca
static const wchar_t* TARGET_PROCESS_NAME = L"millewin.exe";
static const wchar_t* TARGET_CLASS_PREFIX = L"FNWND";

/**
 * @brief Struttura per passare dati alla callback di EnumWindows.
 */
struct EnumWindowsData {
    std::vector<WindowInfo>* windows;
    bool verifyProcess;  // Se true, verifica anche il nome del processo
};

/**
 * @brief Confronta due stringhe in modo case-insensitive.
 */
static bool iequals(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (towlower(a[i]) != towlower(b[i])) return false;
    }
    return true;
}

/**
 * @brief Verifica se una stringa inizia con un prefisso (case-insensitive).
 */
static bool startsWith(const std::wstring& str, const std::wstring& prefix) {
    if (str.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); i++) {
        if (towlower(str[i]) != towlower(prefix[i])) return false;
    }
    return true;
}

std::wstring getWindowTitle(HWND hwnd) {
    int length = GetWindowTextLengthW(hwnd);
    if (length == 0) {
        return L"";
    }

    std::wstring title(length + 1, L'\0');
    GetWindowTextW(hwnd, &title[0], length + 1);
    title.resize(length);

    return title;
}

std::wstring getWindowClassName(HWND hwnd) {
    wchar_t className[256] = {0};
    GetClassNameW(hwnd, className, 256);
    return std::wstring(className);
}

/**
 * @brief Ottiene il nome del processo usando CreateToolhelp32Snapshot.
 *        Questo metodo è più affidabile di OpenProcess + QueryFullProcessImageName.
 */
static std::wstring getProcessNameFromSnapshot(DWORD processId) {
    std::wstring processName;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processName;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (pe32.th32ProcessID == processId) {
                processName = pe32.szExeFile;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return processName;
}

std::wstring getProcessName(DWORD processId) {
    // Metodo 1: Usa CreateToolhelp32Snapshot (più affidabile)
    std::wstring processName = getProcessNameFromSnapshot(processId);
    if (!processName.empty()) {
        return processName;
    }

    // Metodo 2: Fallback con OpenProcess (potrebbe fallire per permessi)
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess != NULL) {
        wchar_t exePath[MAX_PATH] = {0};
        DWORD size = MAX_PATH;

        if (QueryFullProcessImageNameW(hProcess, 0, exePath, &size)) {
            std::wstring fullPath(exePath);
            size_t lastSlash = fullPath.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                processName = fullPath.substr(lastSlash + 1);
            } else {
                processName = fullPath;
            }
        }

        CloseHandle(hProcess);
    }

    return processName;
}

bool isProcessRunning(const std::wstring& processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    bool found = false;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (iequals(pe32.szExeFile, processName)) {
                found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return found;
}

/**
 * @brief Callback per EnumWindows.
 */
static BOOL CALLBACK enumWindowsCallback(HWND hwnd, LPARAM lParam) {
    EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);

    // Ignora finestre non visibili
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    // Verifica PRIMA se la classe inizia con "FNWND"
    // Questo è il controllo più veloce e specifico
    std::wstring className = getWindowClassName(hwnd);
    if (!startsWith(className, TARGET_CLASS_PREFIX)) {
        return TRUE;
    }

    // Ottieni il PID del processo proprietario
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == 0) {
        return TRUE;
    }

    // Verifica opzionale del nome processo
    // (la classe FNWND* è già molto specifica per MilleWin)
    if (data->verifyProcess) {
        std::wstring processName = getProcessName(processId);
        if (!processName.empty() && !iequals(processName, TARGET_PROCESS_NAME)) {
            return TRUE;
        }
        // Se processName è vuoto, assumiamo che potrebbe essere MilleWin
        // (fallback per quando non riusciamo a ottenere il nome)
    }

    // Aggiungi la finestra alla lista
    WindowInfo info;
    info.hwnd = hwnd;
    info.title = getWindowTitle(hwnd);
    info.className = className;
    info.processId = processId;

    data->windows->push_back(info);

    return TRUE;
}

std::vector<WindowInfo> findMilleWinWindows() {
    std::vector<WindowInfo> windows;

    EnumWindowsData data;
    data.windows = &windows;
    data.verifyProcess = true;  // Prima prova con verifica processo

    EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data));

    // Se non troviamo nulla, riprova senza verifica del processo
    // (la classe FNWND* è comunque specifica per MilleWin)
    if (windows.empty()) {
        data.verifyProcess = false;
        EnumWindows(enumWindowsCallback, reinterpret_cast<LPARAM>(&data));
    }

    return windows;
}

std::optional<WindowInfo> findMainMilleWinWindow() {
    std::vector<WindowInfo> windows = findMilleWinWindows();

    if (windows.empty()) {
        return std::nullopt;
    }

    // Se abbiamo più finestre, cerca quella con un titolo che sembra
    // contenere dati del paziente (contiene un codice fiscale o info paziente)
    for (const auto& win : windows) {
        // Finestre con titolo lungo sono probabilmente quelle del paziente
        if (win.title.length() > 30) {
            return win;
        }
    }

    // Altrimenti restituisci la prima
    return windows[0];
}

bool isMillewinInstalled() {
    // Metodo 1: Cerca nel Registry le chiavi di installazione di MilleWin
    // MilleWin è prodotto da Millennium e crea chiavi sotto HKLM\SOFTWARE\Millennium
    const wchar_t* registryPaths[] = {
        L"SOFTWARE\\WOW6432Node\\Millennium\\Millewin",
        L"SOFTWARE\\Millennium\\Millewin",
        L"SOFTWARE\\WOW6432Node\\Dedalus\\Millewin",
        L"SOFTWARE\\Dedalus\\Millewin"
    };

    HKEY hKey;
    for (const auto& path : registryPaths) {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
    }

    // Metodo 2: Cerca l'eseguibile in percorsi comuni
    const wchar_t* commonPaths[] = {
        L"C:\\Millewin\\millewin.exe",
        L"C:\\Program Files\\Millewin\\millewin.exe",
        L"C:\\Program Files (x86)\\Millewin\\millewin.exe",
        L"C:\\Programmi\\Millewin\\millewin.exe"
    };

    for (const auto& exePath : commonPaths) {
        DWORD attrs = GetFileAttributesW(exePath);
        if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            return true;
        }
    }

    return false;
}

} // namespace windowfinder
