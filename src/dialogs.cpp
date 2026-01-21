#include "dialogs.h"
#include "resource.h"
#include <commctrl.h>
#include <shellscalingapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shcore.lib")

namespace dialogs {

// ============================================================================
// Dati globali per il dialog hotkey
// ============================================================================

static hotkeymanager::HotkeyConfig g_dialogConfig;
static hotkeymanager::HotkeyConfig g_capturedConfig;
static std::wstring g_dialogError;
static bool g_dialogAccepted = false;
static bool g_hasCapturedHotkey = false;
static WNDPROC g_originalStaticProc = nullptr;

// ============================================================================
// Utility functions
// ============================================================================

static void updateCurrentLabel(HWND hDlg) {
    std::wstring label = g_capturedConfig.toString();
    SetDlgItemTextW(hDlg, IDC_STATIC_CURRENT, label.c_str());
}

// Ottiene i modificatori attualmente premuti
static UINT getCurrentModifiers() {
    UINT modifiers = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000) modifiers |= MOD_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000) modifiers |= MOD_ALT;
    if (GetKeyState(VK_SHIFT) & 0x8000) modifiers |= MOD_SHIFT;
    if ((GetKeyState(VK_LWIN) & 0x8000) || (GetKeyState(VK_RWIN) & 0x8000)) modifiers |= MOD_WIN;
    return modifiers;
}

// Subclass procedure per catturare i tasti
static LRESULT CALLBACK HotkeyCaptureProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_GETDLGCODE:
            // Richiedi tutti i tasti, inclusi Tab, Enter, ecc.
            return DLGC_WANTALLKEYS;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            UINT vkCode = (UINT)wParam;

            // Ignora i tasti modificatori da soli
            if (hotkeymanager::HotkeyManager::isModifierKey(vkCode)) {
                // Mostra i modificatori premuti temporaneamente
                UINT mods = getCurrentModifiers();
                if (mods != 0) {
                    std::wstring label = hotkeymanager::HotkeyManager::getModifiersName(mods) + L" + ...";
                    SetWindowTextW(hwnd, label.c_str());
                }
                return 0;
            }

            // Cattura la combinazione
            UINT modifiers = getCurrentModifiers();

            // Richiedi almeno un modificatore per la maggior parte dei tasti
            // (esclusi F1-F24 che possono funzionare da soli)
            bool isFunctionKey = (vkCode >= VK_F1 && vkCode <= VK_F24);
            if (modifiers == 0 && !isFunctionKey) {
                SetWindowTextW(hwnd, L"Usa almeno un modificatore (CTRL, ALT, SHIFT, WIN)");
                return 0;
            }

            // Salva la configurazione catturata
            g_capturedConfig.modifiers = modifiers;
            g_capturedConfig.vkCode = vkCode;
            g_hasCapturedHotkey = true;

            // Aggiorna il controllo
            std::wstring label = g_capturedConfig.toString();
            SetWindowTextW(hwnd, label.c_str());

            // Aggiorna anche l'etichetta "combinazione attuale"
            HWND hDlg = GetParent(hwnd);
            updateCurrentLabel(hDlg);

            // Pulisci eventuali errori
            SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, L"");

            return 0;
        }

        case WM_CHAR:
        case WM_SYSCHAR:
            // Ignora i caratteri per evitare il beep
            return 0;

        case WM_SETFOCUS:
            // Quando riceve il focus, mostra istruzioni
            if (!g_hasCapturedHotkey) {
                SetWindowTextW(hwnd, L"Premi la combinazione di tasti...");
            }
            break;

        case WM_KILLFOCUS:
            // Quando perde il focus, ripristina il testo
            if (!g_hasCapturedHotkey) {
                SetWindowTextW(hwnd, L"Clicca qui e premi la combinazione");
            }
            break;

        case WM_LBUTTONDOWN:
            // Cattura il focus quando cliccato
            SetFocus(hwnd);
            return 0;
    }

    return CallWindowProc(g_originalStaticProc, hwnd, msg, wParam, lParam);
}

// ============================================================================
// Dialog procedure per la configurazione hotkey
// ============================================================================

static INT_PTR CALLBACK HotkeyDialogProc(HWND hDlg, UINT message,
                                          WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            // Imposta il dialog sempre in primo piano
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE);

            // Centra il dialog sullo schermo
            RECT rcDesktop, rcDlg;
            GetWindowRect(GetDesktopWindow(), &rcDesktop);
            GetWindowRect(hDlg, &rcDlg);

            int x = (rcDesktop.right - rcDesktop.left - (rcDlg.right - rcDlg.left)) / 2;
            int y = (rcDesktop.bottom - rcDesktop.top - (rcDlg.bottom - rcDlg.top)) / 2;

            SetWindowPos(hDlg, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);

            // Porta in primo piano
            SetForegroundWindow(hDlg);

            // Inizializza la configurazione catturata con quella corrente
            g_capturedConfig = g_dialogConfig;
            g_hasCapturedHotkey = false;

            // Subclass del controllo di cattura
            HWND hCapture = GetDlgItem(hDlg, IDC_HOTKEY_CAPTURE);
            g_originalStaticProc = (WNDPROC)SetWindowLongPtr(hCapture, GWLP_WNDPROC,
                                                              (LONG_PTR)HotkeyCaptureProc);

            // Aggiorna etichetta combinazione attuale
            updateCurrentLabel(hDlg);

            // Mostra messaggio di errore se presente
            if (!g_dialogError.empty()) {
                SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, g_dialogError.c_str());
            }

            // Imposta il focus sul controllo di cattura
            SetFocus(hCapture);

            return FALSE;  // Non impostare il focus di default
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_BTN_RETRY:
                case IDOK: {
                    // Verifica che sia stata catturata una hotkey
                    if (!g_hasCapturedHotkey) {
                        SetDlgItemTextW(hDlg, IDC_STATIC_ERROR,
                                        L"Premi una combinazione di tasti prima di confermare.");
                        return TRUE;
                    }

                    // Usa la configurazione catturata
                    g_dialogConfig = g_capturedConfig;
                    g_dialogAccepted = true;

                    // Ripristina la procedura originale
                    HWND hCapture = GetDlgItem(hDlg, IDC_HOTKEY_CAPTURE);
                    SetWindowLongPtr(hCapture, GWLP_WNDPROC, (LONG_PTR)g_originalStaticProc);

                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_BTN_CANCEL:
                case IDCANCEL:
                    g_dialogAccepted = false;

                    // Ripristina la procedura originale
                    HWND hCapture = GetDlgItem(hDlg, IDC_HOTKEY_CAPTURE);
                    SetWindowLongPtr(hCapture, GWLP_WNDPROC, (LONG_PTR)g_originalStaticProc);

                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
        }

        case WM_CLOSE: {
            g_dialogAccepted = false;

            // Ripristina la procedura originale
            HWND hCapture = GetDlgItem(hDlg, IDC_HOTKEY_CAPTURE);
            SetWindowLongPtr(hCapture, GWLP_WNDPROC, (LONG_PTR)g_originalStaticProc);

            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }

    return FALSE;
}

