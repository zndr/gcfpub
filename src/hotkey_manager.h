#ifndef HOTKEY_MANAGER_H
#define HOTKEY_MANAGER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

namespace hotkeymanager {

/**
 * @brief Configurazione di una hotkey.
 *
 * Supporta qualsiasi combinazione di modificatori (CTRL, ALT, SHIFT, WIN)
 * e qualsiasi tasto virtuale.
 */
struct HotkeyConfig {
    UINT modifiers;     ///< Combinazione di modificatori (MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_WIN)
    UINT vkCode;        ///< Codice del tasto virtuale
    int hotkeyId;       ///< ID univoco per la registrazione

    HotkeyConfig() : modifiers(MOD_CONTROL), vkCode(VK_NUMPAD1), hotkeyId(0) {}

    /**
     * @brief Ottiene il codice del modificatore per RegisterHotKey.
     */
    UINT getModifierCode() const {
        return modifiers | MOD_NOREPEAT;
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
     * @brief Ottiene il nome dei modificatori.
     *
     * @param modifiers Combinazione di flag MOD_*
     * @return Nome leggibile dei modificatori (es. "CTRL + ALT")
     */
    static std::wstring getModifiersName(UINT modifiers);

    /**
     * @brief Ottiene il nome del tasto.
     *
     * @param vkCode Codice del tasto virtuale
     * @return Nome leggibile del tasto
     */
    static std::wstring getKeyName(UINT vkCode);

    /**
     * @brief Verifica se un tasto virtuale e' un modificatore.
     *
     * @param vkCode Codice del tasto virtuale
     * @return true se il tasto e' CTRL, ALT, SHIFT o WIN
     */
    static bool isModifierKey(UINT vkCode);

private:
    HWND m_hwnd;
    HotkeyConfig m_config;
    bool m_isRegistered;
};

} // namespace hotkeymanager

#endif // HOTKEY_MANAGER_H
