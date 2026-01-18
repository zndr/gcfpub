# MilleWin CF Extractor

Applicazione portabile per Windows che estrae il codice fiscale dalla finestra di MilleWin e lo copia negli appunti.

## Requisiti

- Windows 10/11 (x64)
- MilleWin installato

## Compilazione

### Build rapida (consigliata)

1. Aprire il "Developer Command Prompt for VS 2022"
2. Navigare nella cartella del progetto
3. Eseguire:

```batch
build.bat
```

Lo script:
- Genera automaticamente l'icona dell'applicazione (`res/app.ico`)
- Configura e compila il progetto
- Copia l'eseguibile nella root del progetto

### Build manuale

```batch
:: Genera l'icona (opzionale)
cd tools
cl /EHsc /O2 generate_icon.cpp /Fe:generate_icon.exe user32.lib gdi32.lib
generate_icon.exe
cd ..

:: Compila
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Utilizzo

1. Avviare `mwcf_extractor.exe`
2. L'applicazione si avvia nella system tray (area notifiche) con icona "CF"
3. All'avvio, registra la combinazione **CTRL + 1 (tastierino numerico)**
4. Con MilleWin aperto e un paziente selezionato, premere la hotkey
5. Il codice fiscale viene copiato negli appunti

### Menu System Tray

Click destro sull'icona nella system tray per:
- **Salva hotkey**: Salva la configurazione corrente su file
- **Cambia hotkey...**: Modifica la combinazione di tasti
- **Avvio automatico**: Abilita/disabilita l'avvio con Windows
- **Informazioni**: Mostra informazioni sull'applicazione
- **Esci**: Chiude l'applicazione

### Notifiche

L'applicazione mostra notifiche overlay nell'angolo in basso a destra:
- **Verde**: Codice fiscale copiato con successo
- **Giallo**: Avviso (nessun paziente, MilleWin non trovato)
- **Rosso**: Errore

Le notifiche si chiudono automaticamente dopo 3 secondi.

## Caratteristiche

- **Portabile**: Singolo file .exe, nessuna installazione richiesta
- **Icona personalizzata**: Icona "CF" su sfondo verde nella system tray e nell'eseguibile
- **Hotkey configurabile**: CTRL/ALT/SHIFT + 0-9 (tastierino numerico)
- **Conversione omocodia**: Converte automaticamente i CF con caratteri omocodici in formato standard
- **Multi-monitor**: Supporto completo per configurazioni multi-monitor con DPI diversi
- **Configurazione persistente**: Salva le impostazioni in `mwcf_extractor.ini`
- **Avvio automatico**: Opzione per avviare l'app con Windows

## Note tecniche

- L'applicazione cerca finestre del processo `millewin.exe` con classe che inizia per `FNWND`
- Il codice fiscale viene estratto dal titolo della finestra usando una regex completa
- I caratteri omocodici (L, M, N, P, Q, R, S, T, U, V) vengono convertiti in cifre
- La hotkey viene de-registrata automaticamente alla chiusura
- Tutti i messaggi appaiono sempre in primo piano

## File generati

- `mwcf_extractor.ini`: Configurazione (hotkey, autostart)
- Registro Windows: Chiave per avvio automatico (se abilitato)

## Licenza

Public Domain
