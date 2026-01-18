#ifndef APP_ICON_H
#define APP_ICON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace appicon {

/**
 * @brief Crea l'icona dell'applicazione per la system tray.
 *
 * Genera un'icona con le lettere "CF" su sfondo colorato.
 *
 * @param size Dimensione dell'icona (16, 32, 48, etc.)
 * @param active true per icona attiva (verde), false per inattiva (grigio)
 * @return Handle dell'icona creata (deve essere distrutta con DestroyIcon)
 */
HICON createTrayIcon(int size, bool active);

/**
 * @brief Crea l'icona grande dell'applicazione.
 *
 * @param size Dimensione dell'icona (32, 48, 256, etc.)
 * @return Handle dell'icona creata
 */
HICON createAppIcon(int size);

} // namespace appicon

#endif // APP_ICON_H
