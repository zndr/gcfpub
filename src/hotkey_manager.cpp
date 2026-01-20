#include "hotkey_manager.h"
#include "resource.h"

namespace hotkeymanager {

// ============================================================================
// HotkeyConfig implementation
// ============================================================================

std::wstring HotkeyConfig::toString() const {
    std::wstring result = HotkeyManager::getModifiersName(modifiers);
    if (!result.empty()) {
        result += L" + ";
    }
    result += HotkeyManager::getKeyName(vkCode);
    return result;
}

HotkeyConfig HotkeyConfig::getDefault(int id) {
    HotkeyConfig config;
    config.modifiers = MOD_CONTROL;
    config.vkCode = VK_NUMPAD1;
    config.hotkeyId = id;
    return config;
}

// ============================================================================
// HotkeyManager implementation
// ============================================================================

HotkeyManager::HotkeyManager(HWND hwnd)
    : m_hwnd(hwnd)
    , m_config(HotkeyConfig::getDefault(HOTKEY_ID))
    , m_isRegistered(false)
{
}

HotkeyManager::~HotkeyManager() {
    unregisterHotkey();
}

bool HotkeyManager::registerHotkey() {
    // De-registra prima se già registrata
    if (m_isRegistered) {
        unregisterHotkey();
    }

    // Tenta la registrazione
    BOOL result = RegisterHotKey(
        m_hwnd,
        m_config.hotkeyId,
        m_config.getModifierCode(),
        m_config.vkCode
    );

    m_isRegistered = (result != FALSE);
    return m_isRegistered;
}

void HotkeyManager::unregisterHotkey() {
    if (m_isRegistered) {
        UnregisterHotKey(m_hwnd, m_config.hotkeyId);
        m_isRegistered = false;
    }
}

bool HotkeyManager::setConfig(const HotkeyConfig& config) {
    // Salva il vecchio stato
    bool wasRegistered = m_isRegistered;

    // De-registra la vecchia hotkey
    unregisterHotkey();

    // Imposta la nuova configurazione
    m_config = config;

    // Se era registrata, prova a registrare la nuova
    if (wasRegistered) {
        return registerHotkey();
    }

    return true;
}

std::wstring HotkeyManager::getModifiersName(UINT modifiers) {
    std::wstring result;

    if (modifiers & MOD_CONTROL) {
        result += L"CTRL";
    }
    if (modifiers & MOD_ALT) {
        if (!result.empty()) result += L" + ";
        result += L"ALT";
    }
    if (modifiers & MOD_SHIFT) {
        if (!result.empty()) result += L" + ";
        result += L"SHIFT";
    }
    if (modifiers & MOD_WIN) {
        if (!result.empty()) result += L" + ";
        result += L"WIN";
    }

    return result;
}

std::wstring HotkeyManager::getKeyName(UINT vkCode) {
    // Numpad 0-9
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        return std::to_wstring(vkCode - VK_NUMPAD0) + L" (Num)";
    }

    // Lettere A-Z
    if (vkCode >= 'A' && vkCode <= 'Z') {
        return std::wstring(1, static_cast<wchar_t>(vkCode));
    }

    // Numeri 0-9
    if (vkCode >= '0' && vkCode <= '9') {
        return std::wstring(1, static_cast<wchar_t>(vkCode));
    }

    // Tasti funzione F1-F24
    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return L"F" + std::to_wstring(vkCode - VK_F1 + 1);
    }

    // Altri tasti comuni
    switch (vkCode) {
        // Tasti speciali
        case VK_SPACE: return L"SPACE";
        case VK_RETURN: return L"ENTER";
        case VK_TAB: return L"TAB";
        case VK_ESCAPE: return L"ESC";
        case VK_BACK: return L"BACKSPACE";
        case VK_DELETE: return L"DELETE";
        case VK_INSERT: return L"INSERT";
        case VK_HOME: return L"HOME";
        case VK_END: return L"END";
        case VK_PRIOR: return L"PAGE UP";
        case VK_NEXT: return L"PAGE DOWN";

        // Frecce
        case VK_UP: return L"UP";
        case VK_DOWN: return L"DOWN";
        case VK_LEFT: return L"LEFT";
        case VK_RIGHT: return L"RIGHT";

        // Numpad operatori
        case VK_MULTIPLY: return L"* (Num)";
        case VK_ADD: return L"+ (Num)";
        case VK_SUBTRACT: return L"- (Num)";
        case VK_DECIMAL: return L". (Num)";
        case VK_DIVIDE: return L"/ (Num)";

        // Punteggiatura
        case VK_OEM_1: return L";";
        case VK_OEM_PLUS: return L"=";
        case VK_OEM_COMMA: return L",";
        case VK_OEM_MINUS: return L"-";
        case VK_OEM_PERIOD: return L".";
        case VK_OEM_2: return L"/";
        case VK_OEM_3: return L"`";
        case VK_OEM_4: return L"[";
        case VK_OEM_5: return L"\\";
        case VK_OEM_6: return L"]";
        case VK_OEM_7: return L"'";

        // Altri
        case VK_PAUSE: return L"PAUSE";
        case VK_SCROLL: return L"SCROLL LOCK";
        case VK_NUMLOCK: return L"NUM LOCK";
        case VK_CAPITAL: return L"CAPS LOCK";
        case VK_PRINT: return L"PRINT SCREEN";
        case VK_SNAPSHOT: return L"PRINT SCREEN";

        default:
            // Usa GetKeyNameText per ottenere il nome dal sistema
            UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
            if (scanCode != 0) {
                wchar_t keyName[64] = {0};
                int result = GetKeyNameTextW(scanCode << 16, keyName, 64);
                if (result > 0) {
                    return std::wstring(keyName);
                }
            }
            // Fallback: mostra il codice VK
            return L"VK_" + std::to_wstring(vkCode);
    }
}

bool HotkeyManager::isModifierKey(UINT vkCode) {
    switch (vkCode) {
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL:
        case VK_MENU:      // ALT
        case VK_LMENU:
        case VK_RMENU:
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
        case VK_LWIN:
        case VK_RWIN:
            return true;
        default:
            return false;
    }
}

} // namespace hotkeymanager
