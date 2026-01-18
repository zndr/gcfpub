#include "tray_icon.h"
#include "app_icon.h"
#include "resource.h"

namespace trayicon {

TrayIcon::TrayIcon(HWND hwnd, UINT callbackMessage)
    : m_hwnd(hwnd)
    , m_callbackMessage(callbackMessage)
    , m_isVisible(false)
    , m_currentState(TrayIconState::Active)
{
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    m_nid.uCallbackMessage = m_callbackMessage;
    m_nid.uVersion = NOTIFYICON_VERSION_4;
}

TrayIcon::~TrayIcon() {
    remove();
}

HICON TrayIcon::loadIconForState(TrayIconState state) {
    // Usa l'icona generata programmaticamente
    bool isActive = (state == TrayIconState::Active);
    return appicon::createTrayIcon(16, isActive);
}

bool TrayIcon::create(const std::wstring& tooltip) {
    if (m_isVisible) {
        return true;
    }

    m_nid.hIcon = loadIconForState(m_currentState);
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);

    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
        m_isVisible = true;
        return true;
    }

    return false;
}

void TrayIcon::remove() {
    if (m_isVisible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_isVisible = false;
    }

    if (m_nid.hIcon) {
        DestroyIcon(m_nid.hIcon);
        m_nid.hIcon = NULL;
    }
}

void TrayIcon::setTooltip(const std::wstring& tooltip) {
    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);

    if (m_isVisible) {
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::setState(TrayIconState state) {
    if (state == m_currentState) {
        return;
    }

    m_currentState = state;

    // Distruggi la vecchia icona
    if (m_nid.hIcon) {
        DestroyIcon(m_nid.hIcon);
    }

    // Carica la nuova icona
    m_nid.hIcon = loadIconForState(state);

    if (m_isVisible) {
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::showBalloon(const std::wstring& title,
                           const std::wstring& message,
                           DWORD icon,
                           UINT timeout) {
    if (!m_isVisible) {
        return;
    }

    NOTIFYICONDATAW nidBalloon = m_nid;
    nidBalloon.uFlags |= NIF_INFO;
    nidBalloon.dwInfoFlags = icon;
    nidBalloon.uTimeout = timeout;

    wcsncpy_s(nidBalloon.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(nidBalloon.szInfo, message.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &nidBalloon);
}

void TrayIcon::showContextMenu(HMENU hMenu, int x, int y) {
    // Imposta la finestra in primo piano
    SetForegroundWindow(m_hwnd);

    // Mostra il menu
    TrackPopupMenu(hMenu,
                   TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON,
                   x, y, 0, m_hwnd, NULL);

    // Invia un messaggio nullo per chiudere correttamente il menu
    PostMessage(m_hwnd, WM_NULL, 0, 0);
}

HMENU createTrayMenu(bool hotkeyModified, bool autostartEnabled,
                     bool isPortable, bool hasDesktopIcon) {
    HMENU hMenu = CreatePopupMenu();

    if (hMenu) {
        // Salva hotkey (abilitato solo se modificata)
        UINT saveFlags = MF_STRING | (hotkeyModified ? 0 : MF_GRAYED);
        AppendMenuW(hMenu, saveFlags, IDM_TRAY_SAVE_HOTKEY, L"Salva hotkey");

        // Cambia hotkey
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_CHANGE_HOTKEY, L"Cambia hotkey...");

        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

        // Avvio automatico (checkbox)
        UINT autoFlags = MF_STRING | (autostartEnabled ? MF_CHECKED : MF_UNCHECKED);
        AppendMenuW(hMenu, autoFlags, IDM_TRAY_AUTOSTART, L"Avvio automatico");

        // Collegamento sul desktop (solo versione portabile, checkbox)
        if (isPortable) {
            UINT desktopFlags = MF_STRING | (hasDesktopIcon ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuW(hMenu, desktopFlags, IDM_TRAY_DESKTOP_ICON, L"Collegamento sul desktop");
        }

        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

        // Controlla aggiornamenti
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_CHECK_UPDATES, L"Controlla aggiornamenti...");

        // Informazioni
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_ABOUT, L"Informazioni");

        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

        // Esci
        AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Esci");
    }

    return hMenu;
}

} // namespace trayicon
