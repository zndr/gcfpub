/**
 * @file main.cpp
 * @brief MilleWin CF Extractor - Entry point e logica principale
 *
 * Applicazione portabile per Windows che estrae il codice fiscale dalla
 * finestra di MilleWin e lo copia negli appunti tramite hotkey.
 */

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shellapi.h>
#include <shellscalingapi.h>
#include <memory>
#include <string>

#include "resource.h"
#include "hotkey_manager.h"
#include "window_finder.h"
#include "cf_parser.h"
#include "clipboard.h"
#include "tray_icon.h"
#include "dialogs.h"
#include "overlay.h"
#include "config.h"
#include "app_icon.h"

#pragma comment(lib, "shcore.lib")

// ============================================================================
// Global variables
// ============================================================================

static HWND g_hwndMain = NULL;
static std::unique_ptr<hotkeymanager::HotkeyManager> g_hotkeyManager;
static std::unique_ptr<trayicon::TrayIcon> g_trayIcon;
static config::AppConfig g_config;
static config::AppConfig g_savedConfig;  // Configurazione salvata su file
static bool g_hotkeyModified = false;    // true se la hotkey è stata modificata
static bool g_running = true;

// ============================================================================
// Forward declarations
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool initializeApplication(HINSTANCE hInstance);
void processHotkeyAction();
bool tryRegisterHotkey();
void showConfigDialog();
void saveHotkey();
void toggleAutostart();
void cleanup();
void enableDpiAwareness();

// ============================================================================
// DPI Awareness
// ============================================================================

void enableDpiAwareness() {
    // Windows 10 1703+: Per-Monitor V2
    typedef BOOL (WINAPI *SetProcessDpiAwarenessContextFunc)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pSetProcessDpiAwarenessContext =
            (SetProcessDpiAwarenessContextFunc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetProcessDpiAwarenessContext) {
            pSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }

    // Windows 8.1+: Per-Monitor
    typedef HRESULT (WINAPI *SetProcessDpiAwarenessFunc)(int);
    HMODULE hShcore = LoadLibraryW(L"shcore.dll");
    if (hShcore) {
        auto pSetProcessDpiAwareness =
            (SetProcessDpiAwarenessFunc)GetProcAddress(hShcore, "SetProcessDpiAwareness");
        if (pSetProcessDpiAwareness) {
            pSetProcessDpiAwareness(2);  // PROCESS_PER_MONITOR_DPI_AWARE
            return;
        }
    }

    // Fallback: System DPI aware
    SetProcessDPIAware();
}

