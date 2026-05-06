# Console Deck PRO

A DIY macro deck built around an Arduino Nano — 9 programmable buttons, a rotary encoder, an OLED display, and an expandable module system (sliders, knobs, extra buttons). Each button can open a URL, launch an app, send a hotkey, control volume or screen brightness, and more. Configure everything from the desktop UI, no code editing required.

This is an **open-source maker project**. The full package (3D-printable enclosure files, electronics schematics, and BOM) is available as part of the crowdfunding campaign.

---

## What's in this repo

| Folder / File | Description |
|---|---|
| `console_deck_pro_arduino/` | Arduino firmware (upload once to the board) |
| `UI/` | Flutter desktop app (Windows / macOS / Linux) |
| `console_deck_pro.py` | Python backend — bridges the Arduino with the OS |
| `config.example.json` | Example configuration — copy to get started |
| `requirements.txt` | Python dependencies |
| `BUILDING.md` | How to build the installer from source |
| `PLATFORMS.md` | Platform-specific notes (Windows / macOS / Linux) |

---

## Hardware you need

- Arduino Nano (ATmega328P)
- SH1107 128×128 OLED display (I2C, Seeed variant)
- 9 momentary push buttons
- 1 rotary encoder with push button
- USB cable (Nano → PC)
- *(optional)* expansion module: 2× analog sliders, 2× knobs, or 6× extra buttons

Refer to the campaign page for the full schematic, BOM, and 3D-printable files.

---

## Getting started (Windows)

### Step 1 — Download this repo

Click **Code → Download ZIP** on GitHub, then extract the folder anywhere on your PC.

> Alternatively: `git clone https://github.com/LucaDiLorenzo98/console_deck_pro.git`

---

### Step 2 — Flash the Arduino firmware

You only need to do this once.

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Open Arduino IDE → **Sketch → Include Library → Manage Libraries** → search **U8g2** → Install
3. In the extracted folder, open `console_deck_pro_arduino/console_deck_pro_arduino.ino`
4. Plug the Arduino Nano into your PC via USB
5. Set **Tools → Board → Arduino AVR Boards → Arduino Nano**
6. Set **Tools → Processor → ATmega328P** *(try "Old Bootloader" if upload fails)*
7. Set **Tools → Port** → select the COM port that appeared when you plugged in the Nano
8. Click **Upload** (the → arrow)

The OLED display should light up and show the boot screen. Done — you never need to touch Arduino IDE again.

---

### Step 3 — Install the desktop software

1. Go to the [latest release](../../releases/latest) on GitHub
2. Download `ConsoleDeckPro_Setup.exe`
3. Run the installer and follow the wizard

The installer includes everything: the UI app and the backend — no Python installation required.

---

### Step 4 — First launch

1. Open **Console Deck PRO** from the Start menu (or desktop shortcut)
2. The backend starts automatically in the background — nothing extra to run
3. Go to **Settings** and select the Arduino serial port (e.g. `COM3`)
4. The OLED display will show the connected state and begin displaying PC stats

> **Tip:** enable **Start with Windows** in Settings to have the app launch automatically at login.

---

### Step 5 — Configure your buttons

1. Go to the **Modules** tab in the app
2. Click any button slot to assign an action
3. Changes save automatically and take effect immediately

---

## Button actions

| Action | What it does |
|---|---|
| `open_url` | Opens a URL in the default browser |
| `open_app` | Launches an executable by name |
| `hotkey` | Sends a key combination (e.g. `Ctrl+C`, `Win+PrintScreen`) |
| `type_text` | Types a text string |
| `mute` | Toggles system mute |
| `set_volume` | Maps slider/knob to system volume |
| `set_brightness` | Maps slider/knob to screen brightness |
| `home_assistant` | Triggers a Home Assistant service call |

---

## Expansion modules

Connect an expansion board to the side connector and select the module type from the on-device menu:

- **Sliders** — 2 analog sliders (typically volume + brightness)
- **Knobs** — 2 rotary knobs
- **Buttons** — 6 additional momentary buttons
- **Media deck** — crossfader between two audio tracks
- **Piano** — 12-key synth with custom sample support

---

## Manual setup (macOS / Linux or advanced users)

1. Install Python 3.10+
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
3. Start the backend:
   ```bash
   python console_deck_pro.py
   ```
4. Build or run the Flutter UI:
   ```bash
   cd UI
   flutter run -d windows   # or macos / linux
   ```

See [PLATFORMS.md](PLATFORMS.md) for platform-specific notes.

---

## Building from source

See [BUILDING.md](BUILDING.md) for instructions on building the Python backend, the Flutter app, and the Windows installer.

---

## License

[MIT](LICENSE)
