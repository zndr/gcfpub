#include "update_checker.h"
#include "resource.h"
#include <winhttp.h>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <shellapi.h>

#pragma comment(lib, "winhttp.lib")

namespace updatechecker {

// Risultato globale per il controllo asincrono
static UpdateCheckResult g_lastResult;
static std::mutex g_resultMutex;

// User-Agent per le richieste HTTP
static const wchar_t* USER_AGENT = L"MWCFExtractor/1.0";

// API endpoint per i tag
static const wchar_t* API_HOST = L"api.github.com";
static const wchar_t* API_PATH = L"/repos/zndr/gcfpub/tags";

std::wstring getCurrentVersion() {
    return APP_VERSION;
}

int compareVersions(const std::wstring& v1, const std::wstring& v2) {
    // Rimuovi il prefisso 'v' se presente
    std::wstring ver1 = v1;
    std::wstring ver2 = v2;

    if (!ver1.empty() && (ver1[0] == L'v' || ver1[0] == L'V')) {
        ver1 = ver1.substr(1);
    }
    if (!ver2.empty() && (ver2[0] == L'v' || ver2[0] == L'V')) {
        ver2 = ver2.substr(1);
    }

    // Parse version components (major.minor.patch)
    auto parseVersion = [](const std::wstring& v) -> std::vector<int> {
        std::vector<int> parts;
        std::wistringstream iss(v);
        std::wstring part;

        while (std::getline(iss, part, L'.')) {
            try {
                parts.push_back(std::stoi(part));
            } catch (...) {
                parts.push_back(0);
            }
        }

        // Assicurati di avere almeno 3 componenti
        while (parts.size() < 3) {
            parts.push_back(0);
        }

        return parts;
    };

    std::vector<int> parts1 = parseVersion(ver1);
    std::vector<int> parts2 = parseVersion(ver2);

    for (size_t i = 0; i < 3; ++i) {
        if (parts1[i] < parts2[i]) return -1;
        if (parts1[i] > parts2[i]) return 1;
    }

    return 0;
}

// Funzione helper per estrarre la versione dal JSON
static std::wstring extractLatestVersion(const std::string& json) {
    // Cerchiamo il primo "name": "vX.X.X" nel JSON
    // Il formato e': [{"name":"v1.0.0",...},...]

    std::string searchKey = "\"name\":\"";
    size_t pos = json.find(searchKey);

    if (pos == std::string::npos) {
        // Prova con spazio dopo i due punti
        searchKey = "\"name\": \"";
        pos = json.find(searchKey);
    }

    if (pos == std::string::npos) {
        return L"";
    }

    pos += searchKey.length();
    size_t endPos = json.find("\"", pos);

    if (endPos == std::string::npos) {
        return L"";
    }

    std::string version = json.substr(pos, endPos - pos);

    // Converti in wstring
    std::wstring wversion(version.begin(), version.end());

    return wversion;
}

UpdateCheckResult checkForUpdates() {
    UpdateCheckResult result;
    result.success = false;
    result.updateAvailable = false;
    result.currentVersion = getCurrentVersion();

    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    do {
        // Apri sessione WinHTTP
        hSession = WinHttpOpen(
            USER_AGENT,
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!hSession) {
            result.errorMessage = L"Impossibile inizializzare la connessione";
            break;
        }

        // Connetti all'host
        hConnect = WinHttpConnect(
            hSession,
            API_HOST,
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        );

        if (!hConnect) {
            result.errorMessage = L"Impossibile connettersi al server";
            break;
        }

        // Crea la richiesta
        hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            API_PATH,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        );

        if (!hRequest) {
            result.errorMessage = L"Impossibile creare la richiesta";
            break;
        }

        // Aggiungi header per GitHub API
        WinHttpAddRequestHeaders(
            hRequest,
            L"Accept: application/vnd.github.v3+json",
            (DWORD)-1,
            WINHTTP_ADDREQ_FLAG_ADD
        );

        // Invia la richiesta
        if (!WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        )) {
            result.errorMessage = L"Impossibile inviare la richiesta";
            break;
        }

        // Ricevi la risposta
        if (!WinHttpReceiveResponse(hRequest, NULL)) {
            result.errorMessage = L"Nessuna risposta dal server";
            break;
        }

        // Verifica lo status code
        DWORD statusCode = 0;
        DWORD statusCodeSize = sizeof(statusCode);
        WinHttpQueryHeaders(
            hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusCodeSize,
            WINHTTP_NO_HEADER_INDEX
        );

        if (statusCode != 200) {
            result.errorMessage = L"Errore dal server (HTTP " + std::to_wstring(statusCode) + L")";
            break;
        }

        // Leggi il body della risposta
        std::string responseBody;
        DWORD bytesRead = 0;
        DWORD bytesAvailable = 0;

        do {
            bytesAvailable = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
                break;
            }

            if (bytesAvailable == 0) {
                break;
            }

            std::vector<char> buffer(bytesAvailable + 1);
            if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                buffer[bytesRead] = '\0';
                responseBody.append(buffer.data(), bytesRead);
            }
        } while (bytesAvailable > 0);

        if (responseBody.empty()) {
            result.errorMessage = L"Risposta vuota dal server";
            break;
        }

        // Estrai la versione piu' recente
        std::wstring latestVersion = extractLatestVersion(responseBody);

        if (latestVersion.empty()) {
            result.errorMessage = L"Impossibile determinare la versione";
            break;
        }

        result.latestVersion = latestVersion;
        result.success = true;

        // Confronta le versioni
        int cmp = compareVersions(result.currentVersion, latestVersion);
        result.updateAvailable = (cmp < 0);

    } while (false);

    // Cleanup
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return result;
}

void checkForUpdatesAsync(HWND hwnd, UINT message) {
    std::thread([hwnd, message]() {
        UpdateCheckResult result = checkForUpdates();

        {
            std::lock_guard<std::mutex> lock(g_resultMutex);
            g_lastResult = result;
        }

        // Notifica la finestra principale
        PostMessage(hwnd, message, 0, 0);
    }).detach();
}

UpdateCheckResult getLastCheckResult() {
    std::lock_guard<std::mutex> lock(g_resultMutex);
    return g_lastResult;
}

void openReleasesPage() {
    ShellExecuteW(
        NULL,
        L"open",
        RELEASES_URL,
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
}

} // namespace updatechecker
