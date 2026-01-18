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
static std::wstring g_dialogError;
static bool g_dialogAccepted = false;

// ============================================================================
// Utility functions
// ============================================================================

static void populateModifierCombo(HWND hCombo, hotkeymanager::Modifier selected) {
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    int selIdx = 0;

    // CTRL
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"CTRL");
    SendMessage(hCombo, CB_SETITEMDATA, 0, (LPARAM)hotkeymanager::Modifier::CTRL);
    if (selected == hotkeymanager::Modifier::CTRL) selIdx = 0;

    // ALT
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"ALT");
    SendMessage(hCombo, CB_SETITEMDATA, 1, (LPARAM)hotkeymanager::Modifier::ALT);
    if (selected == hotkeymanager::Modifier::ALT) selIdx = 1;

    // SHIFT
    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"SHIFT");
    SendMessage(hCombo, CB_SETITEMDATA, 2, (LPARAM)hotkeymanager::Modifier::SHIFT);
    if (selected == hotkeymanager::Modifier::SHIFT) selIdx = 2;

    SendMessage(hCombo, CB_SETCURSEL, selIdx, 0);
}

static void populateKeyCombo(HWND hCombo, UINT selectedVK) {
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    int selIdx = 0;

    // Aggiungi i tasti del numpad 0-9
    for (int i = 0; i <= 9; i++) {
        wchar_t label[32];
        swprintf_s(label, L"%d (Num)", i);
        int idx = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)label);
        UINT vk = hotkeymanager::HotkeyManager::getNumpadVK(i);
        SendMessage(hCombo, CB_SETITEMDATA, idx, (LPARAM)vk);

        if (vk == selectedVK) {
            selIdx = idx;
        }
    }

    SendMessage(hCombo, CB_SETCURSEL, selIdx, 0);
}

static void updateCurrentLabel(HWND hDlg) {
    HWND hComboMod = GetDlgItem(hDlg, IDC_COMBO_MODIFIER);
    HWND hComboKey = GetDlgItem(hDlg, IDC_COMBO_KEY);

    int modIdx = (int)SendMessage(hComboMod, CB_GETCURSEL, 0, 0);
    int keyIdx = (int)SendMessage(hComboKey, CB_GETCURSEL, 0, 0);

    if (modIdx >= 0 && keyIdx >= 0) {
        hotkeymanager::Modifier mod =
            (hotkeymanager::Modifier)SendMessage(hComboMod, CB_GETITEMDATA, modIdx, 0);
        UINT vk = (UINT)SendMessage(hComboKey, CB_GETITEMDATA, keyIdx, 0);

        std::wstring label = hotkeymanager::HotkeyManager::getModifierName(mod) +
                             L" + " +
                             hotkeymanager::HotkeyManager::getKeyName(vk);

        SetDlgItemTextW(hDlg, IDC_STATIC_CURRENT, label.c_str());
    }
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

            // Popola le combo
            HWND hComboMod = GetDlgItem(hDlg, IDC_COMBO_MODIFIER);
            HWND hComboKey = GetDlgItem(hDlg, IDC_COMBO_KEY);

            populateModifierCombo(hComboMod, g_dialogConfig.modifier);
            populateKeyCombo(hComboKey, g_dialogConfig.vkCode);

            // Aggiorna etichetta combinazione attuale
            updateCurrentLabel(hDlg);

            // Mostra messaggio di errore se presente
            if (!g_dialogError.empty()) {
                SetDlgItemTextW(hDlg, IDC_STATIC_ERROR, g_dialogError.c_str());
            }

            return TRUE;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDC_COMBO_MODIFIER:
                case IDC_COMBO_KEY:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        updateCurrentLabel(hDlg);
                    }
                    break;

                case IDC_BTN_RETRY:
                case IDOK: {
                    // Leggi la configurazione selezionata
                    HWND hComboMod = GetDlgItem(hDlg, IDC_COMBO_MODIFIER);
                    HWND hComboKey = GetDlgItem(hDlg, IDC_COMBO_KEY);

                    int modIdx = (int)SendMessage(hComboMod, CB_GETCURSEL, 0, 0);
                    int keyIdx = (int)SendMessage(hComboKey, CB_GETCURSEL, 0, 0);

                    g_dialogConfig.modifier =
                        (hotkeymanager::Modifier)SendMessage(hComboMod, CB_GETITEMDATA, modIdx, 0);
                    g_dialogConfig.vkCode =
                        (UINT)SendMessage(hComboKey, CB_GETITEMDATA, keyIdx, 0);

                    g_dialogAccepted = true;
                    EndDialog(hDlg, IDOK);
                    return TRUE;
                }

                case IDC_BTN_CANCEL:
                case IDCANCEL:
                    g_dialogAccepted = false;
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
        }

        case WM_CLOSE:
            g_dialogAccepted = false;
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
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
        L"Versione 1.0.0\n\n"
        L"Estrae il codice fiscale dalla finestra di MilleWin\n"
        L"e lo copia negli appunti.\n\n"
        L"Applicazione portabile - nessuna installazione richiesta.";

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
        L"Nessun paziente",
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
