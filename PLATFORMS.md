# Supporto multi-piattaforma (Windows, macOS, Linux)

Lo script Python e l’UI Flutter sono pensati per funzionare su **Windows**, **macOS** e **Linux**. L’esperienza è la stessa; alcune funzioni usano API di sistema diverse.

## Cosa funziona uguale ovunque

- **Serial/Arduino**: connessione, lettura pulsanti, encoder, moduli (slider, knob, media, piano). Le porte sono diverse (`COM3` su Windows, `/dev/cu.usbserial-*` su macOS, `/dev/ttyUSB*` su Linux); l’app le elenca dal backend.
- **Config**: path unificati (vedi sotto). Stesso file `config.json` e stesse voci (serial, mappings, special_modules).
- **Azioni**: URL, app, hotkey, digitazione testo, mute e volume (tastiera) funzionano su tutte le piattaforme.
- **Media deck e piano**: VLC è cross‑platform; le note di fallback su macOS/Linux usano `afplay`/`aplay` se VLC non è disponibile.

## Differenze per piattaforma

| Funzione            | Windows              | macOS                    | Linux                         |
|---------------------|----------------------|---------------------------|-------------------------------|
| **Volume** | Tasti volume (WinAPI/APPCOMMAND) | Tasti volume (simulazione) | Tasti volume (simulazione)    |
| **Luminosità**      | sbc (WMI/VCP)        | Non supportata da sbc*    | sbc (es. `sys/backlight`)     |
| **Temperatura CPU** | psutil o WMI         | psutil                    | psutil                        |
| **GPU / nvidia-smi**| Opzionale            | N/A                       | Se presente, usata per stats  |
| **Piano (nota default)** | VLC o winsound   | VLC o afplay              | VLC o aplay                   |

\* Su macOS la libreria `screen-brightness-control` non è supportata; le azioni di luminosità vanno in errore in log ma il resto dell’app continua a funzionare.

## Path di configurazione

- **Windows**: `%APPDATA%\ConsoleDeckPro\config.json`
- **macOS**: `~/Library/Application Support/ConsoleDeckPro/config.json`
- **Linux**: `$XDG_CONFIG_HOME/ConsoleDeckPro/config.json` oppure `~/.config/ConsoleDeckPro/config.json`

L’UI Flutter usa gli stessi path (e migra da eventuali path legacy se presenti).

## Dipendenze Python

- **Comuni**: `fastapi`, `uvicorn`, `pyserial`, `pyautogui`, `psutil`, `screen-brightness-control`, `python-vlc`
- **Solo Windows** (opzionali): `GPUtil` (GPU)

## Requisiti extra su Linux

- **Luminosità**: permessi in lettura/scrittura su `/sys/class/backlight/...` (o uso di `ddcutil`/`xrandr` a seconda del backend sbc).
- **Piano (fallback)**: `aplay` (pacchetto `alsa-utils`) per le note predefinite senza VLC.

In sintesi: sì, puoi usarlo su macOS e Linux allo stesso modo; volume e luminosità hanno fallback o limiti descritti sopra.