HotkeyDialogResult showHotkeyConfigDialog(
    HWND hwndParent,
    const hotkeymanager::HotkeyConfig& currentConfig,
    const std::wstring& errorMessage)
{
    g_dialogConfig = currentConfig;
    g_dialogError = errorMessage;
    g_dialogAccepted = false;
    g_hasCapturedHotkey = false;

    DialogBoxW(GetModuleHandle(NULL),
               MAKEINTRESOURCEW(IDD_HOTKEY_CONFIG),
               hwndParent,
               HotkeyDialogProc);

    HotkeyDialogResult result;
    result.accepted = g_dialogAccepted;
    result.config = g_dialogConfig;

    return result;
}

// ============================================================================
// Timed MessageBox (sempre in primo piano)
// ============================================================================

static HWND g_timedMsgBoxHwnd = NULL;
static UINT_PTR g_timedMsgBoxTimerId = 0;

static VOID CALLBACK TimedMsgBoxCallback(HWND hwnd, UINT uMsg,
                                          UINT_PTR idEvent, DWORD dwTime) {
    (void)hwnd; (void)uMsg; (void)dwTime;
    if (g_timedMsgBoxHwnd != NULL) {
        KillTimer(NULL, g_timedMsgBoxTimerId);
        EndDialog(g_timedMsgBoxHwnd, 0);
        g_timedMsgBoxHwnd = NULL;
    }
}

static LRESULT CALLBACK TimedMsgBoxHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HCBT_ACTIVATE) {
        g_timedMsgBoxHwnd = (HWND)wParam;
        // Imposta il MessageBox sempre in primo piano
        SetWindowPos(g_timedMsgBoxHwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int showTimedMessageBox(HWND hwndParent,
                        const std::wstring& title,
                        const std::wstring& message,
                        UINT timeoutMs,
                        UINT icon) {
    (void)hwndParent;

    // Installa un hook per catturare l'handle del MessageBox
    HHOOK hHook = SetWindowsHookEx(WH_CBT, TimedMsgBoxHook,
                                    NULL, GetCurrentThreadId());

    // Imposta il timer
    g_timedMsgBoxTimerId = SetTimer(NULL, 0, timeoutMs, TimedMsgBoxCallback);

    // Mostra il MessageBox (MB_TOPMOST non esiste, usiamo l'hook)
    // MB_SETFOREGROUND porta la finestra in primo piano
    int result = MessageBoxW(NULL, message.c_str(), title.c_str(),
                             MB_OK | icon | MB_SETFOREGROUND | MB_SYSTEMMODAL);

    // Pulisci
    KillTimer(NULL, g_timedMsgBoxTimerId);
    UnhookWindowsHookEx(hHook);
    g_timedMsgBoxHwnd = NULL;

    return result;
}

// ============================================================================
// Altri dialogs (sempre in primo piano)
// ============================================================================

void showAboutDialog(HWND hwndParent) {
    (void)hwndParent;
    std::wstring message =
        L"MilleWin CF Extractor\n"
        L"Versione " APP_VERSION L"\n\n"
        L"Estrae il codice fiscale dalla finestra di MilleWin\n"
        L"e lo copia negli appunti.";

    MessageBoxW(NULL, message.c_str(), L"Informazioni",
                MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_SYSTEMMODAL);
}

void showErrorMessage(HWND hwndParent, const std::wstring& title,
                      const std::wstring& message) {
    (void)hwndParent;
    MessageBoxW(NULL, message.c_str(), title.c_str(),
                MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_SYSTEMMODAL);
}

void showNoPatientMessage(HWND hwndParent) {
    showTimedMessageBox(
        hwndParent,
        L"MWCF-Extractor - Avviso",
        L"Apri la cartella di un paziente per poterne estrarre il codice fiscale",
        MSGBOX_TIMEOUT_MS,
        MB_ICONINFORMATION
    );
}

void showMilleWinNotFoundMessage(HWND hwndParent) {
    showTimedMessageBox(
        hwndParent,
        L"MilleWin non trovato",
        L"Avvia MilleWin per poter estrarre il codice fiscale",
        MSGBOX_TIMEOUT_MS,
        MB_ICONWARNING
    );
}

} // namespace dialogs
