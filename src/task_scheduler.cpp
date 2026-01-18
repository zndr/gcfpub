#include "task_scheduler.h"
#include <comdef.h>
#include <taskschd.h>
#include <shlobj.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")

namespace taskscheduler {

static const wchar_t* AUTOSTART_KEY_NAME = L"MWCFExtractor";
static const wchar_t* TASK_FOLDER = L"\\";
static const wchar_t* TASK_AUTHOR = L"MWCFExtractor";
static const wchar_t* TASK_DESCRIPTION = L"Avvia automaticamente MilleWin CF Extractor al login";

static std::wstring getExePath() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

bool isTaskEnabled() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool needsUninit = SUCCEEDED(hr);

    // Se CoInitialize fallisce con RPC_E_CHANGED_MODE, significa che COM e' gia' inizializzato
    // in un altro modo, ma possiamo comunque usarlo
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    bool result = false;
    ITaskService* pService = nullptr;
    ITaskFolder* pFolder = nullptr;
    IRegisteredTask* pTask = nullptr;

    do {
        // Crea il servizio Task Scheduler
        hr = CoCreateInstance(
            CLSID_TaskScheduler,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITaskService,
            (void**)&pService
        );
        if (FAILED(hr)) break;

        // Connetti al servizio
        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) break;

        // Ottieni la cartella root
        hr = pService->GetFolder(_bstr_t(TASK_FOLDER), &pFolder);
        if (FAILED(hr)) break;

        // Prova a ottenere il task
        hr = pFolder->GetTask(_bstr_t(TASK_NAME), &pTask);
        if (FAILED(hr)) break;

        // Verifica che il task sia abilitato
        VARIANT_BOOL enabled = VARIANT_FALSE;
        hr = pTask->get_Enabled(&enabled);
        if (SUCCEEDED(hr) && enabled == VARIANT_TRUE) {
            // Verifica che il percorso dell'exe sia corretto
            ITaskDefinition* pDef = nullptr;
            hr = pTask->get_Definition(&pDef);
            if (SUCCEEDED(hr) && pDef) {
                IActionCollection* pActions = nullptr;
                hr = pDef->get_Actions(&pActions);
                if (SUCCEEDED(hr) && pActions) {
                    IAction* pAction = nullptr;
                    hr = pActions->get_Item(1, &pAction);
                    if (SUCCEEDED(hr) && pAction) {
                        IExecAction* pExecAction = nullptr;
                        hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
                        if (SUCCEEDED(hr) && pExecAction) {
                            BSTR path = nullptr;
                            hr = pExecAction->get_Path(&path);
                            if (SUCCEEDED(hr) && path) {
                                std::wstring taskPath(path, SysStringLen(path));
                                std::wstring exePath = getExePath();
                                result = (_wcsicmp(taskPath.c_str(), exePath.c_str()) == 0);
                                SysFreeString(path);
                            }
                            pExecAction->Release();
                        }
                        pAction->Release();
                    }
                    pActions->Release();
                }
                pDef->Release();
            }
        }
    } while (false);

    if (pTask) pTask->Release();
    if (pFolder) pFolder->Release();
    if (pService) pService->Release();

    if (needsUninit) {
        CoUninitialize();
    }

    return result;
}