// ============================================================================
// Entry point
// ============================================================================

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Abilita DPI awareness prima di qualsiasi altra operazione
    enableDpiAwareness();

    // Verifica che non ci sia già un'istanza in esecuzione
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"Global\\MWCFExtractor_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, L"L'applicazione e' gia' in esecuzione.",
                   APP_NAME, MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
        return 0;
    }

    // Inizializza l'applicazione
    if (!initializeApplication(hInstance)) {
        MessageBoxW(NULL, L"Errore durante l'inizializzazione dell'applicazione.",
                   APP_NAME, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }

    // Message loop
    MSG msg;
    while (g_running && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    cleanup();
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

    return (int)msg.wParam;
}

// ============================================================================
// Application initialization
// ============================================================================

bool initializeApplication(HINSTANCE hInstance) {
    // Carica la configurazione
    g_config = config::load();
    g_savedConfig = g_config;

    // Migra autostart da Registry a Task Scheduler (se necessario)
    config::migrateFromRegistry();

    // Verifica stato autostart dal Task Scheduler
    g_config.autostart = config::isAutostartEnabled();

    // Registra la classe della finestra
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = APP_CLASS_NAME;

    // Prova a caricare l'icona dalla risorsa, altrimenti genera dinamicamente
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if (!wcex.hIcon) {
        wcex.hIcon = appicon::createAppIcon(32);
    }
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    if (!wcex.hIconSm) {
        wcex.hIconSm = appicon::createAppIcon(16);
    }

    if (!RegisterClassExW(&wcex)) {
        return false;
    }

    // Crea la finestra nascosta (per ricevere i messaggi)
    g_hwndMain = CreateWindowExW(
        0,
        APP_CLASS_NAME,
        APP_NAME,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, hInstance, NULL
    );

    if (g_hwndMain == NULL) {
        return false;
    }

    // Inizializza overlay
    if (!overlay::initialize(hInstance)) {
        // Non fatale, continua comunque
    }

    // Crea il gestore hotkey con la configurazione caricata
    g_hotkeyManager = std::make_unique<hotkeymanager::HotkeyManager>(g_hwndMain);

    hotkeymanager::HotkeyConfig hkConfig;
    hkConfig.modifier = g_config.hotkeyModifier;
    hkConfig.vkCode = g_config.hotkeyVK;
    hkConfig.hotkeyId = HOTKEY_ID;
    g_hotkeyManager->setConfig(hkConfig);

    // Crea l'icona tray
    g_trayIcon = std::make_unique<trayicon::TrayIcon>(g_hwndMain, WM_TRAYICON);

    // Prova a registrare la hotkey
    if (!tryRegisterHotkey()) {
        // L'utente ha annullato - esci
        return false;
    }

    // Mostra l'icona nella tray
    std::wstring tooltip = std::wstring(APP_NAME) + L" - " +
                           g_hotkeyManager->getConfig().toString();
    g_trayIcon->create(tooltip);

    // Mostra notifica di avvio
    overlay::show(L"CF Extractor avviato",
                  g_hotkeyManager->getConfig().toString(),
                  overlay::OverlayType::Success,
                  2000);

    return true;
}

// ============================================================================
// Hotkey registration with fallback dialog
// ============================================================================

bool tryRegisterHotkey() {
    // Prima prova con la configurazione corrente
    if (g_hotkeyManager->registerHotkey()) {
        return true;
    }

    // La registrazione è fallita, mostra il dialog
    std::wstring errorMsg = L"La combinazione " +
                            g_hotkeyManager->getConfig().toString() +
                            L" non e' disponibile.";

    while (true) {
        dialogs::HotkeyDialogResult result = dialogs::showHotkeyConfigDialog(
            g_hwndMain,
            g_hotkeyManager->getConfig(),
            errorMsg
        );

        if (!result.accepted) {
            // L'utente ha annullato
            return false;
        }

        // Prova a registrare la nuova combinazione
        g_hotkeyManager->unregisterHotkey();
        hotkeymanager::HotkeyConfig newConfig = result.config;
        newConfig.hotkeyId = HOTKEY_ID;

        if (g_hotkeyManager->setConfig(newConfig) && g_hotkeyManager->registerHotkey()) {
            // Aggiorna configurazione
            g_config.hotkeyModifier = newConfig.modifier;
            g_config.hotkeyVK = newConfig.vkCode;

            // Verifica se è diversa da quella salvata
            g_hotkeyModified = (g_config.hotkeyModifier != g_savedConfig.hotkeyModifier ||
                                g_config.hotkeyVK != g_savedConfig.hotkeyVK);
            return true;
        }

        // Fallita anche questa combinazione
        errorMsg = L"La combinazione " +
                   g_hotkeyManager->getConfig().toString() +
                   L" non e' disponibile. Scegline un'altra.";
    }
}

// ============================================================================
// Process hotkey action
// ============================================================================

void processHotkeyAction() {
    // Cerca la finestra di MilleWin
    auto windowInfo = windowfinder::findMainMilleWinWindow();

    if (!windowInfo.has_value()) {
        // MilleWin non trovato
        dialogs::showMilleWinNotFoundMessage(NULL);
        return;
    }

    const std::wstring& title = windowInfo->title;

    // Verifica se siamo nella schermata "Ricerca paziente"
    bool startsWithVersion = (title.find(L"MilleWin versione") == 0);
    bool containsRicerca = (title.find(L"Ricerca paziente") != std::wstring::npos);

    if (startsWithVersion && containsRicerca) {
        dialogs::showNoPatientMessage(NULL);
        return;
    }

    // Prova a estrarre il codice fiscale dal titolo
    auto cfOpt = cfparser::extractCodiceFiscale(title);

    if (!cfOpt.has_value()) {
        dialogs::showNoPatientMessage(NULL);
        return;
    }

    std::wstring cf = cfOpt.value();

    // Converti da omocodia a formato standard
    std::wstring cfNormalized = cfparser::normalizeOmocodia(cf);

    // Copia il codice fiscale normalizzato negli appunti
    if (clipboard::copyToClipboard(g_hwndMain, cfNormalized)) {
        // Mostra overlay di successo
        std::wstring message = cfNormalized;
        if (cf != cfNormalized) {
            message += L" (da omocodice)";
        }

        overlay::show(L"Codice Fiscale copiato", message,
                      overlay::OverlayType::Success, OVERLAY_TIMEOUT_MS);

        // Suona un beep di conferma
        MessageBeep(MB_OK);
    } else {
        dialogs::showErrorMessage(
            NULL,
            L"Errore",
            L"Impossibile copiare il codice fiscale negli appunti."
        );
    }
}

// ============================================================================
// Show configuration dialog
// ============================================================================

void showConfigDialog() {
    dialogs::HotkeyDialogResult result = dialogs::showHotkeyConfigDialog(
        g_hwndMain,
        g_hotkeyManager->getConfig(),
        L""
    );

    if (result.accepted) {
        // Prova a registrare la nuova combinazione
        g_hotkeyManager->unregisterHotkey();
        hotkeymanager::HotkeyConfig newConfig = result.config;
        newConfig.hotkeyId = HOTKEY_ID;

        if (g_hotkeyManager->setConfig(newConfig) && g_hotkeyManager->registerHotkey()) {
            // Aggiorna configurazione
            g_config.hotkeyModifier = newConfig.modifier;
            g_config.hotkeyVK = newConfig.vkCode;

            // Verifica se è diversa da quella salvata
            g_hotkeyModified = (g_config.hotkeyModifier != g_savedConfig.hotkeyModifier ||
                                g_config.hotkeyVK != g_savedConfig.hotkeyVK);

            // Aggiorna il tooltip
            std::wstring tooltip = std::wstring(APP_NAME) + L" - " +
                                   g_hotkeyManager->getConfig().toString();
            g_trayIcon->setTooltip(tooltip);

            overlay::show(L"Hotkey aggiornata",
                          g_hotkeyManager->getConfig().toString(),
                          overlay::OverlayType::Success, 2000);
        } else {
            dialogs::showErrorMessage(
                g_hwndMain,
                L"Errore",
                L"Impossibile registrare la combinazione di tasti selezionata."
            );

            // Riprova con il dialog
            tryRegisterHotkey();
        }
    }
}

// ============================================================================
// Save hotkey configuration
// ============================================================================

void saveHotkey() {
    g_savedConfig.hotkeyModifier = g_config.hotkeyModifier;
    g_savedConfig.hotkeyVK = g_config.hotkeyVK;

    if (config::save(g_config)) {
        g_hotkeyModified = false;
        overlay::show(L"Configurazione salvata",
                      g_hotkeyManager->getConfig().toString(),
                      overlay::OverlayType::Success, 2000);
    } else {
        dialogs::showErrorMessage(NULL, L"Errore",
                                  L"Impossibile salvare la configurazione.");
    }
}

// ============================================================================
// Toggle autostart
// ============================================================================

void toggleAutostart() {
    bool newState = !g_config.autostart;

    if (config::setAutostart(newState)) {
        g_config.autostart = newState;
        config::save(g_config);

        if (newState) {
            overlay::show(L"Avvio automatico",
                          L"Abilitato",
                          overlay::OverlayType::Success, 2000);
        } else {
            overlay::show(L"Avvio automatico",
                          L"Disabilitato",
                          overlay::OverlayType::Warning, 2000);
        }
    } else {
        dialogs::showErrorMessage(NULL, L"Errore",
                                  L"Impossibile modificare l'avvio automatico.");
    }
}

// ============================================================================
// Cleanup
// ============================================================================

void cleanup() {
    overlay::cleanup();

    if (g_hotkeyManager) {
        g_hotkeyManager->unregisterHotkey();
        g_hotkeyManager.reset();
    }

    if (g_trayIcon) {
        g_trayIcon->remove();
        g_trayIcon.reset();
    }
}

// ============================================================================
// Window procedure
// ============================================================================

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HOTKEY:
            if (wParam == HOTKEY_ID) {
                processHotkeyAction();
            }
            return 0;

        case WM_TRAYICON:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU: {
                    // Mostra il menu contestuale
                    POINT pt;
                    GetCursorPos(&pt);
                    HMENU hMenu = trayicon::createTrayMenu(g_hotkeyModified, g_config.autostart);
                    if (hMenu) {
                        g_trayIcon->showContextMenu(hMenu, pt.x, pt.y);
                        DestroyMenu(hMenu);
                    }
                    break;
                }

                case WM_LBUTTONDBLCLK:
                    // Doppio click - esegui l'azione hotkey
                    processHotkeyAction();
                    break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_TRAY_SAVE_HOTKEY:
                    saveHotkey();
                    break;

                case IDM_TRAY_CHANGE_HOTKEY:
                    showConfigDialog();
                    break;

                case IDM_TRAY_AUTOSTART:
                    toggleAutostart();
                    break;

                case IDM_TRAY_ABOUT:
                    dialogs::showAboutDialog(hwnd);
                    break;

                case IDM_TRAY_EXIT:
                    g_running = false;
                    PostQuitMessage(0);
                    break;
            }
            return 0;

        case WM_CLOSE:
            // Nascondi invece di chiudere
            ShowWindow(hwnd, SW_HIDE);
            return 0;

        case WM_DESTROY:
            cleanup();
            g_running = false;
            PostQuitMessage(0);
            return 0;

        case WM_DPICHANGED: {
            // Gestione cambio DPI (spostamento tra monitor)
            RECT* pRect = (RECT*)lParam;
            SetWindowPos(hwnd, NULL,
                         pRect->left, pRect->top,
                         pRect->right - pRect->left,
                         pRect->bottom - pRect->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}
