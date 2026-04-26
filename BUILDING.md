# Building from source

## Prerequisites

- Python 3.10+ with pip
- [Flutter SDK](https://flutter.dev/docs/get-started/install) (stable channel)
- [Inno Setup 6](https://jrsoftware.org/isinfo.php) (Windows installer, optional)
- PyInstaller: `pip install pyinstaller`

---

## 1 — Build the Python backend

From the project root:

```bash
py -m PyInstaller --noconfirm --onefile --noconsole --name console_deck_pro console_deck_pro.py
```

Output: `dist\console_deck_pro.exe`

---

## 2 — Build the Flutter UI

```bash
cd UI
flutter build windows
```

Output: `UI\build\windows\x64\runner\Release\console_deck_ui.exe`

To include the skin creator feature, pass the API credentials at build time:

```bash
flutter build windows \
  --dart-define=SKIN_API_URL=https://your-cloud-function-url \
  --dart-define=SKIN_API_KEY=your-api-key
```

---

## 3 — Build the Windows installer

1. Copy `dist\console_deck_pro.exe` to the project root (the `.iss` script expects it there)
2. Open `UI\setup.iss` in Inno Setup Compiler
3. Press **Compile** (Ctrl+F9)

Output: `UI\Output\ConsoleDeckPro_Setup.exe`

**Optional:** add a custom app icon by placing `app_icon.ico` in `UI\windows\runner\resources\` and enabling the icon resource in `UI\windows\runner\Runner.rc`.

---

## Installer behavior

- Installs the Flutter UI and the Python backend exe
- Copies `console_deck_pro.py`, `module_extensions.py`, `requirements.txt`, and `config.example.json` for manual launch / reference
- Does **not** auto-start the backend (manual launch only)
- Cleans up legacy files from older installer versions on upgrade

---

## Running in development (no installer)

```bash
# Terminal 1 — start the backend
python console_deck_pro.py

# Terminal 2 — run the UI in debug mode
cd UI
flutter run -d windows
```
