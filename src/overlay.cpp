#include "overlay.h"
#include "resource.h"
#include <shellscalingapi.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shcore.lib")

namespace overlay {

// ============================================================================
// Costanti
// ============================================================================

static const wchar_t* OVERLAY_CLASS_NAME = L"MWCFOverlayClass";
static const int OVERLAY_WIDTH = 320;
static const int OVERLAY_HEIGHT = 80;
static const int OVERLAY_MARGIN = 20;
static const int OVERLAY_PADDING = 12;
static const int OVERLAY_CORNER_RADIUS = 10;
static const BYTE OVERLAY_ALPHA = 240;

// Colori
static const COLORREF COLOR_SUCCESS_BG = RGB(34, 139, 34);      // Verde scuro
static const COLORREF COLOR_SUCCESS_BORDER = RGB(50, 205, 50);  // Verde chiaro
static const COLORREF COLOR_WARNING_BG = RGB(218, 165, 32);     // Giallo scuro
static const COLORREF COLOR_WARNING_BORDER = RGB(255, 215, 0);  // Giallo
static const COLORREF COLOR_ERROR_BG = RGB(178, 34, 34);        // Rosso scuro
static const COLORREF COLOR_ERROR_BORDER = RGB(220, 20, 60);    // Rosso
static const COLORREF COLOR_TEXT = RGB(255, 255, 255);          // Bianco

// ============================================================================
// Variabili globali
// ============================================================================

static HINSTANCE g_hInstance = NULL;
static HWND g_hwndOverlay = NULL;
static UINT_PTR g_timerId = 0;
static std::wstring g_title;
static std::wstring g_message;
static OverlayType g_type = OverlayType::Success;
static HFONT g_fontTitle = NULL;
static HFONT g_fontMessage = NULL;
static bool g_initialized = false;

// ============================================================================
// Funzioni DPI-aware
// ============================================================================

static int getDpiForMonitor(HMONITOR hMonitor) {
    UINT dpiX = 96, dpiY = 96;

    // Windows 8.1+
    typedef HRESULT (WINAPI *GetDpiForMonitorFunc)(HMONITOR, int, UINT*, UINT*);
    static GetDpiForMonitorFunc pGetDpiForMonitor = NULL;
    static bool initialized = false;

    if (!initialized) {
        HMODULE hShcore = LoadLibraryW(L"shcore.dll");
        if (hShcore) {
            pGetDpiForMonitor = (GetDpiForMonitorFunc)GetProcAddress(hShcore, "GetDpiForMonitor");
        }
        initialized = true;
    }

    if (pGetDpiForMonitor) {
        pGetDpiForMonitor(hMonitor, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY);
    }

    return (int)dpiX;
}

static int scaleForDpi(int value, int dpi) {
    return MulDiv(value, dpi, 96);
}

// ============================================================================
// Trova il monitor con il cursore
// ============================================================================

static HMONITOR getMonitorFromCursor() {
    POINT pt;
    GetCursorPos(&pt);
    return MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
}

static RECT getWorkAreaForMonitor(HMONITOR hMonitor) {
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &mi);
    return mi.rcWork;
}

// ============================================================================
// Creazione font
// ============================================================================

static void createFonts(int dpi) {
    if (g_fontTitle) DeleteObject(g_fontTitle);
    if (g_fontMessage) DeleteObject(g_fontMessage);

    int titleSize = scaleForDpi(14, dpi);
    int messageSize = scaleForDpi(18, dpi);

    g_fontTitle = CreateFontW(
        titleSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );

    g_fontMessage = CreateFontW(
        messageSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );
}

// ============================================================================
// Window Procedure per l'overlay
// ============================================================================

static void getColorsForType(OverlayType type, COLORREF& bg, COLORREF& border) {
    switch (type) {
        case OverlayType::Success:
            bg = COLOR_SUCCESS_BG;
            border = COLOR_SUCCESS_BORDER;
            break;
        case OverlayType::Warning:
            bg = COLOR_WARNING_BG;
            border = COLOR_WARNING_BORDER;
            break;
        case OverlayType::Error:
            bg = COLOR_ERROR_BG;
            border = COLOR_ERROR_BORDER;
            break;
    }
}

