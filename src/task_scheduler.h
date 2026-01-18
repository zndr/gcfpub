#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace taskscheduler {

/**
 * @brief Nome del task in Task Scheduler.
 */
static const wchar_t* TASK_NAME = L"MWCFExtractor_Autostart";

/**
 * @brief Verifica se il task di autostart esiste ed e' abilitato.
 *
 * @return true se il task esiste ed e' abilitato
 */
bool isTaskEnabled();

/**
 * @brief Crea o rimuove il task di autostart.
 *
 * @param enable true per creare il task, false per rimuoverlo
 * @return true se l'operazione e' riuscita
 */
bool setTaskEnabled(bool enable);

/**
 * @brief Verifica se esiste una voce autostart nel Registry (metodo legacy).
 *
 * @return true se esiste la voce nel Registry
 */
bool hasLegacyRegistryAutostart();

/**
 * @brief Rimuove la voce autostart dal Registry (metodo legacy).
 *
 * @return true se l'operazione e' riuscita
 */
bool removeLegacyRegistryAutostart();

/**
 * @brief Ottiene l'ultimo codice di errore delle operazioni Task Scheduler.
 *
 * @return Codice HRESULT dell'ultimo errore, o S_OK se nessun errore
 */
HRESULT getLastError();

/**
 * @brief Ottiene una descrizione dell'ultimo errore.
 *
 * @return Stringa con la descrizione dell'errore
 */
std::wstring getLastErrorMessage();

/**
 * @brief Verifica se esiste il collegamento sul desktop.
 *
 * @return true se il collegamento esiste
 */
bool hasDesktopShortcut();

/**
 * @brief Crea o rimuove il collegamento sul desktop.
 *
 * @param create true per creare, false per rimuovere
 * @return true se l'operazione e' riuscita
 */
bool setDesktopShortcut(bool create);

} // namespace taskscheduler

#endif // TASK_SCHEDULER_H
