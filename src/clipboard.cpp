#include "clipboard.h"

namespace clipboard {

bool copyToClipboard(HWND hwnd, const std::wstring& text) {
    if (text.empty()) {
        return false;
    }

    // Apri la clipboard
    if (!OpenClipboard(hwnd)) {
        return false;
    }

    // Svuota la clipboard
    if (!EmptyClipboard()) {
        CloseClipboard();
        return false;
    }

    // Calcola la dimensione necessaria (incluso il terminatore null)
    size_t size = (text.length() + 1) * sizeof(wchar_t);

    // Alloca memoria globale per il testo
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (hGlobal == NULL) {
        CloseClipboard();
        return false;
    }

    // Blocca la memoria e copia il testo
    wchar_t* pGlobal = static_cast<wchar_t*>(GlobalLock(hGlobal));
    if (pGlobal == NULL) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    wcscpy_s(pGlobal, text.length() + 1, text.c_str());
    GlobalUnlock(hGlobal);

    // Imposta i dati nella clipboard (formato Unicode)
    if (SetClipboardData(CF_UNICODETEXT, hGlobal) == NULL) {
        GlobalFree(hGlobal);
        CloseClipboard();
        return false;
    }

    // Chiudi la clipboard (non liberare hGlobal, ora appartiene alla clipboard)
    CloseClipboard();

    return true;
}

std::wstring getFromClipboard(HWND hwnd) {
    std::wstring result;

    // Verifica se il formato Unicode è disponibile
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        return result;
    }

    // Apri la clipboard
    if (!OpenClipboard(hwnd)) {
        return result;
    }

    // Ottieni i dati
    HGLOBAL hGlobal = GetClipboardData(CF_UNICODETEXT);
    if (hGlobal != NULL) {
        const wchar_t* pGlobal = static_cast<const wchar_t*>(GlobalLock(hGlobal));
        if (pGlobal != NULL) {
            result = pGlobal;
            GlobalUnlock(hGlobal);
        }
    }

    CloseClipboard();

    return result;
}

bool clearClipboard(HWND hwnd) {
    if (!OpenClipboard(hwnd)) {
        return false;
    }

    BOOL success = EmptyClipboard();
    CloseClipboard();

    return success != FALSE;
}

} // namespace clipboard
