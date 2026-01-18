/**
 * @file generate_icon.cpp
 * @brief Tool per generare il file app.ico per l'applicazione
 *
 * Compilare ed eseguire prima della build principale:
 *   cl /EHsc generate_icon.cpp /Fe:generate_icon.exe
 *   generate_icon.exe
 */

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <fstream>
#include <vector>
#include <cstdint>

// Colori
static const COLORREF COLOR_BG = RGB(46, 125, 50);       // Verde scuro
static const COLORREF COLOR_BORDER = RGB(76, 175, 80);   // Verde chiaro
static const COLORREF COLOR_TEXT = RGB(255, 255, 255);   // Bianco

#pragma pack(push, 1)

struct ICONDIR {
    uint16_t reserved;   // Must be 0
    uint16_t type;       // 1 for icon
    uint16_t count;      // Number of images
};

struct ICONDIRENTRY {
    uint8_t width;       // Width (0 means 256)
    uint8_t height;      // Height (0 means 256)
    uint8_t colorCount;  // 0 for 32-bit
    uint8_t reserved;    // 0
    uint16_t planes;     // 1
    uint16_t bitCount;   // 32
    uint32_t bytesInRes; // Size of image data
    uint32_t imageOffset;// Offset to image data
};

struct BITMAPINFOHEADER_ICO {
    uint32_t size;
    int32_t width;
    int32_t height;      // Height * 2 (includes mask)
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    int32_t xPelsPerMeter;
    int32_t yPelsPerMeter;
    uint32_t clrUsed;
    uint32_t clrImportant;
};

#pragma pack(pop)

// Genera i pixel per un'icona con "CF"
std::vector<uint32_t> generateIconPixels(int size) {
    std::vector<uint32_t> pixels(size * size, 0);

    // Crea un DC e bitmap per disegnare
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size;  // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HBITMAP hbm = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);

    // Sfondo trasparente
    RECT rc = {0, 0, size, size};
    HBRUSH hBrushBg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdcMem, &rc, hBrushBg);
    DeleteObject(hBrushBg);

    // Rettangolo arrotondato
    int margin = size / 8;
    int radius = size / 4;

    HBRUSH hBrush = CreateSolidBrush(COLOR_BG);
    HPEN hPen = CreatePen(PS_SOLID, (size > 20 ? 2 : 1), COLOR_BORDER);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdcMem, hPen);

    RoundRect(hdcMem, margin, margin, size - margin, size - margin, radius * 2, radius * 2);

    SelectObject(hdcMem, hOldBrush);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);

    // Testo "CF"
    int fontSize = size * 5 / 10;
    HFONT hFont = CreateFontW(
        fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI"
    );

    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, COLOR_TEXT);

    RECT rcText = {0, 0, size, size};
    DrawTextW(hdcMem, L"CF", -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);

    // Copia i pixel
    GdiFlush();
    memcpy(pixels.data(), pvBits, size * size * 4);

    // Applica alpha
    for (int i = 0; i < size * size; i++) {
        uint32_t pixel = pixels[i];
        uint8_t r = (pixel >> 16) & 0xFF;
        uint8_t g = (pixel >> 8) & 0xFF;
        uint8_t b = pixel & 0xFF;

        if (r == 0 && g == 0 && b == 0) {
            pixels[i] = 0x00000000;  // Trasparente
        } else {
            pixels[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }

    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbm);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Inverti verticalmente per formato ICO (bottom-up)
    std::vector<uint32_t> flipped(size * size);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            flipped[(size - 1 - y) * size + x] = pixels[y * size + x];
        }
    }

    return flipped;
}

// Genera la maschera AND (1-bit)
std::vector<uint8_t> generateMask(const std::vector<uint32_t>& pixels, int size) {
    int rowBytes = ((size + 31) / 32) * 4;  // Allineamento a 4 byte
    std::vector<uint8_t> mask(rowBytes * size, 0);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            uint32_t pixel = pixels[y * size + x];
            uint8_t alpha = (pixel >> 24) & 0xFF;

            if (alpha < 128) {
                // Pixel trasparente -> bit = 1
                int byteIdx = y * rowBytes + x / 8;
                int bitIdx = 7 - (x % 8);
                mask[byteIdx] |= (1 << bitIdx);
            }
        }
    }

    return mask;
}

bool writeIcon(const char* filename) {
    // Dimensioni delle icone da includere
    const int sizes[] = {16, 32, 48, 256};
    const int numSizes = sizeof(sizes) / sizeof(sizes[0]);

    std::vector<std::vector<uint32_t>> allPixels;
    std::vector<std::vector<uint8_t>> allMasks;

    for (int i = 0; i < numSizes; i++) {
        auto pixels = generateIconPixels(sizes[i]);
        auto mask = generateMask(pixels, sizes[i]);
        allPixels.push_back(pixels);
        allMasks.push_back(mask);
    }

    // Calcola offset e dimensioni
    uint32_t currentOffset = sizeof(ICONDIR) + numSizes * sizeof(ICONDIRENTRY);

    std::vector<ICONDIRENTRY> entries(numSizes);
    std::vector<std::vector<uint8_t>> imageData(numSizes);

    for (int i = 0; i < numSizes; i++) {
        int size = sizes[i];
        int maskRowBytes = ((size + 31) / 32) * 4;

        // Crea i dati dell'immagine
        BITMAPINFOHEADER_ICO bih = {0};
        bih.size = sizeof(BITMAPINFOHEADER_ICO);
        bih.width = size;
        bih.height = size * 2;  // XOR + AND mask
        bih.planes = 1;
        bih.bitCount = 32;
        bih.compression = 0;
        bih.sizeImage = size * size * 4 + maskRowBytes * size;

        std::vector<uint8_t> data;
        data.resize(sizeof(BITMAPINFOHEADER_ICO) + bih.sizeImage);

        memcpy(data.data(), &bih, sizeof(bih));
        memcpy(data.data() + sizeof(bih), allPixels[i].data(), size * size * 4);
        memcpy(data.data() + sizeof(bih) + size * size * 4, allMasks[i].data(), maskRowBytes * size);

        imageData[i] = data;

        // Entry
        entries[i].width = (size >= 256) ? 0 : (uint8_t)size;
        entries[i].height = (size >= 256) ? 0 : (uint8_t)size;
        entries[i].colorCount = 0;
        entries[i].reserved = 0;
        entries[i].planes = 1;
        entries[i].bitCount = 32;
        entries[i].bytesInRes = (uint32_t)data.size();
        entries[i].imageOffset = currentOffset;

        currentOffset += (uint32_t)data.size();
    }

    // Scrivi il file
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    ICONDIR header = {0, 1, (uint16_t)numSizes};
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (int i = 0; i < numSizes; i++) {
        file.write(reinterpret_cast<const char*>(&entries[i]), sizeof(ICONDIRENTRY));
    }

    for (int i = 0; i < numSizes; i++) {
        file.write(reinterpret_cast<const char*>(imageData[i].data()), imageData[i].size());
    }

    file.close();
    return true;
}

int main() {
    if (writeIcon("../res/app.ico")) {
        printf("Icon generated successfully: res/app.ico\n");
        return 0;
    } else {
        printf("Failed to generate icon\n");
        return 1;
    }
}