bool setTaskEnabled(bool enable) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    bool needsUninit = SUCCEEDED(hr);

    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }

    bool result = false;
    ITaskService* pService = nullptr;
    ITaskFolder* pFolder = nullptr;
    ITaskDefinition* pTask = nullptr;
    IRegistrationInfo* pRegInfo = nullptr;
    IPrincipal* pPrincipal = nullptr;
    ITaskSettings* pSettings = nullptr;
    ITriggerCollection* pTriggerCollection = nullptr;
    ITrigger* pTrigger = nullptr;
    ILogonTrigger* pLogonTrigger = nullptr;
    IActionCollection* pActionCollection = nullptr;
    IAction* pAction = nullptr;
    IExecAction* pExecAction = nullptr;
    IRegisteredTask* pRegisteredTask = nullptr;

    do {
        // Crea il servizio Task Scheduler
        hr = CoCreateInstance(
            CLSID_TaskScheduler,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITaskService,
            (void**)&pService
        );
        if (FAILED(hr)) break;

        // Connetti al servizio
        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) break;

        // Ottieni la cartella root
        hr = pService->GetFolder(_bstr_t(TASK_FOLDER), &pFolder);
        if (FAILED(hr)) break;

        if (!enable) {
            // Rimuovi il task esistente
            hr = pFolder->DeleteTask(_bstr_t(TASK_NAME), 0);
            // Se il task non esiste, consideriamo comunque successo
            result = SUCCEEDED(hr) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            break;
        }

        // Crea una nuova definizione del task
        hr = pService->NewTask(0, &pTask);
        if (FAILED(hr)) break;

        // Imposta le informazioni di registrazione
        hr = pTask->get_RegistrationInfo(&pRegInfo);
        if (FAILED(hr)) break;

        hr = pRegInfo->put_Author(_bstr_t(TASK_AUTHOR));
        if (FAILED(hr)) break;

        hr = pRegInfo->put_Description(_bstr_t(TASK_DESCRIPTION));
        if (FAILED(hr)) break;

        // Imposta il principal (esegui come utente corrente)
        hr = pTask->get_Principal(&pPrincipal);
        if (FAILED(hr)) break;

        hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        if (FAILED(hr)) break;

        hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_LUA);  // Non richiede elevazione
        if (FAILED(hr)) break;

        // Imposta le settings del task
        hr = pTask->get_Settings(&pSettings);
        if (FAILED(hr)) break;

        hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
        if (FAILED(hr)) break;

        hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        if (FAILED(hr)) break;

        hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        if (FAILED(hr)) break;

        hr = pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S"));  // Nessun limite
        if (FAILED(hr)) break;

        // Crea il trigger di logon
        hr = pTask->get_Triggers(&pTriggerCollection);
        if (FAILED(hr)) break;

        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (FAILED(hr)) break;

        hr = pTrigger->QueryInterface(IID_ILogonTrigger, (void**)&pLogonTrigger);
        if (FAILED(hr)) break;

        hr = pLogonTrigger->put_Id(_bstr_t(L"LogonTriggerId"));
        if (FAILED(hr)) break;

        // Ritardo di 5 secondi dopo il login per dare tempo al sistema
        hr = pLogonTrigger->put_Delay(_bstr_t(L"PT5S"));
        if (FAILED(hr)) break;

        // Crea l'azione (esegui l'exe)
        hr = pTask->get_Actions(&pActionCollection);
        if (FAILED(hr)) break;

        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (FAILED(hr)) break;

        hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
        if (FAILED(hr)) break;

        std::wstring exePath = getExePath();
        hr = pExecAction->put_Path(_bstr_t(exePath.c_str()));
        if (FAILED(hr)) break;

        // Prima elimina il task se esiste gia'
        pFolder->DeleteTask(_bstr_t(TASK_NAME), 0);

        // Registra il task
        hr = pFolder->RegisterTaskDefinition(
            _bstr_t(TASK_NAME),
            pTask,
            TASK_CREATE_OR_UPDATE,
            _variant_t(),
            _variant_t(),
            TASK_LOGON_INTERACTIVE_TOKEN,
            _variant_t(L""),
            &pRegisteredTask
        );

        result = SUCCEEDED(hr);

    } while (false);

    // Cleanup
    if (pRegisteredTask) pRegisteredTask->Release();
    if (pExecAction) pExecAction->Release();
    if (pAction) pAction->Release();
    if (pActionCollection) pActionCollection->Release();
    if (pLogonTrigger) pLogonTrigger->Release();
    if (pTrigger) pTrigger->Release();
    if (pTriggerCollection) pTriggerCollection->Release();
    if (pSettings) pSettings->Release();
    if (pPrincipal) pPrincipal->Release();
    if (pRegInfo) pRegInfo->Release();
    if (pTask) pTask->Release();
    if (pFolder) pFolder->Release();
    if (pService) pService->Release();

    if (needsUninit) {
        CoUninitialize();
    }

    return result;
}

bool hasLegacyRegistryAutostart() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_READ, &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    wchar_t value[MAX_PATH] = {0};
    DWORD size = sizeof(value);
    DWORD type = 0;

    result = RegQueryValueExW(hKey, AUTOSTART_KEY_NAME, NULL, &type,
                              (LPBYTE)value, &size);

    RegCloseKey(hKey);

    return (result == ERROR_SUCCESS && type == REG_SZ);
}

bool removeLegacyRegistryAutostart() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey
    );

    if (result != ERROR_SUCCESS) {
        return false;
    }

    result = RegDeleteValueW(hKey, AUTOSTART_KEY_NAME);

    RegCloseKey(hKey);

    // Se il valore non esiste, consideriamo comunque successo
    return (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND);
}

} // namespace taskscheduler
