#ifndef UPDATE_CHECKER_H
#define UPDATE_CHECKER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <functional>

namespace updatechecker {

/**
 * @brief URL del repository pubblico per il controllo aggiornamenti.
 */
static const wchar_t* GITHUB_REPO = L"zndr/gcfpub";
static const wchar_t* RELEASES_URL = L"https://github.com/zndr/gcfpub/releases";

/**
 * @brief Risultato del controllo aggiornamenti.
 */
struct UpdateCheckResult {
    bool success;           // true se il controllo e' riuscito
    bool updateAvailable;   // true se c'e' un aggiornamento disponibile
    std::wstring latestVersion;  // Versione piu' recente (es. "1.1.0")
    std::wstring currentVersion; // Versione corrente
    std::wstring errorMessage;   // Messaggio di errore (se success = false)
};

/**
 * @brief Callback per il controllo aggiornamenti asincrono.
 */
using UpdateCheckCallback = std::function<void(const UpdateCheckResult&)>;

/**
 * @brief Controlla se sono disponibili aggiornamenti.
 *
 * Esegue una richiesta HTTP all'API di GitHub per ottenere
 * l'ultima versione disponibile e la confronta con quella corrente.
 *
 * @return Risultato del controllo
 */
UpdateCheckResult checkForUpdates();

/**
 * @brief Controlla aggiornamenti in modo asincrono.
 *
 * Esegue il controllo in un thread separato e chiama il callback
 * quando il risultato e' disponibile.
 *
 * @param callback Funzione da chiamare con il risultato
 * @param hwnd Handle della finestra per il messaggio di notifica
 * @param message ID del messaggio da inviare quando completato
 */
void checkForUpdatesAsync(HWND hwnd, UINT message);

/**
 * @brief Ottiene il risultato dell'ultimo controllo asincrono.
 *
 * @return Risultato dell'ultimo controllo
 */
UpdateCheckResult getLastCheckResult();

/**
 * @brief Confronta due stringhe di versione.
 *
 * @param v1 Prima versione (es. "1.0.0")
 * @param v2 Seconda versione (es. "1.1.0")
 * @return -1 se v1 < v2, 0 se v1 == v2, 1 se v1 > v2
 */
int compareVersions(const std::wstring& v1, const std::wstring& v2);

/**
 * @brief Apre la pagina delle release nel browser predefinito.
 */
void openReleasesPage();

/**
 * @brief Ottiene la versione corrente dell'applicazione.
 *
 * @return Stringa della versione (es. "1.0.0")
 */
std::wstring getCurrentVersion();

} // namespace updatechecker

#endif // UPDATE_CHECKER_H
