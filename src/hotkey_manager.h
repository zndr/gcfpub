#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace hotkeymanager {

/**
 * @brief Tipi di modificatori per hotkey.
 */
enum class Modifier {
    CTRL = MOD_CONTROL,
    ALT = MOD_ALT,
    SHIFT = MOD_SHIFT
};

/**
 * @brief Configurazione di una hotkey.
 */
struct HotkeyConfig {
    Modifier modifier;  ///< Modificatore (CTRL, ALT, SHIFT)
    UINT vkCode;        ///< Codice del tasto virtuale
    int hotkeyId;       ///< ID univoco per la registrazione

    /**
     * @brief Ottiene il codice del modificatore per RegisterHotKey.
     */
    UINT getModifierCode() const {
        return static_cast<UINT>(modifier) | MOD_NOREPEAT;
    }

    /**
     * @brief Ottiene una rappresentazione testuale della hotkey.
     */
    std::wstring toString() const;

    /**
     * @brief Crea la configurazione di default (CTRL + NUMPAD1).
     */
    static HotkeyConfig getDefault(int id);
};

/**
 * @brief Gestisce la registrazione e de-registrazione delle hotkey.
 */
class HotkeyManager {
public:
    /**
     * @brief Costruttore.
     *
     * @param hwnd Handle della finestra per la registrazione
     */
    explicit HotkeyManager(HWND hwnd);

    /**
     * @brief Distruttore - de-registra automaticamente la hotkey.
     */
    ~HotkeyManager();

    /**
     * @brief Registra la hotkey con la configurazione corrente.
     *
     * @return true se la registrazione è riuscita
     */
    bool registerHotkey();

    /**
     * @brief De-registra la hotkey corrente.
     */
    void unregisterHotkey();

    /**
     * @brief Verifica se la hotkey è attualmente registrata.
     */
    bool isRegistered() const { return m_isRegistered; }

    /**
     * @brief Imposta una nuova configurazione.
     *
     * @param config La nuova configurazione
     * @return true se la configurazione è stata applicata con successo
     */
    bool setConfig(const HotkeyConfig& config);

    /**
     * @brief Ottiene la configurazione corrente.
     */
    const HotkeyConfig& getConfig() const { return m_config; }

    /**
     * @brief Ottiene il nome del modificatore.
     *
     * @param mod Il modificatore
     * @return Nome leggibile del modificatore
     */
    static std::wstring getModifierName(Modifier mod);

    /**
     * @brief Ottiene il nome del tasto.
     *
     * @param vkCode Codice del tasto virtuale
     * @return Nome leggibile del tasto
     */
    static std::wstring getKeyName(UINT vkCode);

    /**
     * @brief Ottiene il codice VK per un tasto del numpad (0-9).
     *
     * @param digit Cifra (0-9)
     * @return Codice VK corrispondente, o 0 se non valido
     */
    static UINT getNumpadVK(int digit);

    /**
     * @brief Ottiene la cifra da un codice VK del numpad.
     *
     * @param vkCode Codice del tasto virtuale
     * @return Cifra (0-9), o -1 se non è un tasto numpad
     */
    static int getNumpadDigit(UINT vkCode);

private:
    HWND m_hwnd;
    HotkeyConfig m_config;
    bool m_isRegistered;
};

} // namespace hotkeymanager

#endif // HOTKEY_MANAGER_H
