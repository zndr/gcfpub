#ifndef INSTALL_MODE_H
#define INSTALL_MODE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace installmode {

/**
 * @brief Modalita' di esecuzione dell'applicazione.
 */
enum class Mode {
    Portable,   // Eseguibile standalone, config nella stessa cartella
    Installed   // Installato in Program Files, config in %APPDATA%
};

/**
 * @brief Chiave del registry per identificare l'installazione.
 */
static const wchar_t* INSTALL_REGISTRY_KEY = L"SOFTWARE\\MWCFExtractor";
static const wchar_t* INSTALL_REGISTRY_VALUE = L"InstallPath";

/**
 * @brief Nome della cartella in %APPDATA%.
 */
static const wchar_t* APPDATA_FOLDER = L"MWCFExtractor";

/**
 * @brief Determina la modalita' di esecuzione corrente.
 *
 * L'applicazione e' considerata "installata" se:
 * 1. Esiste la chiave HKLM\SOFTWARE\MWCFExtractor\InstallPath nel registry
 * 2. Oppure l'exe si trova in Program Files o Program Files (x86)
 *
 * @return Mode::Installed o Mode::Portable
 */
Mode getMode();

/**
 * @brief Ottiene il percorso della cartella di configurazione.
 *
 * - Portable: cartella dell'eseguibile
 * - Installed: %APPDATA%\MWCFExtractor
 *
 * @return Percorso della cartella di configurazione
 */
std::wstring getConfigDir();

/**
 * @brief Ottiene il percorso completo del file di configurazione.
 *
 * @return Percorso completo del file .ini
 */
std::wstring getConfigFilePath();

/**
 * @brief Crea la cartella di configurazione se non esiste.
 *
 * In modalita' portable non fa nulla (la cartella e' quella dell'exe).
 * In modalita' installata crea %APPDATA%\MWCFExtractor se non esiste.
 *
 * @return true se la cartella esiste o e' stata creata con successo
 */
bool ensureConfigDirExists();

/**
 * @brief Verifica se l'exe e' in una cartella Program Files.
 *
 * @return true se l'exe e' in Program Files o Program Files (x86)
 */
bool isInProgramFiles();

/**
 * @brief Ottiene il percorso di installazione dal registry.
 *
 * @return Percorso di installazione, o stringa vuota se non trovato
 */
std::wstring getRegistryInstallPath();

} // namespace installmode

#endif // INSTALL_MODE_H
