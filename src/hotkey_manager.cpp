#include "hotkey_manager.h"
#include "resource.h"

namespace hotkeymanager {

// ============================================================================
// HotkeyConfig implementation
// ============================================================================

std::wstring HotkeyConfig::toString() const {
    return HotkeyManager::getModifierName(modifier) + L" + " +
           HotkeyManager::getKeyName(vkCode);
}

HotkeyConfig HotkeyConfig::getDefault(int id) {
    HotkeyConfig config;
    config.modifier = Modifier::CTRL;
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

std::wstring HotkeyManager::getModifierName(Modifier mod) {
    switch (mod) {
        case Modifier::CTRL:
            return L"CTRL";
        case Modifier::ALT:
            return L"ALT";
        case Modifier::SHIFT:
            return L"SHIFT";
        default:
            return L"???";
    }
}

std::wstring HotkeyManager::getKeyName(UINT vkCode) {
    int digit = getNumpadDigit(vkCode);
    if (digit >= 0) {
        return std::to_wstring(digit) + L" (Num)";
    }

    // Per altri tasti
    switch (vkCode) {
        case VK_F1: return L"F1";
        case VK_F2: return L"F2";
        case VK_F3: return L"F3";
        case VK_F4: return L"F4";
        case VK_F5: return L"F5";
        case VK_F6: return L"F6";
        case VK_F7: return L"F7";
        case VK_F8: return L"F8";
        case VK_F9: return L"F9";
        case VK_F10: return L"F10";
        case VK_F11: return L"F11";
        case VK_F12: return L"F12";
        default:
            return L"???";
    }
}

UINT HotkeyManager::getNumpadVK(int digit) {
    if (digit < 0 || digit > 9) {
        return 0;
    }
    return VK_NUMPAD0 + digit;
}

int HotkeyManager::getNumpadDigit(UINT vkCode) {
    if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
        return vkCode - VK_NUMPAD0;
    }
    return -1;
}

} // namespace hotkeymanager
