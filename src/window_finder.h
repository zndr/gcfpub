#ifndef WINDOW_FINDER_H
#define WINDOW_FINDER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <optional>
#include <vector>

namespace windowfinder {

/**
 * @brief Informazioni su una finestra trovata.
 */
struct WindowInfo {
    HWND hwnd;              ///< Handle della finestra
    std::wstring title;     ///< Titolo della finestra
    std::wstring className; ///< Nome della classe della finestra
    DWORD processId;        ///< ID del processo proprietario
};

/**
 * @brief Cerca tutte le finestre di MilleWin con classe che inizia con "FNWND".
 *
 * @return Vettore di informazioni sulle finestre trovate
 */
std::vector<WindowInfo> findMilleWinWindows();

/**
 * @brief Cerca la finestra principale di MilleWin (prima trovata).
 *
 * @return Informazioni sulla finestra, o std::nullopt se non trovata
 */
std::optional<WindowInfo> findMainMilleWinWindow();

/**
 * @brief Verifica se un processo con il nome specificato è in esecuzione.
 *
 * @param processName Nome del processo (es. "millewin.exe")
 * @return true se il processo è in esecuzione
 */
bool isProcessRunning(const std::wstring& processName);

/**
 * @brief Ottiene il nome del processo dato il suo ID.
 *
 * @param processId ID del processo
 * @return Nome del processo, o stringa vuota se non trovato
 */
std::wstring getProcessName(DWORD processId);

/**
 * @brief Ottiene il titolo di una finestra.
 *
 * @param hwnd Handle della finestra
 * @return Titolo della finestra
 */
std::wstring getWindowTitle(HWND hwnd);

/**
 * @brief Ottiene il nome della classe di una finestra.
 *
 * @param hwnd Handle della finestra
 * @return Nome della classe
 */
std::wstring getWindowClassName(HWND hwnd);

} // namespace windowfinder

#endif // WINDOW_FINDER_H
