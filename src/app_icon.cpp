#include "app_icon.h"
#include <cmath>

namespace appicon {

// Colori
static const COLORREF COLOR_ACTIVE_BG = RGB(46, 125, 50);       // Verde scuro
static const COLORREF COLOR_ACTIVE_BORDER = RGB(76, 175, 80);   // Verde
static const COLORREF COLOR_INACTIVE_BG = RGB(97, 97, 97);      // Grigio
static const COLORREF COLOR_INACTIVE_BORDER = RGB(158, 158, 158);
static const COLORREF COLOR_TEXT = RGB(255, 255, 255);          // Bianco

/**
 * @brief Disegna un rettangolo con angoli arrotondati.
 */
static void drawRoundRect(HDC hdc, int x, int y, int width, int height,
                          int radius, COLORREF fillColor, COLORREF borderColor) {
    HBRUSH hBrush = CreateSolidBrush(fillColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    RoundRect(hdc, x, y, x + width, y + height, radius * 2, radius * 2);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

/**
 * @brief Crea un'icona con testo "CF".
 */
static HICON createIconWithText(int size, COLORREF bgColor, COLORREF borderColor) {
    // Crea DC e bitmap
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Bitmap per il colore
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HBITMAP hbmColor = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

    // Sfondo trasparente
    HBRUSH hBrushBg = CreateSolidBrush(RGB(0, 0, 0));
    RECT rc = {0, 0, size, size};
    FillRect(hdcMem, &rc, hBrushBg);
    DeleteObject(hBrushBg);

    // Rettangolo arrotondato
    int margin = size / 8;
    int radius = size / 4;
    drawRoundRect(hdcMem, margin, margin, size - margin * 2, size - margin * 2,
                  radius, bgColor, borderColor);

    // Testo "CF"
    int fontSize = size / 2;
    HFONT hFont = CreateFontW(
        fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );

    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, COLOR_TEXT);

    RECT rcText = {0, 0, size, size};
    DrawTextW(hdcMem, L"CF", -1, &rcText,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);

    // Applica trasparenza alpha ai pixel
    DWORD* pixels = (DWORD*)pvBits;
    COLORREF bgTransparent = RGB(0, 0, 0);
    for (int i = 0; i < size * size; i++) {
        DWORD pixel = pixels[i];
        BYTE r = (pixel >> 16) & 0xFF;
        BYTE g = (pixel >> 8) & 0xFF;
        BYTE b = pixel & 0xFF;

        // Se il pixel è nero (sfondo), rendi trasparente
        if (r == 0 && g == 0 && b == 0) {
            pixels[i] = 0x00000000;  // Trasparente
        } else {
            // Aggiungi alpha pieno
            pixels[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }

    SelectObject(hdcMem, hbmOld);

    // Crea bitmap maschera
    HBITMAP hbmMask = CreateBitmap(size, size, 1, 1, NULL);
    HDC hdcMask = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMaskOld = (HBITMAP)SelectObject(hdcMask, hbmMask);

    // Riempi la maschera (nero = visibile, bianco = trasparente)
    HBRUSH hBrushWhite = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH hBrushBlack = CreateSolidBrush(RGB(0, 0, 0));

    FillRect(hdcMask, &rc, hBrushWhite);  // Tutto trasparente

    // Disegna la forma visibile in nero
    HBRUSH hOldMaskBrush = (HBRUSH)SelectObject(hdcMask, hBrushBlack);
    HPEN hPenBlack = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hOldMaskPen = (HPEN)SelectObject(hdcMask, hPenBlack);

    RoundRect(hdcMask, margin, margin, size - margin, size - margin,
              radius * 2, radius * 2);

    SelectObject(hdcMask, hOldMaskBrush);
    SelectObject(hdcMask, hOldMaskPen);
    DeleteObject(hPenBlack);
    DeleteObject(hBrushWhite);
    DeleteObject(hBrushBlack);

    SelectObject(hdcMask, hbmMaskOld);
    DeleteDC(hdcMask);

    // Crea l'icona
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;

    HICON hIcon = CreateIconIndirect(&ii);

    // Cleanup
    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return hIcon;
}

HICON createTrayIcon(int size, bool active) {
    if (active) {
        return createIconWithText(size, COLOR_ACTIVE_BG, COLOR_ACTIVE_BORDER);
    } else {
        return createIconWithText(size, COLOR_INACTIVE_BG, COLOR_INACTIVE_BORDER);
    }
}

HICON createAppIcon(int size) {
    return createIconWithText(size, COLOR_ACTIVE_BG, COLOR_ACTIVE_BORDER);
}

} // namespace appicon
