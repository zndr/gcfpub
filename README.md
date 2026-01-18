# MilleWin CF Extractor

Applicazione per Windows che estrae il codice fiscale dalla finestra di MilleWin e lo copia negli appunti tramite hotkey.

## Download

Sono disponibili due versioni:

| Versione | File | Descrizione |
|----------|------|-------------|
| **Portabile** | `mwcf_extractor.exe` | Singolo eseguibile, nessuna installazione richiesta |
| **Installer** | `MWCFExtractor_Setup_x.x.x.exe` | Installazione completa con Menu Start e opzioni avanzate |

### Differenze tra le versioni

| Caratteristica | Portabile | Installer |
|----------------|-----------|-----------|
| Installazione | Nessuna | Program Files |
| Configurazione | Cartella dell'exe | %APPDATA%\MWCFExtractor |
| Avvio automatico | Task Scheduler | Task Scheduler (opzionale) |
| Menu Start | No | Si |
| Disinstallazione | Elimina il file | Pannello di controllo |
| Aggiornamento | Sostituisci il file | Esegui nuovo installer |

**Consiglio**: Usa la versione **Portabile** se vuoi un singolo file da tenere ovunque. Usa l'**Installer** se preferisci un'installazione tradizionale con icona nel Menu Start.

## Requisiti

- Windows 10/11 (x64)
- MilleWin installato

## Utilizzo

1. Avvia l'applicazione (o installala con l'installer)
2. L'icona "CF" appare nella system tray (area notifiche)
3. Con MilleWin aperto e un paziente selezionato, premi **CTRL + 1** (tastierino numerico)
4. Il codice fiscale viene copiato negli appunti

### Menu System Tray

Click destro sull'icona nella system tray per:
- **Salva hotkey**: Salva la configurazione corrente
- **Cambia hotkey...**: Modifica la combinazione di tasti
- **Avvio automatico**: Abilita/disabilita l'avvio con Windows
- **Informazioni**: Mostra informazioni sull'applicazione
- **Esci**: Chiude l'applicazione

### Notifiche

L'applicazione mostra notifiche overlay nell'angolo in basso a destra:
- **Verde**: Codice fiscale copiato con successo
- **Giallo**: Avviso (nessun paziente, MilleWin non trovato)
- **Rosso**: Errore

## Caratteristiche

- **Hotkey configurabile**: CTRL/ALT/SHIFT + 0-9 (tastierino numerico)
- **Conversione omocodia**: Converte automaticamente i CF con caratteri omocodici
- **Multi-monitor**: Supporto completo per configurazioni multi-monitor con DPI diversi
- **Avvio automatico**: Usa Task Scheduler per aggirare i controlli di Windows 11
- **Configurazione persistente**: Salva le impostazioni in un file .ini

## Note tecniche

- L'applicazione cerca finestre del processo `millewin.exe` con classe `FNWND*`
- Il codice fiscale viene estratto dal titolo della finestra usando una regex
- I caratteri omocodici (L, M, N, P, Q, R, S, T, U, V) vengono convertiti in cifre
- L'avvio automatico usa Task Scheduler invece del Registry per compatibilità con Windows 11

## Compilazione (per sviluppatori)

### Requisiti
- Visual Studio 2022 (o Build Tools)
- CMake 3.16+
- Inno Setup 6 (solo per l'installer)

### Build

```batch
build.bat
```

### Creare l'installer

```batch
installer\build_installer.bat
```

## Licenza

Public Domain
