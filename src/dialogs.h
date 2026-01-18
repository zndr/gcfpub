#ifndef DIALOGS_H
#define DIALOGS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "hotkey_manager.h"

namespace dialogs {

/**
 * @brief Risultato del dialog di configurazione hotkey.
 */
struct HotkeyDialogResult {
    bool accepted;                          ///< true se l'utente ha accettato
    hotkeymanager::HotkeyConfig config;     ///< Configurazione selezionata
};

/**
 * @brief Mostra il dialog per configurare la hotkey.
 *
 * @param hwndParent Handle della finestra padre
 * @param currentConfig Configurazione attuale
 * @param errorMessage Messaggio di errore da mostrare (opzionale)
 * @return Risultato del dialog
 */
HotkeyDialogResult showHotkeyConfigDialog(
    HWND hwndParent,
    const hotkeymanager::HotkeyConfig& currentConfig,
    const std::wstring& errorMessage = L""
);

/**
 * @brief Mostra un MessageBox che si chiude automaticamente dopo un timeout.
 *
 * @param hwndParent Handle della finestra padre
 * @param title Titolo del MessageBox
 * @param message Testo del messaggio
 * @param timeoutMs Timeout in millisecondi
 * @param icon Icona da mostrare (MB_ICONINFORMATION, etc.)
 * @return ID del pulsante premuto (IDOK, IDCANCEL, etc.) o 0 se timeout
 */
int showTimedMessageBox(
    HWND hwndParent,
    const std::wstring& title,
    const std::wstring& message,
    UINT timeoutMs,
    UINT icon = MB_ICONINFORMATION
);

/**
 * @brief Mostra il dialog "Informazioni" sull'applicazione.
 *
 * @param hwndParent Handle della finestra padre
 */
void showAboutDialog(HWND hwndParent);

/**
 * @brief Mostra un messaggio di errore.
 *
 * @param hwndParent Handle della finestra padre
 * @param title Titolo
 * @param message Messaggio
 */
void showErrorMessage(HWND hwndParent, const std::wstring& title,
                      const std::wstring& message);

/**
 * @brief Mostra il messaggio "Nessun paziente".
 *
 * @param hwndParent Handle della finestra padre
 */
void showNoPatientMessage(HWND hwndParent);

/**
 * @brief Mostra il messaggio "MilleWin non trovato".
 *
 * @param hwndParent Handle della finestra padre
 */
void showMilleWinNotFoundMessage(HWND hwndParent);

} // namespace dialogs

#endif // DIALOGS_H
