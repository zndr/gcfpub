#include "cf_parser.h"
#include <regex>
#include <algorithm>
#include <cctype>

namespace cfparser {

// Regex completa per codice fiscale italiano (supporta omocodia)
// Pattern fornito che gestisce tutte le varianti valide del CF
static const std::wregex CF_REGEX(
    L"(?:(?:[B-DF-HJ-NP-TV-Z]|[AEIOU])[AEIOU][AEIOUX]|[B-DF-HJ-NP-TV-Z]{2}[A-Z]){2}"
    L"[\\dLMNP-V]{2}"
    L"(?:[A-EHLMPR-T](?:[04LQ][1-9MNP-V]|[1256LMRS][\\dLMNP-V])|[DHPS][37PT][0L]|[ACELMRT][37PT][01LM])"
    L"(?:[A-MZ][1-9MNP-V][\\dLMNP-V]{2}|[A-M][0L](?:[1-9MNP-V][\\dLMNP-V]|[0L][1-9MNP-V]))"
    L"[A-Z]",
    std::regex_constants::icase
);

// Tabella conversione caratteri omocodici -> cifre
static const wchar_t OMOCODIA_CHARS[] = L"LMNPQRSTUV";
static const wchar_t OMOCODIA_DIGITS[] = L"0123456789";

// Tabella valori per calcolo CIN - caratteri dispari (posizioni 1,3,5,7,9,11,13,15)
static const int CIN_ODD_VALUES[] = {
    1,  0,  5, 7, 9, 13, 15, 17, 19, 21,  // 0-9
    1,  0,  5, 7, 9, 13, 15, 17, 19, 21,  // A-J (A=0, B=1, ...)
    2,  4, 18, 20, 11,  3,  6,  8, 12, 14, // K-T
    16, 10, 22, 25, 24, 23                 // U-Z
};

// Tabella valori per calcolo CIN - caratteri pari (posizioni 2,4,6,8,10,12,14)
static const int CIN_EVEN_VALUES[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  // 0-9
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  // A-J
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, // K-T
    20, 21, 22, 23, 24, 25                  // U-Z
};

/**
 * @brief Converte un carattere in indice per le tabelle CIN
 */
static int charToIndex(wchar_t c) {
    c = towupper(c);
    if (c >= L'0' && c <= L'9') {
        return c - L'0';
    } else if (c >= L'A' && c <= L'Z') {
        return c - L'A' + 10;
    }
    return 0;
}

/**
 * @brief Converte un carattere omocodico in cifra
 */
static wchar_t omocodiaToDigit(wchar_t c) {
    c = towupper(c);
    for (int i = 0; i < 10; i++) {
        if (c == OMOCODIA_CHARS[i]) {
            return OMOCODIA_DIGITS[i];
        }
    }
    return c;
}

std::wstring normalizeOmocodia(const std::wstring& cf) {
    if (cf.length() != 16) {
        return cf;
    }

    std::wstring normalized = cf;

    // Converti in maiuscolo
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), towupper);

    // Posizioni che possono contenere caratteri omocodici (0-indexed): 6,7,9,10,12,13,14
    const int omocodiaPositions[] = {6, 7, 9, 10, 12, 13, 14};

    for (int pos : omocodiaPositions) {
        normalized[pos] = omocodiaToDigit(normalized[pos]);
    }

    return normalized;
}

wchar_t calculateCIN(const std::wstring& cf) {
    if (cf.length() < 15) {
        return L'?';
    }

    // Normalizza per gestire omocodia nel calcolo
    std::wstring normalized = cf.substr(0, 15);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), towupper);

    int sum = 0;

    for (int i = 0; i < 15; i++) {
        int idx = charToIndex(normalized[i]);

        // Posizioni dispari (1,3,5...) in base 1, quindi indici pari (0,2,4...) in base 0
        if (i % 2 == 0) {
            // Posizione dispari (1-indexed) -> usa tabella dispari
            sum += CIN_ODD_VALUES[idx];
        } else {
            // Posizione pari (1-indexed) -> usa tabella pari
            sum += CIN_EVEN_VALUES[idx];
        }
    }

    return L'A' + (sum % 26);
}

bool verifyCIN(const std::wstring& cf) {
    if (cf.length() != 16) {
        return false;
    }

    wchar_t expectedCIN = calculateCIN(cf);
    wchar_t actualCIN = towupper(cf[15]);

    return expectedCIN == actualCIN;
}

bool isValidCodiceFiscale(const std::wstring& cf) {
    if (cf.length() != 16) {
        return false;
    }

    // Verifica che corrisponda al pattern regex
    if (!std::regex_match(cf, CF_REGEX)) {
        return false;
    }

    // Verifica il carattere di controllo
    return verifyCIN(cf);
}

std::optional<std::wstring> extractCodiceFiscale(const std::wstring& text) {
    std::wsmatch match;

    // Cerca il pattern del codice fiscale nel testo
    if (std::regex_search(text, match, CF_REGEX)) {
        std::wstring cf = match.str();

        // Converti in maiuscolo
        std::transform(cf.begin(), cf.end(), cf.begin(), towupper);

        // Verifica validità (opzionale - la regex è già molto precisa)
        // Se vuoi essere più permissivo, puoi rimuovere questo controllo
        if (verifyCIN(cf)) {
            return cf;
        }

        // Se il CIN non è valido ma il pattern corrisponde,
        // potrebbe essere un errore di battitura nel CF originale.
        // Restituiamo comunque il CF trovato.
        return cf;
    }

    return std::nullopt;
}

} // namespace cfparser
