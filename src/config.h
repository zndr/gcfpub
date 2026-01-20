#ifndef CONFIG_H
#define CONFIG_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "hotkey_manager.h"

namespace config {

/**
 * @brief Configurazione dell'applicazione.
 */
struct AppConfig {
    UINT hotkeyModifiers;  ///< Combinazione di MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN
    UINT hotkeyVK;         ///< Codice del tasto virtuale
    bool autostart;

    AppConfig()
        : hotkeyModifiers(MOD_CONTROL)
        , hotkeyVK(VK_NUMPAD1)
        , autostart(false)
    {}
};

/**
 * @brief Carica la configurazione dal file INI.
 *
 * @return La configurazione caricata (o default se il file non esiste)
 */
AppConfig load();

/**
 * @brief Salva la configurazione nel file INI.
 *
 * @param cfg La configurazione da salvare
 * @return true se il salvataggio è riuscito
 */
bool save(const AppConfig& cfg);

/**
 * @brief Ottiene il percorso del file di configurazione.
 *
 * Il file .ini è nella stessa cartella dell'eseguibile.
 *
 * @return Percorso completo del file .ini
 */
std::wstring getConfigPath();

/**
 * @brief Ottiene il percorso dell'eseguibile corrente.
 *
 * @return Percorso completo dell'eseguibile
 */
std::wstring getExePath();

/**
 * @brief Verifica se l'autostart è abilitato nel registro.
 *
 * @return true se l'app è configurata per l'avvio automatico
 */
bool isAutostartEnabled();

/**
 * @brief Abilita o disabilita l'autostart tramite Task Scheduler.
 *
 * @param enable true per abilitare, false per disabilitare
 * @return true se l'operazione è riuscita
 */
bool setAutostart(bool enable);

/**
 * @brief Migra l'autostart dal Registry al Task Scheduler.
 *
 * Questa funzione verifica se esiste una voce autostart nel Registry
 * (metodo legacy) e la migra al Task Scheduler, rimuovendo poi la voce
 * dal Registry.
 *
 * @return true se la migrazione è riuscita o non era necessaria
 */
bool migrateFromRegistry();

} // namespace config

#endif // CONFIG_H
