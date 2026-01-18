#ifndef OVERLAY_H
#define OVERLAY_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace overlay {

/**
 * @brief Tipo di notifica overlay.
 */
enum class OverlayType {
    Success,    ///< Successo (verde)
    Warning,    ///< Avviso (giallo)
    Error       ///< Errore (rosso)
};

/**
 * @brief Inizializza il sistema di overlay.
 *
 * @param hInstance Handle dell'istanza dell'applicazione
 * @return true se l'inizializzazione è riuscita
 */
bool initialize(HINSTANCE hInstance);

/**
 * @brief Pulisce le risorse del sistema di overlay.
 */
void cleanup();

/**
 * @brief Mostra una notifica overlay nell'angolo in basso a destra.
 *
 * La notifica appare sopra tutte le finestre e si chiude automaticamente
 * dopo il timeout specificato. Supporta multi-monitor con DPI diversi.
 *
 * @param title Titolo della notifica
 * @param message Messaggio della notifica
 * @param type Tipo di notifica (Success, Warning, Error)
 * @param durationMs Durata in millisecondi (default 3000)
 */
void show(const std::wstring& title,
          const std::wstring& message,
          OverlayType type = OverlayType::Success,
          UINT durationMs = 3000);

/**
 * @brief Nasconde la notifica overlay corrente.
 */
void hide();

/**
 * @brief Verifica se una notifica è attualmente visibile.
 */
bool isVisible();

} // namespace overlay

#endif // OVERLAY_H
