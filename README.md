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
| `module_extensions.py` | Audio module support (media deck, piano synth) |
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

## Getting started

### 1 — Flash the firmware

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install the required library: open **Sketch → Include Library → Manage Libraries**, search for **U8g2** and install it
3. Open `console_deck_pro_arduino/console_deck_pro_arduino.ino` in Arduino IDE
4. Select **Tools → Board → Arduino AVR Boards → Arduino Nano**
5. Select **Tools → Processor → ATmega328P** (or *ATmega328P (Old Bootloader)* if upload fails)
6. Select **Tools → Port** → choose the COM port for your Nano
7. Click **Upload** (→ arrow button)

The display should light up and show the Console Deck PRO boot screen.

---

### 2 — Install the desktop software (Windows)

Download `ConsoleDeckPro_Setup.exe` from the [latest release](../../releases/latest) and run it. The installer includes both the UI app and the Python backend files.

#### Manual setup (macOS / Linux or advanced users)

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

---

### 3 — First configuration

1. Start the Python backend (`console_deck_pro.py` or the installed exe)
2. Open the Console Deck PRO desktop app
3. Go to **Settings** and select the Arduino serial port (e.g. `COM3` on Windows, `/dev/ttyUSB0` on Linux)
4. The OLED display will show a connected state and start sending PC stats
5. Go to **Modules** to map each button to an action

A working starting point is provided in `config.example.json` — the app will guide you through the rest.

---

## Button actions

| Action | What it does |
|---|---|
| `open_url` | Opens a URL in the default browser |
| `open_app` | Launches an executable by name |
| `hotkey` | Sends a key combination (e.g. `["ctrl", "c"]`) |
| `type_text` | Types a text string |
| `mute` | Toggles system mute |
| `set_volume` | Maps slider/knob to system volume |
| `set_brightness` | Maps slider/knob to screen brightness |

---

## Expansion modules

Connect an expansion board to the side connector and select the module type from the on-device menu:

- **Sliders** — 2 analog sliders (typically volume + brightness)
- **Knobs** — 2 rotary knobs
- **Buttons** — 6 additional momentary buttons
- **Media deck** — crossfader between two audio tracks
- **Piano** — 12-key synth with custom sample support

---

## Platform support

See [PLATFORMS.md](PLATFORMS.md) for a full breakdown of what works on Windows, macOS, and Linux.

---

## Building from source

See [BUILDING.md](BUILDING.md) for instructions on building the Python backend exe, the Flutter app, and the Windows installer.

---

## License

[MIT](LICENSE)
