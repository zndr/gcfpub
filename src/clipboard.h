#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace clipboard {

/**
 * @brief Copia una stringa negli appunti di Windows.
 *
 * @param hwnd Handle della finestra proprietaria (può essere NULL)
 * @param text Il testo da copiare
 * @return true se l'operazione è riuscita, false altrimenti
 */
bool copyToClipboard(HWND hwnd, const std::wstring& text);

/**
 * @brief Legge il testo dagli appunti di Windows.
 *
 * @param hwnd Handle della finestra proprietaria (può essere NULL)
 * @return Il testo negli appunti, o stringa vuota se non disponibile
 */
std::wstring getFromClipboard(HWND hwnd);

/**
 * @brief Svuota gli appunti di Windows.
 *
 * @param hwnd Handle della finestra proprietaria (può essere NULL)
 * @return true se l'operazione è riuscita, false altrimenti
 */
bool clearClipboard(HWND hwnd);

} // namespace clipboard

#endif // CLIPBOARD_H
