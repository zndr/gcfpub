#include "config.h"
#include "resource.h"
#include "task_scheduler.h"
#include "install_mode.h"
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace config {

static const wchar_t* SECTION_HOTKEY = L"Hotkey";
static const wchar_t* SECTION_GENERAL = L"General";

std::wstring getExePath() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

std::wstring getConfigPath() {
    // Assicurati che la cartella di configurazione esista
    installmode::ensureConfigDirExists();

    // Usa il percorso appropriato in base alla modalita'
    return installmode::getConfigFilePath();
}

static hotkeymanager::Modifier stringToModifier(const std::wstring& str) {
    if (str == L"ALT") return hotkeymanager::Modifier::ALT;
    if (str == L"SHIFT") return hotkeymanager::Modifier::SHIFT;
    return hotkeymanager::Modifier::CTRL;  // Default
}

static std::wstring modifierToString(hotkeymanager::Modifier mod) {
    switch (mod) {
        case hotkeymanager::Modifier::ALT: return L"ALT";
        case hotkeymanager::Modifier::SHIFT: return L"SHIFT";
        default: return L"CTRL";
    }
}

static UINT stringToVK(const std::wstring& str) {
    if (str.length() >= 7 && str.substr(0, 6) == L"NUMPAD") {
        int digit = str[6] - L'0';
        if (digit >= 0 && digit <= 9) {
            return VK_NUMPAD0 + digit;
        }
    }
    return VK_NUMPAD1;  // Default
}

static std::wstring vkToString(UINT vk) {
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        return L"NUMPAD" + std::to_wstring(vk - VK_NUMPAD0);
    }
    return L"NUMPAD1";  // Default
}

AppConfig load() {
    AppConfig cfg;
    std::wstring path = getConfigPath();

    // Verifica se il file esiste
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return cfg;  // Restituisci defaults
    }

    wchar_t buffer[64] = {0};

    // Leggi modificatore
    GetPrivateProfileStringW(SECTION_HOTKEY, L"Modifier", L"CTRL",
                             buffer, 64, path.c_str());
    cfg.hotkeyModifier = stringToModifier(buffer);

    // Leggi tasto
    GetPrivateProfileStringW(SECTION_HOTKEY, L"Key", L"NUMPAD1",
                             buffer, 64, path.c_str());
    cfg.hotkeyVK = stringToVK(buffer);

    // Leggi autostart
    cfg.autostart = GetPrivateProfileIntW(SECTION_GENERAL, L"Autostart", 0, path.c_str()) != 0;

    return cfg;
}

bool save(const AppConfig& cfg) {
    std::wstring path = getConfigPath();

    // Scrivi modificatore
    std::wstring modStr = modifierToString(cfg.hotkeyModifier);
    if (!WritePrivateProfileStringW(SECTION_HOTKEY, L"Modifier",
                                    modStr.c_str(), path.c_str())) {
        return false;
    }

    // Scrivi tasto
    std::wstring keyStr = vkToString(cfg.hotkeyVK);
    if (!WritePrivateProfileStringW(SECTION_HOTKEY, L"Key",
                                    keyStr.c_str(), path.c_str())) {
        return false;
    }

    // Scrivi autostart
    if (!WritePrivateProfileStringW(SECTION_GENERAL, L"Autostart",
                                    cfg.autostart ? L"1" : L"0", path.c_str())) {
        return false;
    }

    return true;
}

bool isAutostartEnabled() {
    // Usa Task Scheduler invece del Registry
    return taskscheduler::isTaskEnabled();
}

bool setAutostart(bool enable) {
    // Usa Task Scheduler invece del Registry
    return taskscheduler::setTaskEnabled(enable);
}

bool migrateFromRegistry() {
    // Verifica se esiste una voce autostart nel Registry (metodo legacy)
    if (!taskscheduler::hasLegacyRegistryAutostart()) {
        return true;  // Nulla da migrare
    }

    // Verifica se esiste gia' il task in Task Scheduler
    if (taskscheduler::isTaskEnabled()) {
        // Il task esiste gia', rimuovi solo la voce legacy
        taskscheduler::removeLegacyRegistryAutostart();
        return true;
    }

    // Migra: crea il task e rimuovi la voce registry
    bool taskCreated = taskscheduler::setTaskEnabled(true);
    if (taskCreated) {
        taskscheduler::removeLegacyRegistryAutostart();
    }

    return taskCreated;
}

} // namespace config