static void paintOverlay(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    // Ottieni colori
    COLORREF bgColor, borderColor;
    getColorsForType(g_type, bgColor, borderColor);

    // Crea DC compatibile per double buffering
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Sfondo con angoli arrotondati
    HBRUSH hBrushBg = CreateSolidBrush(bgColor);
    HBRUSH hBrushBorder = CreateSolidBrush(borderColor);
    HPEN hPenBorder = CreatePen(PS_SOLID, 2, borderColor);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, hBrushBg);
    HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPenBorder);

    RoundRect(hdcMem, rc.left, rc.top, rc.right, rc.bottom,
              OVERLAY_CORNER_RADIUS * 2, OVERLAY_CORNER_RADIUS * 2);

    // Testo
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, COLOR_TEXT);

    RECT rcText = rc;
    rcText.left += OVERLAY_PADDING;
    rcText.right -= OVERLAY_PADDING;
    rcText.top += OVERLAY_PADDING;
    rcText.bottom -= OVERLAY_PADDING;

    // Titolo
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, g_fontTitle);
    DrawTextW(hdcMem, g_title.c_str(), -1, &rcText,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Messaggio (più grande)
    SelectObject(hdcMem, g_fontMessage);
    rcText.top += scaleForDpi(22, getDpiForMonitor(getMonitorFromCursor()));
    DrawTextW(hdcMem, g_message.c_str(), -1, &rcText,
              DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

    // Copia sul DC principale
    BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcMem, hOldFont);
    SelectObject(hdcMem, hOldBrush);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hBrushBg);
    DeleteObject(hBrushBorder);
    DeleteObject(hPenBorder);
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            paintOverlay(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_TIMER:
            if (wParam == g_timerId) {
                hide();
            }
            return 0;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // Click chiude l'overlay
            hide();
            return 0;

        case WM_ERASEBKGND:
            return 1;  // Non cancellare lo sfondo

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

// ============================================================================
// API pubblica
// ============================================================================

bool initialize(HINSTANCE hInstance) {
    if (g_initialized) return true;

    g_hInstance = hInstance;

    // Registra la classe della finestra
    WNDCLASSEXW wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayWndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszClassName = OVERLAY_CLASS_NAME;

    if (!RegisterClassExW(&wcex)) {
        return false;
    }

    g_initialized = true;
    return true;
}

void cleanup() {
    hide();

    if (g_fontTitle) {
        DeleteObject(g_fontTitle);
        g_fontTitle = NULL;
    }
    if (g_fontMessage) {
        DeleteObject(g_fontMessage);
        g_fontMessage = NULL;
    }

    if (g_initialized) {
        UnregisterClassW(OVERLAY_CLASS_NAME, g_hInstance);
        g_initialized = false;
    }
}

void show(const std::wstring& title,
          const std::wstring& message,
          OverlayType type,
          UINT durationMs) {

    if (!g_initialized) return;

    // Nascondi overlay precedente
    hide();

    // Salva i dati
    g_title = title;
    g_message = message;
    g_type = type;

    // Trova il monitor corrente
    HMONITOR hMonitor = getMonitorFromCursor();
    int dpi = getDpiForMonitor(hMonitor);
    RECT workArea = getWorkAreaForMonitor(hMonitor);

    // Crea i font per il DPI corrente
    createFonts(dpi);

    // Calcola dimensioni e posizione (in basso a destra)
    int width = scaleForDpi(OVERLAY_WIDTH, dpi);
    int height = scaleForDpi(OVERLAY_HEIGHT, dpi);
    int margin = scaleForDpi(OVERLAY_MARGIN, dpi);

    int x = workArea.right - width - margin;
    int y = workArea.bottom - height - margin;

    // Crea la finestra overlay
    g_hwndOverlay = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        OVERLAY_CLASS_NAME,
        L"",
        WS_POPUP,
        x, y, width, height,
        NULL, NULL, g_hInstance, NULL
    );

    if (!g_hwndOverlay) return;

    // Imposta la trasparenza
    SetLayeredWindowAttributes(g_hwndOverlay, 0, OVERLAY_ALPHA, LWA_ALPHA);

    // Mostra la finestra senza attivare
    ShowWindow(g_hwndOverlay, SW_SHOWNOACTIVATE);
    UpdateWindow(g_hwndOverlay);

    // Imposta il timer per nascondere
    g_timerId = SetTimer(g_hwndOverlay, 1, durationMs, NULL);
}

void hide() {
    if (g_hwndOverlay) {
        if (g_timerId) {
            KillTimer(g_hwndOverlay, g_timerId);
            g_timerId = 0;
        }
        DestroyWindow(g_hwndOverlay);
        g_hwndOverlay = NULL;
    }
}

bool isVisible() {
    return g_hwndOverlay != NULL && IsWindowVisible(g_hwndOverlay);
}

} // namespace overlay
