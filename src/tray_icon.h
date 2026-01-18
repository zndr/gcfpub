#ifndef TRAY_ICON_H
#define TRAY_ICON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace trayicon {

/**
 * @brief Stato dell'icona nella system tray.
 */
enum class TrayIconState {
    Active,     ///< Applicazione attiva (verde)
    Inactive,   ///< MilleWin non trovato (giallo)
    Paused      ///< In pausa (grigio)
};

/**
 * @brief Gestisce l'icona nella system tray di Windows.
 */
class TrayIcon {
public:
    /**
     * @brief Costruttore.
     *
     * @param hwnd Handle della finestra proprietaria
     * @param callbackMessage Messaggio WM_USER per le notifiche
     */
    TrayIcon(HWND hwnd, UINT callbackMessage);

    /**
     * @brief Distruttore - rimuove l'icona dalla tray.
     */
    ~TrayIcon();

    /**
     * @brief Crea e mostra l'icona nella tray.
     *
     * @param tooltip Testo del tooltip
     * @return true se l'operazione è riuscita
     */
    bool create(const std::wstring& tooltip);

    /**
     * @brief Rimuove l'icona dalla tray.
     */
    void remove();

    /**
     * @brief Aggiorna il tooltip dell'icona.
     *
     * @param tooltip Nuovo testo del tooltip
     */
    void setTooltip(const std::wstring& tooltip);

    /**
     * @brief Aggiorna lo stato/icona.
     *
     * @param state Nuovo stato
     */
    void setState(TrayIconState state);

    /**
     * @brief Mostra una notifica balloon/toast.
     *
     * @param title Titolo della notifica
     * @param message Messaggio della notifica
     * @param icon Tipo di icona (NIIF_INFO, NIIF_WARNING, NIIF_ERROR)
     * @param timeout Timeout in millisecondi (ignorato in Windows 10+)
     */
    void showBalloon(const std::wstring& title,
                     const std::wstring& message,
                     DWORD icon = NIIF_INFO,
                     UINT timeout = 2000);

    /**
     * @brief Mostra il menu contestuale della tray.
     *
     * @param hMenu Handle del menu
     * @param x Coordinata X
     * @param y Coordinata Y
     */
    void showContextMenu(HMENU hMenu, int x, int y);

    /**
     * @brief Verifica se l'icona è visibile.
     */
    bool isVisible() const { return m_isVisible; }

private:
    HWND m_hwnd;
    UINT m_callbackMessage;
    NOTIFYICONDATAW m_nid;
    bool m_isVisible;
    TrayIconState m_currentState;

    /**
     * @brief Carica l'icona appropriata per lo stato.
     */
    HICON loadIconForState(TrayIconState state);
};

/**
 * @brief Crea il menu contestuale della tray.
 *
 * @param hotkeyModified true se la hotkey è stata modificata (abilita "Salva")
 * @param autostartEnabled true se l'avvio automatico è abilitato
 * @param isPortable true se è la versione portabile
 * @param hasDesktopIcon true se esiste già il collegamento sul desktop
 * @return Handle del menu creato (deve essere distrutto dal chiamante)
 */
HMENU createTrayMenu(bool hotkeyModified = false, bool autostartEnabled = false,
                     bool isPortable = true, bool hasDesktopIcon = false);

} // namespace trayicon

#endif // TRAY_ICON_H
